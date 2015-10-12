
#include <fstream>
#include <templatious/FullPack.hpp>
#include <crypto++/sha.h>

#include "SignatureCheck.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    bool hashSingleFile(CryptoPP::SHA256& hash,const std::string& string) {
        std::ifstream stream(string.c_str(),std::ios::binary);
        if (!stream.is_open()) {
            return false;
        }

        char buf;
        unsigned char* rein = reinterpret_cast<unsigned char*>(buf);

        // I hate c++ so much, I can't even
        // find how to read some bytes and hash
        // it up in 10 minutes.
        while (stream && !stream.eof()) {
            stream.get(buf);
            hash.Update(rein,1);
        }

        return true;
    }

    void bytesToCStr(char* raw,char* string,int rawNum) {
        TEMPLATIOUS_0_TO_N(i,rawNum) {
            char* strPtr = string + i * 2;
            char toConv = raw[i];
            sprintf(strPtr,"%02x",toConv);
        }
    }

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
    bytesToCStr(bytes,bytesStr,sizeof(bytes));
    return bytesStr;
}

}
