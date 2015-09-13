
#include <iostream>
#include <fstream>
#include <util/DumbHash.hpp>

int main(int argc,char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: dumbhash256 [filename]" << std::endl;
        return 1;
    }

    std::ifstream input(argv[1],std::ios::binary);
    if (!input.is_open()) {
        std::cout << "Cannot open file " << argv[1] << std::endl;
        return 1;
    }

    SafeLists::DumbHash256 hash;
    char c;
    int64_t count = 0;
    while (input.get(c)) {
        hash.add(count,c);
        ++count;
    }
    input.close();

    char out[128];
    hash.toString(out);
    std::cout << out << std::endl;
}

