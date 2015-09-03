
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
                if (posB < 32) {
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
            sprintf(
                buf,
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                "%02x%02x%02x%02x%02x%02x%02x%02x"
                ,
                _mem[0], _mem[1], _mem[2], _mem[3],
                _mem[4], _mem[5], _mem[6], _mem[7],
                _mem[8], _mem[9], _mem[10],_mem[11],
                _mem[12],_mem[13],_mem[14],_mem[15],
                _mem[16],_mem[17],_mem[18],_mem[19],
                _mem[20],_mem[21],_mem[22],_mem[23],
                _mem[24],_mem[25],_mem[26],_mem[27],
                _mem[28],_mem[29],_mem[30],_mem[31]
            );
        }
    private:
        char _mem[32];
    };

}

#endif /* end of include guard: DUMBHASH_DGGM3OP4 */

