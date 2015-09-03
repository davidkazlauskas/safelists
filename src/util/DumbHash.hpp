
#ifndef DUMBHASH_DGGM3OP4
#define DUMBHASH_DGGM3OP4

#include <cstring>
#include <cstdint>
#include <templatious/FullPack.hpp>

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
                int posA = remainder / 8;
                int posB = posA + 1;
                _mem[posA] ^= byte>>splitPoint;
                if (posB >= 32) {
                    posB = 0;
                }
                _mem[posB] ^= byte<<(8-splitPoint);
            } else {
                _mem[remainder / 8] ^= byte;
            }
        }

        // buf should be at least 65 bytes long.
        template <int bufSize>
        void toString(char (&buf)[bufSize]) const {
            static_assert( bufSize >= 65, "Too small buffer cholo." );
            typedef unsigned char uc;
            sprintf(
                buf,
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                ,
                uc(_mem[0]), uc(_mem[1]), uc(_mem[2]), uc(_mem[3]),
                uc(_mem[4]), uc(_mem[5]), uc(_mem[6]), uc(_mem[7]),
                uc(_mem[8]), uc(_mem[9]), uc(_mem[10]),uc(_mem[11]),
                uc(_mem[12]),uc(_mem[13]),uc(_mem[14]),uc(_mem[15]),
                uc(_mem[16]),uc(_mem[17]),uc(_mem[18]),uc(_mem[19]),
                uc(_mem[20]),uc(_mem[21]),uc(_mem[22]),uc(_mem[23]),
                uc(_mem[24]),uc(_mem[25]),uc(_mem[26]),uc(_mem[27]),
                uc(_mem[28]),uc(_mem[29]),uc(_mem[30]),uc(_mem[31])
            );
        }
    private:
        char _mem[32];
    };

}

#endif /* end of include guard: DUMBHASH_DGGM3OP4 */

