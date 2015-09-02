#include "Misc.hpp"

namespace SafeLists {

bool isLittleEndianMachine() {
    int16_t i16 = 7;
    char* reint = reinterpret_cast<char*>(&i16);
    return reint[0] == 7;
}

void writeI64AsLittleEndian(int64_t number,char array[sizeof(number)]) {
    static bool isLittle = isLittleEndianMachine();
    static_assert( sizeof(number) == 8, "I'm so paranoid it's not even funny." );
}

}
