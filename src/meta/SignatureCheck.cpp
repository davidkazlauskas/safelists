
#include <fstream>
#include <templatious/FullPack.hpp>
#include <crypto++/sha.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
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

    return GetFileListError::Success;
}

}
