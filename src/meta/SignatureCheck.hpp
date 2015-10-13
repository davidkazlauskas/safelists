#ifndef SIGNATURECHECK_DEV9MTG8
#define SIGNATURECHECK_DEV9MTG8

#include <string>
#include <vector>

namespace SafeLists {

std::string hashFileListSha256(const std::vector<std::string>& paths);

enum class GetFileListError {
    Success,
    CouldNotOpenFile,
    ParseError,
    InvalidJson
};

GetFileListError getFileList(
    const char* jsonPath,
    std::vector< std::string >& out,
    std::string& outSignature
);

}

#endif /* end of include guard: SIGNATURECHECK_DEV9MTG8 */
