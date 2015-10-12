
#include <fstream>
#include <crypto++/sha.h>

#include "SignatureCheck.hpp"

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

}

namespace SafeLists {

std::string hashFileListSha256(const std::vector<std::string>& paths) {
    CryptoPP::SHA256 hash;

    return "";
}

}
