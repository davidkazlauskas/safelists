
#include <fstream>
#include <templatious/FullPack.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <openssl/pem.h>
#include <util/ScopeGuard.hpp>

#include "SignatureCheck.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    bool hashSingleFile(SHA256_CTX& hash,const std::string& string) {
        auto file = ::fopen(string.c_str(),"rb");
        if (nullptr == file) {
            return false;
        }
        auto guard = SCOPE_GUARD_LC(
            ::fclose(file);
        );

        char buf;
        unsigned char* rein = reinterpret_cast<unsigned char*>(&buf);

        int state;
        do {
            state = ::fgetc(file);
            if (state != EOF) {
                buf = static_cast<char>(state);
                ::SHA256_Update(&hash,rein,1);
            }
        } while (state != EOF);

        return true;
    }

    void bytesToCStr(char* raw,char* string,int rawNum) {
        TEMPLATIOUS_0_TO_N(i,rawNum) {
            char* strPtr = string + i * 2;
            unsigned char toConv = raw[i];
            sprintf(strPtr,"%02x",toConv);
        }
        string[rawNum * 2 + 1] = '\0';
    }

    char singleByteNumeric(char original) {
        if (original >= 'a' && original <= 'f') {
            return original - 'a' + 10;
        }

        if (original >= 'A' && original <= 'F') {
            return original - 'A' + 10;
        }

        if (original >= '0' && original <= '9') {
            return original - '0';
        }

        assert( false && "Invalid input." );
    }

    unsigned char decodeByte(const char* twoHex) {
        char higher = twoHex[0];
        char lower = twoHex[1];

        unsigned char outRes = 0;

        outRes += singleByteNumeric(higher) * 16;
        outRes += singleByteNumeric(lower);

        return outRes;
    }

    void encodeHexString(int strLen,const char* string,unsigned char* outBuf) {
        assert( strLen % 2 == 0 );
        for (int i = 0; i < strLen; i += 2) {
            int half = i / 2;
            const char* ptr = string + i;

            outBuf[half] = decodeByte(ptr);
        }
    }

    namespace rj = rapidjson;
}

namespace SafeLists {

std::string hashFileListSha256(const std::vector<std::string>& paths) {
    SHA256_CTX ctx;
    ::SHA256_Init(&ctx);

    TEMPLATIOUS_FOREACH(auto& i, paths) {
        bool res = hashSingleFile(ctx,i);
        if (!res) {
            return "";
        }
    }

    char bytes[1024];
    char bytesStr[2048];

    ::SHA256_Final(reinterpret_cast<unsigned char*>(bytes),&ctx);
    bytesToCStr(bytes,bytesStr,SHA256_DIGEST_LENGTH);
    return bytesStr;
}

int hashFileListSha256(
    const std::vector<std::string>& paths,
    unsigned char (&arr)[32])
{
    SHA256_CTX ctx;
    ::SHA256_Init(&ctx);

    TEMPLATIOUS_FOREACH(auto& i, paths) {
        bool res = hashSingleFile(ctx,i);
        if (!res) {
            return 1;
        }
    }

    char bytes[1024];
    char bytesStr[2048];

    ::SHA224_Final(arr,&ctx);
    return 0;
}

GetFileListError getFileListWSignature(
    const char* jsonPath,
    std::vector< std::string >& out,
    std::string& signature)
{
    auto file = ::fopen(jsonPath,"r");
    if (nullptr == file) {
        return GetFileListError::CouldNotOpenFile;
    }
    auto closeGuard = SCOPE_GUARD_LC(
        ::fclose(file);
    );

    // hopefully signature doesn't exceed this...
    char readBuffer[256*256];
    rj::FileReadStream stream(file,readBuffer,sizeof(readBuffer));

    rj::Document d;
    d.ParseStream(stream);
    if (d.HasParseError()) {
        return GetFileListError::ParseError;
    }

    if (!d.HasMember("signature") || !d["signature"].IsString()) {
        return GetFileListError::InvalidJson;
    }
    signature = d["signature"].GetString();

    if (!d.HasMember("files") || !d["files"].IsArray()) {
        return GetFileListError::InvalidJson;
    }
    auto beg = d["files"].MemberBegin();
    auto end = d["files"].MemberEnd();
    for (; beg != end; ++beg) {
        if (!beg->value.IsString()) {
            return GetFileListError::InvalidJson;
        }
        SA::add(out,beg->value.GetString());
    }

    return GetFileListError::Success;
}

SignFileListError signFileList(
    const char* privateKeyPath,
    const std::vector< std::string >& paths,
    std::string& outSig)
{
    auto file = fopen(privateKeyPath,"r");
    if (nullptr == file) {
        return SignFileListError::CouldNotOpenKey;
    }

    auto closeGuard = SCOPE_GUARD_LC(
        fclose(file);
    );

    auto key = ::PEM_read_RSAPrivateKey(file,nullptr,nullptr,nullptr);
    if (nullptr == key) {
        return SignFileListError::KeyReadFail;
    }

    auto rsaFreeGuard = SCOPE_GUARD_LC(
        ::RSA_free(key);
    );

    // close right away, we don't need
    // this no more.
    closeGuard.fire();

    unsigned char hashOfAll[32];
    int res = hashFileListSha256(paths,hashOfAll);
    if (0 != res) {
        return SignFileListError::HashingFailed;
    }

    int outLen = 0;
    unsigned char sigret[1024 * 4];
    outLen = ::RSA_private_encrypt(
        sizeof(hashOfAll),
        hashOfAll,
        sigret,
        key,
        RSA_PKCS1_PADDING);

    if (outLen < 0) {
        return SignFileListError::SigningFailed;
    }

    assert( outLen < sizeof(sigret) );

    char sigRetString[sizeof(sigret) * 2 + 1];
    for (int i = 0; i < outLen; ++i) {
        auto ptr = sigRetString + i * 2;
        sprintf(ptr,"%02x",sigret[i]);
    }

    outSig = sigRetString;
    return SignFileListError::Success;
}

VerifyFileListError verifyFileList(
    const char* publicKeyPath,
    const char* signature,
    const std::vector< std::string >& paths
)
{
    auto file = fopen(publicKeyPath,"r");
    if (nullptr == file) {
        return VerifyFileListError::CouldNotOpenKey;
    }

    auto closeGuard = SCOPE_GUARD_LC(
        fclose(file);
    );

    auto key = ::PEM_read_RSA_PUBKEY(file,nullptr,nullptr,nullptr);
    if (nullptr == key) {
        return VerifyFileListError::KeyReadFail;
    }

    auto rsaFreeGuard = SCOPE_GUARD_LC(
        ::RSA_free(key);
    );

    // close right away, we don't need
    // this no more.
    closeGuard.fire();


    int sigLen = strlen(signature);
    unsigned char originalHash[1024 * 2];
    unsigned char output[64];
    encodeHexString(sigLen,signature,originalHash);
    int recSize = ::RSA_public_decrypt(
        sigLen/2,
        originalHash,
        output,
        key,
        RSA_PKCS1_PADDING);

    if (32 != recSize) {
        return VerifyFileListError::DigestRecoveryFail;
    }

    unsigned char hashOfAll[32];
    int res = hashFileListSha256(paths,hashOfAll);
    if (0 != res) {
        return VerifyFileListError::HashingFailed;
    }

    return 0 == memcmp(hashOfAll,output,sizeof(hashOfAll)) ?
        VerifyFileListError::Success :
        VerifyFileListError::VerificationFailed;
}

}
