#ifndef RANDOMFILEWRITER_7DHBRNML
#define RANDOMFILEWRITER_7DHBRNML

#include <util/AutoReg.hpp>

namespace SafeLists {

struct RandomFileWriter {

    // write block into file.
    // Signature:
    // <
    //    WriteData,
    //    std::string (filename),
    //    std::unique_ptr< char[] > (data),
    //    int64_t (start),
    //    int64_t (end)
    // >
    DUMMY_REG(WriteData,"RFW_WriteData");

    // Clear cache. All handles,
    // or specified path removed
    // and cache closed.
    // Signature:
    // < ClearCache >
    // < ClearCache, std::string (path) >
    DUMMY_REG(ClearCache,"RFW_ClearCache");

    // Dummy pack. Useful
    // for waiting on writes
    // sent previously before
    // this pack.
    DUMMY_REG(WaitWrites,"RFW_WaitWrites");

    // singleton
    static StrongMsgPtr make();
};

}

#endif /* end of include guard: RANDOMFILEWRITER_7DHBRNML */
