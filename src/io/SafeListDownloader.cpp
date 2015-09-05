
#include <sqlite3.h>
#include <util/DumbHash.hpp>
#include <util/ScopeGuard.hpp>

#include "SafeListDownloader.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

struct SafeListDownloaderImpl : public Messageable {
    SafeListDownloaderImpl() = delete;
    SafeListDownloaderImpl(const SafeListDownloaderImpl&) = delete;
    SafeListDownloaderImpl(SafeListDownloaderImpl&&) = delete;
    SafeListDownloaderImpl(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify
    ) :
        _path(path),
        _fileWriter(fileWriter),
        _fileDownloader(fileDownloader),
        _toNotify(toNotify)
    {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _cache.enqueue(msg);
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "Synchronous messages disabled on SafeListDownloader." );
    }

    static std::shared_ptr< SafeListDownloaderImpl > startSession(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify)
    {
        auto result = std::make_shared< SafeListDownloaderImpl >(
            path,
            fileWriter,
            fileDownloader,
            toNotify);
        std::thread(
            [=]() {
                result->mainLoop(result);
            }
        ).detach();
        return result;
    }
private:
    struct ToDownloadList {
        int _id;
        std::string _link;
        std::string _path;
    };

    typedef std::vector< ToDownloadList > TDVec;

    static int downloadQueryCallback(void* userdata,int column,char** header,char** value) {
        TDVec& list = *reinterpret_cast< TDVec* >(userdata);
        SA::add(list,ToDownloadList());
        list.back()._id = std::atoi(value[0]);
        list.back()._link = std::atoi(value[1]);
        list.back()._path = std::atoi(value[2]);
        return 0;
    }

    void mainLoop(const std::shared_ptr< SafeListDownloaderImpl >& impl) {
        sqlite3* conn = nullptr;
        int res = sqlite3_open(_path.c_str(),&conn);
        if (0 != res) {
            assert( false &&
                "Todo, message to notify that session doesn't exist.");
        }

        bool isFinished = false;

        auto sqliteGuard = makeScopeGuard(
            [&]() {
                sqlite3_close(conn);
                if (isFinished) {
                    std::remove(_path.c_str());
                }
            }
        );

        TDVec toDownload;
        //int res = sqlite3_exec()
    }

    std::string _path;
    StrongMsgPtr _fileWriter;
    StrongMsgPtr _fileDownloader;
    WeakMsgPtr _toNotify;
    MessageCache _cache;
};

StrongMsgPtr SafeListDownloader::startNew(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify
)
{
    return nullptr;
}

}

