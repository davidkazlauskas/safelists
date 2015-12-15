
#include <fstream>
#include <templatious/FullPack.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <util/ScopeGuard.hpp>
#include <util/Base64.hpp>
#include <sodium.h>

#include "SignatureCheck.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    bool hashSingleFile(
        ::crypto_hash_sha256_state& hash,
        const std::string& rootPath,
        const std::string& relPath)
    {
        std::string rightPath = rootPath + relPath;
        auto file = ::fopen(rightPath.c_str(),"rb");
        if (nullptr == file) {
            return false;
        }
        auto guard = SCOPE_GUARD_LC(
            ::fclose(file);
        );

        char buf;
        unsigned char* rein = reinterpret_cast<unsigned char*>(&buf);

        ::crypto_hash_sha256_update(
            &hash,
            reinterpret_cast<const unsigned char*>(relPath.c_str()),
            relPath.size());

        int state;
        do {
            state = ::fgetc(file);
            if (state != EOF) {
                buf = static_cast<char>(state);
                ::crypto_hash_sha256_update(&hash,rein,1);
            }
        } while (state != EOF);

        return true;
    }

    void bytesToCStr(char* raw,char* string,size_t rawNum) {
        TEMPLATIOUS_0_TO_N(i,rawNum) {
            char* strPtr = string + i * 2;
            unsigned char toConv = raw[i];
            sprintf(strPtr,"%02x",toConv);
        }
        string[rawNum * 2] = '\0';
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
        return -1;
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

std::string hashFileListSha256(
        const std::string& rootPath,
        const std::vector<std::string>& paths)
{
    ::crypto_hash_sha256_state ctx;
    ::crypto_hash_sha256_init(&ctx);

    TEMPLATIOUS_FOREACH(auto& i, paths) {
        bool res = hashSingleFile(ctx,rootPath,i);
        if (!res) {
            return "";
        }
    }

    char bytes[1024];
    char bytesStr[2048];

    ::crypto_hash_sha256_final(
        &ctx,
        reinterpret_cast<unsigned char*>(bytes));
    bytesToCStr(bytes,bytesStr,
        crypto_hash_sha256_bytes());
    return bytesStr;
}

int hashFileListSha256(
    const std::string& rootPath,
    const std::vector<std::string>& paths,
    unsigned char (&arr)[32])
{
    ::crypto_hash_sha256_state ctx;
    ::crypto_hash_sha256_init(&ctx);

    TEMPLATIOUS_FOREACH(auto& i, paths) {
        bool res = hashSingleFile(ctx,rootPath,i);
        if (!res) {
            return 1;
        }
    }

    char bytes[1024];
    char bytesStr[2048];

    ::crypto_hash_sha256_final(&ctx,arr);
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
    auto& arr = d["files"];
    auto end = arr.Size();
    for (int i = 0; i < end ; ++i) {
        if (!arr[i].IsString()) {
            return GetFileListError::InvalidJson;
        }
        SA::add(out,arr[i].GetString());
    }

    return GetFileListError::Success;
}

int readSodiumPrivateKey(
    const char* path,
    unsigned char (&sodiumkey)[crypto_sign_SECRETKEYBYTES])
{
    auto file = ::fopen(path,"r");
    if (nullptr == file) {
        return 1;
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
        return 2;
    }

    if (!d.IsObject()) {
        return 3;
    }

    if (   !d.HasMember("keytype")
        || !d.HasMember("privatekeybase64")
        || !d.HasMember("publickeybase64"))
    {
        return 4;
    }

    auto& keytype = d["keytype"];
    auto& privatekeybase64 = d["privatekeybase64"];
    auto& publickeybase64 = d["publickeybase64"];

    if (   !keytype.IsString()
        || !privatekeybase64.IsString()
        || !publickeybase64.IsString())
    {
        return 5;
    }

    if (0 != strcmp(keytype.GetString(),"crypto_sign")) {
        return 6;
    }

    std::vector<char> baseCpy(
        privatekeybase64.GetStringLength());
    std::vector<unsigned char> cpyCpy(
        SA::size(baseCpy));

    ::memcpy(
        baseCpy.data(),
        privatekeybase64.GetString(),
        SA::size(baseCpy));

    size_t outSize = 0;

    int res = ::base64decode(
        baseCpy.data(),SA::size(baseCpy),
        cpyCpy.data(),&outSize);

    if (0 != res) {
        return 7;
    }

    if (outSize != sizeof(sodiumkey)) {
        return 8;
    }

    ::memcpy(sodiumkey,cpyCpy.data(),sizeof(sodiumkey));
    return 0;
}

SignFileListError signFileList(
    const char* privateKeyPath,
    const std::string& rootPath,
    const std::vector< std::string >& paths,
    std::string& outSig)
{
    unsigned char pkey[crypto_sign_SECRETKEYBYTES];
    int readRes = readSodiumPrivateKey(privateKeyPath,pkey);

    unsigned char hashOfAll[32];
    int res = hashFileListSha256(rootPath,paths,hashOfAll);
    if (0 != res) {
        return SignFileListError::HashingFailed;
    }

    char bytesStr[128];
    bytesToCStr(reinterpret_cast<char*>(hashOfAll),
        bytesStr,sizeof(hashOfAll));

    const int HASH_IN_STRING = sizeof(hashOfAll) * 2 + 1;

    unsigned long long outLen = 0;
    unsigned char sigret[1024];
    res = ::crypto_sign_detached(
        sigret,&outLen,
        reinterpret_cast<unsigned char*>(bytesStr),
        HASH_IN_STRING,pkey);
    if (0 != res) {
        return SignFileListError::SigningFailed;
    }

    assert( outLen < sizeof(sigret) );

    char sigRetString[sizeof(sigret) * 2 + 1];
    bytesToCStr(
        reinterpret_cast<char*>(sigret),
        sigRetString,
        sizeof(sigret));

    outSig = sigRetString;
    return SignFileListError::Success;
}

VerifyFileListError verifyFileList(
    const char* publicKeyPath,
    const char* signature,
    const std::string& rootPath,
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

    //auto key = ::PEM_read_RSA_PUBKEY(file,nullptr,nullptr,nullptr);
    //if (nullptr == key) {
        //return VerifyFileListError::KeyReadFail;
    //}

    //auto rsaFreeGuard = SCOPE_GUARD_LC(
        //::RSA_free(key);
    //);

    // close right away, we don't need
    // this no more.
    closeGuard.fire();


    int sigLen = strlen(signature);
    unsigned char originalHash[1024 * 2];
    unsigned char output[64];
    encodeHexString(sigLen,signature,originalHash);
    //int recSize = ::RSA_public_decrypt(
        //sigLen/2,
        //originalHash,
        //output,
        //key,
        //RSA_PKCS1_PADDING);

    //if (32 != recSize) {
        //return VerifyFileListError::DigestRecoveryFail;
    //}

    assert( false && "Not implemented yet" );

    unsigned char hashOfAll[32];
    int res = hashFileListSha256(rootPath,paths,hashOfAll);
    if (0 != res) {
        return VerifyFileListError::HashingFailed;
    }

    return 0 == memcmp(hashOfAll,output,sizeof(hashOfAll)) ?
        VerifyFileListError::Success :
        VerifyFileListError::VerificationFailed;
}

}
