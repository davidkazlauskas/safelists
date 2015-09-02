#ifndef MISC_REYUOCY4
#define MISC_REYUOCY4

#include <cstdint>

namespace SafeLists {

bool isLittleEndianMachine();
void writeI64AsLittleEndian(const int64_t& number,char (&array)[sizeof(number)]);
void readI64FromLittleEndian(int64_t& number,const char (&array)[sizeof(number)]);

}

#endif /* end of include guard: MISC_REYUOCY4 */
