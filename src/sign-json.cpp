
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
    if (argc != 3) {
        printf("Usage: signjson <private key path> <signature file>\n");
        return 1;
    }

    const char* privKey = argv[1];
    const char* jsonSig = argv[2];

    std::vector< std::string > paths;
    std::string outSig;
    auto res = SafeLists::getFileListWSignature(jsonSig,paths,outSig);
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

    fs::path sigPath(jsonSig);
    auto parentDir = fs::absolute(sigPath.parent_path());
    std::string absParent = parentDir.string();
    absParent += "/";

    auto signRes = SafeLists::signFileList(privKey,absParent,paths,outSig);
    if (signRes != SafeLists::SignFileListError::Success) {
        if (signRes == SafeLists::SignFileListError::CouldNotOpenKey) {
            printf("Could not open key...\n");
        } else if (signRes == SafeLists::SignFileListError::KeyReadFail) {
            printf("Invalid key format (PEM expected)...\n");
        } else if (signRes == SafeLists::SignFileListError::HashingFailed) {
            printf("Hashing failed...\n");
        } else if (signRes == SafeLists::SignFileListError::SigningFailed) {
            printf("Signing failed...\n");
        }

        return 1;
    }

    assert( outSig != "" && "Huh?!?" );

    rj::Document out;
    out.SetObject();
    auto& alloc = out.GetAllocator();

    rj::Value sig(rj::kStringType);
    sig.SetString(outSig.c_str(),outSig.size(),alloc);

    out.AddMember("signature",sig,alloc);

    rj::Value arr(rj::kArrayType);
    TEMPLATIOUS_FOREACH(auto& i,paths) {
        rj::Value iPath(rj::kStringType);
        iPath.SetString(i.c_str(),i.size(),alloc);
        arr.PushBack(iPath,alloc);
    }

    out.AddMember("files",arr,alloc);

    char writeBuf[256*256];

    auto toWrite = fopen(jsonSig,"w");
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

