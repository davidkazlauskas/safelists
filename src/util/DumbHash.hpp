
#ifndef DUMBHASH_DGGM3OP4
#define DUMBHASH_DGGM3OP4

#include <cstring>
#include <cstdint>

namespace SafeLists {

    // the only advantage
    // of this hash vs sha256
    // and stuff like that
    // is that you can sum bytes
    // in any order and the result will be the same;
    struct DumbHash256 {

        DumbHash256() {
            memset(_mem,'7',sizeof(_mem));
        }

        DumbHash256(const DumbHash256&) = default;

        void add(int64_t pos,char byte) {
            int remainder = pos % 256;
            int splitPoint = remainder % 8;

            if (splitPoint != 0) {
            
            } else {
                _mem[remainder / 8] ^= byte;
            }

        }

    private:


        char _mem[32];
    };

}

#endif /* end of include guard: DUMBHASH_DGGM3OP4 */

