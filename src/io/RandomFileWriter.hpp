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

    // Clear cache. All handles removed
    // and cache closed.
    // Signature:
    // < ClearCache >
    DUMMY_REG(ClearCache,"RFW_ClearCache");

    // singleton
    static StrongMsgPtr make();
};

}

#endif /* end of include guard: RANDOMFILEWRITER_7DHBRNML */
