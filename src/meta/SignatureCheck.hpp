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

enum class SignFileListError {
    Success,
    CouldNotOpenKey,
    KeyReadFail,
    HashingFailed,
    SigningFailed
};

SignFileListError signFileList(
    const char* privateKeyPath,
    const std::vector< std::string >& paths,
    std::string& outSig
);

}

#endif /* end of include guard: SIGNATURECHECK_DEV9MTG8 */
