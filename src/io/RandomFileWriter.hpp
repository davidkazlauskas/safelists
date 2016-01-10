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

    // Check if file exists.
    // Signature:
    // < DoesFileExist, std::string (path), bool (result) >
    DUMMY_REG(DoesFileExist,"RFW_DoesFileExist");

    // Delete file.
    DUMMY_REG(DeleteFile,"RFW_DeleteFile");

    // Dummy pack. Useful
    // for waiting on writes
    // sent previously before
    // this pack.
    DUMMY_REG(WaitWrites,"RFW_WaitWrites");

    // Write string to file, creating parent
    // directories in progress.
    // Signature:
    // <
    //   WriteStringToFileWDir,
    //   const std::string (path),
    //   const std::string (data),
    //   int (out err code)
    // >
    DUMMY_REG(WriteStringToFileWDir,"RFW_WriteStringToFileWDir");

    // Read string from file.
    // Signature:
    // <
    //   ReadStringFromFile,
    //   const std::string (path),
    //   std::string (out),
    //   int (out err code)
    // >
    DUMMY_REG(ReadStringFromFile,"RFW_ReadStringFromFile");

    // singleton
    static StrongMsgPtr make();
};

}

#endif /* end of include guard: RANDOMFILEWRITER_7DHBRNML */
