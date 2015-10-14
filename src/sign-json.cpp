
#include <boost/filesystem.hpp>
#include <meta/SignatureCheck.hpp>

namespace fs = boost::filesystem;

int main(int argc,char* argv[]) {
    if (argc != 2) {
        printf("Usage: signjson <private key path> <signature file>\n");
        return 1;
    }

    std::vector< std::string > paths;
    std::string outSig;
    auto res = SafeLists::getFileList(argv[1],paths,outSig);
    if (res != SafeLists::GetFileListError::Success) {
        if (res == SafeLists::GetFileListError::InvalidJson) {
            printf("Invalid json...\n");
        } else if (res == SafeLists::GetFileListError::CouldNotOpenFile) {
            printf("Could not open file...\n");
        } else if (res == SafeLists::GetFileListError::ParseError) {
            printf("Json parsing error...\n");
        }
        return 1;
    }

    if (outSig != "") {
        printf("Expected empty signature.\n");
        return 1;
    }

    auto signRes = SafeLists::signFileList(argv[0],paths,outSig);
}

