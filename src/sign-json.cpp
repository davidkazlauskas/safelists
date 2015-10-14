
#include <templatious/FullPack.hpp>
#include <boost/filesystem.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/filewritestream.h>
#include <meta/SignatureCheck.hpp>
#include <util/ScopeGuard.hpp>

namespace fs = boost::filesystem;
namespace rj = rapidjson;

TEMPLATIOUS_TRIPLET_STD;

int main(int argc,char* argv[]) {
    if (argc != 2) {
        printf("Usage: signjson <private key path> <signature file>\n");
        return 1;
    }

    std::vector< std::string > paths;
    std::string outSig;
    auto res = SafeLists::getFileListWSignature(argv[1],paths,outSig);
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

    fs::path sigPath(argv[1]);
    auto parentDir = fs::absolute(sigPath.parent_path());
    std::string absParent = parentDir.string();
    absParent += "/";

    auto signRes = SafeLists::signFileList(argv[0],absParent,paths,outSig);
    if (signRes != SafeLists::SignFileListError::Success) {
        if (signRes == SafeLists::SignFileListError::CouldNotOpenKey) {
            printf("Could not open key...\n");
        } else if (signRes == SafeLists::SignFileListError::KeyReadFail) {
            printf("Invalid key format (PEM expected)...\n");
        } else if (signRes == SafeLists::SignFileListError::HashingFailed) {
            printf("Hasing failed...\n");
        } else if (signRes == SafeLists::SignFileListError::SigningFailed) {
            printf("Signing failed...\n");
        }

        return 1;
    }

    assert( outSig != "" && "Huh?!?" );

    rj::Document out;
    out["signature"] = outSig;

    rj::Value arr(rj::kArrayType);
    TEMPLATIOUS_FOREACH(auto& i,paths) {
        arr.PushBack(i,out.GetAllocator());
    }

    out["files"] = arr;

    char writeBuf[256*256];

    auto toWrite = fopen(argv[1],"w");
    if (nullptr == toWrite) {
        printf("Io error... Could not write json.\n");
        return 1;
    }
    auto close = SCOPE_GUARD_LC(
        ::fclose(toWrite);
    );

    rj::FileWriteStream os(toWrite,writeBuf,sizeof(writeBuf));

    rj::Writer< rj::FileWriteStream > w(os);
    out.Accept(w);

    return 0;
}

