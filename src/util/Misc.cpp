
#include <cstring>

#include "Misc.hpp"

namespace {
    void flipBytes(char& a,char& b) {
        char tmp = a;
        a = b;
        b = tmp;
    }

    static bool isLittle = SafeLists::isLittleEndianMachine();
}

namespace SafeLists {

bool isLittleEndianMachine() {
    int16_t i16 = 7;
    char* reint = reinterpret_cast<char*>(&i16);
    return reint[0] == 7;
}

void writeI64AsLittleEndian(int64_t number,char array[sizeof(number)]) {
    static_assert( sizeof(number) == 8, "I'm so paranoid it's not even funny." );
    memcpy(array,&number,sizeof(number));
    if (!isLittle) {
        flipBytes(array[0],array[7]);
        flipBytes(array[1],array[6]);
        flipBytes(array[2],array[5]);
        flipBytes(array[3],array[4]);
    }
}

}
