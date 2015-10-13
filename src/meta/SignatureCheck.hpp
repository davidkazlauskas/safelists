#ifndef SIGNATURECHECK_DEV9MTG8
#define SIGNATURECHECK_DEV9MTG8

#include <string>
#include <vector>

namespace SafeLists {

std::string hashFileListSha256(const std::vector<std::string>& paths);
std::vector< std::string > getFileList(const char* jsonPath);

}

#endif /* end of include guard: SIGNATURECHECK_DEV9MTG8 */
