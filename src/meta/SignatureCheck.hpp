#ifndef SIGNATURECHECK_DEV9MTG8
#define SIGNATURECHECK_DEV9MTG8

#include <string>
#include <vector>

namespace SafeLists {

std::string hashFileListSha256(
    const std::string& rootPath,
    const std::vector<std::string>& paths);

enum class GetFileListError {
    Success,
    CouldNotOpenFile,
    ParseError,
    InvalidJson
};

GetFileListError getFileListWSignature(
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
    const std::string& rootPath,
    const std::vector< std::string >& paths,
    std::string& outSig
);

enum class VerifyFileListError {
    Success,
    VerificationFailed,
    CouldNotOpenKey,
    KeyReadFail,
    DigestRecoveryFail,
    InvalidSignatureFail,
    HashingFailed
};

VerifyFileListError verifyFileListB64(
    const char* publicKeyBase64,
    const char* signature,
    const std::string& rootPath,
    const std::vector< std::string >& paths
);

VerifyFileListError verifyFileList(
    const char* publicKeyPath,
    const char* signature,
    const std::string& rootPath,
    const std::vector< std::string >& paths
);

}

#endif /* end of include guard: SIGNATURECHECK_DEV9MTG8 */
