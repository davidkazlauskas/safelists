
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

int readSodiumPublicKey(
    const char* path,
    unsigned char (&sodiumkey)[crypto_sign_PUBLICKEYBYTES])
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

    closeGuard.fire();

    if (d.HasParseError()) {
        return 2;
    }

    if (!d.IsObject()) {
        return 3;
    }

    if (   !d.HasMember("keytype")
        || !d.HasMember("publickeybase64"))
    {
        return 4;
    }

    auto& keytype = d["keytype"];
    auto& publickeybase64 = d["publickeybase64"];

    if (   !keytype.IsString()
        || !publickeybase64.IsString())
    {
        return 5;
    }

    if (0 != strcmp(keytype.GetString(),"crypto_sign")) {
        return 6;
    }

    std::vector<char> baseCpy(
        publickeybase64.GetStringLength());
    std::vector<unsigned char> cpyCpy(
        SA::size(baseCpy));

    ::memcpy(
        baseCpy.data(),
        publickeybase64.GetString(),
        SA::size(baseCpy));

    size_t outSize = SA::size(cpyCpy);

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
        || !d.HasMember("secretkeybase64")
        || !d.HasMember("publickeybase64"))
    {
        return 4;
    }

    auto& keytype = d["keytype"];
    auto& privatekeybase64 = d["secretkeybase64"];
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

    size_t outSize = baseCpy.size();

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

    const int HASH_IN_STRING = sizeof(hashOfAll) * 2;

    unsigned long long outLen = 0;
    unsigned char sigret[crypto_sign_BYTES];
    res = ::crypto_sign_detached(
        sigret,&outLen,
        reinterpret_cast<unsigned char*>(bytesStr),
        HASH_IN_STRING,pkey);
    if (0 != res) {
        return SignFileListError::SigningFailed;
    }

    assert( outLen < sizeof(sigret) );

    char sigRetString[sizeof(sigret) * 2];
    int enc = ::base64encode(
        sigret,sizeof(sigret),
        sigRetString,sizeof(sigRetString));

    assert( enc == 0 && "Aww, cmon..." );

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
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    int res = readSodiumPublicKey(publicKeyPath,pk);
    if (0 != res) {
        return VerifyFileListError::KeyReadFail;
    }

    unsigned char hashOfAll[32];
    res = hashFileListSha256(rootPath,paths,hashOfAll);
    if (0 != res) {
        return VerifyFileListError::HashingFailed;
    }

    char hashString[2 * sizeof(hashOfAll) + 1];
    bytesToCStr(
        reinterpret_cast<char*>(hashOfAll),
        hashString,sizeof(hashOfAll));

    unsigned char signatureNoHex[crypto_sign_BYTES];
    size_t outLen = crypto_sign_BYTES;
    int sigLen = strlen(signature);
    std::vector< char > cpy(sigLen);
    ::strcpy(cpy.data(),signature);

    res = ::base64decode(cpy.data(),sigLen,signatureNoHex,&outLen);
    if (0 != res || crypto_sign_BYTES != outLen) {
        return VerifyFileListError::InvalidSignatureFail;
    }

    const int HASH_IN_STRING = sizeof(hashOfAll) * 2;
    int ver = crypto_sign_verify_detached(
        signatureNoHex,
        reinterpret_cast<unsigned char*>(hashString),
        HASH_IN_STRING,pk);

    return 0 == ver ?
        VerifyFileListError::Success :
        VerifyFileListError::VerificationFailed;
}

}
