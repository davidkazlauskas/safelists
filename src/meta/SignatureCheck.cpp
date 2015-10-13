
#include <fstream>
#include <templatious/FullPack.hpp>
#include <crypto++/sha.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <openssl/pem.h>
#include <util/ScopeGuard.hpp>

#include "SignatureCheck.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    bool hashSingleFile(CryptoPP::SHA256& hash,const std::string& string) {
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
                hash.Update(rein,1);
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

    namespace rj = rapidjson;
}

namespace SafeLists {

std::string hashFileListSha256(const std::vector<std::string>& paths) {
    CryptoPP::SHA256 hash;

    TEMPLATIOUS_FOREACH(auto& i, paths) {
        bool res = hashSingleFile(hash,i);
        if (!res) {
            return "";
        }
    }

    char bytes[1024];
    char bytesStr[2048];

    hash.Final(reinterpret_cast<unsigned char*>(bytes));
    bytesToCStr(bytes,bytesStr,hash.DigestSize());
    return bytesStr;
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
    const char* keyPath,
    const std::vector< std::string >& paths)
{
    auto file = fopen(keyPath,"r");
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

    // close right away, we don't need
    // this no more.
    closeGuard.fire();

    auto hash = hashFileListSha256(paths);
    if (hash == "") {
        return SignFileListError::HashingFailed;
    }

    auto rsaFreeGuard = SCOPE_GUARD_LC(
        ::RSA_free(key);
    );

    return SignFileListError::Success;
}

}
