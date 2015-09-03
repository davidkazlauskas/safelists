
#ifndef DUMBHASH_DGGM3OP4
#define DUMBHASH_DGGM3OP4

namespace SafeLists {

    // the only advantage
    // of this hash vs sha256
    // and stuff like that
    // is that you can sum bytes
    // in any order and the result will be the same;
    struct DumbHash256 {

    private:
        char _mem[32];
    };

}

#endif /* end of include guard: DUMBHASH_DGGM3OP4 */

