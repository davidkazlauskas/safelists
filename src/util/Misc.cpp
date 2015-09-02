#include "Misc.hpp"

namespace SafeLists {

bool isLittleEndianMachine() {
    int16_t i16 = 7;
    char* reint = reinterpret_cast<char*>(&i16);
    return reint[0] == 7;
}

}
