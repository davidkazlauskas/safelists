
#include <sqlite3.h>
#include <util/DumbHash.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Semaphore.hpp>

#include "SafeListDownloader.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

struct SingleDownloadJob : public Messageable {
    SingleDownloadJob(const StrongMsgPtr& session,int id) :
        _session(session), _id(id), _handler(genHandler())
    {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "Async disabled for SingleDownloadJob." );
    }

    void message(templatious::VirtualPack& msg) override {
        _handler->tryMatch(msg);
    }
private:
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< int >(
                [](int) {} // dummy
            )
        );
    }

    WeakMsgPtr _session;
    int _id;
    VmfPtr _handler;
};

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
        int64_t _size;
        std::string _link;
        std::string _path;
        std::shared_ptr< SingleDownloadJob > _downloadJob;
    };

    typedef std::vector< ToDownloadList > TDVec;

    static int downloadQueryCallback(void* userdata,int column,char** header,char** value) {
        TDVec& list = *reinterpret_cast< TDVec* >(userdata);
        SA::add(list,ToDownloadList());
        auto& back = list.back();
        back._id = std::atoi(value[0]);
        back._size = std::stoi(value[1]);
        back._link = std::atoi(value[2]);
        back._path = std::atoi(value[3]);
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
        _currentConnection = conn;
        auto sqliteGuard = makeScopeGuard(
            [&]() {
                sqlite3_close(conn);
                this->_currentConnection = nullptr;
                if (isFinished) {
                    std::remove(_path.c_str());
                }
            }
        );

        TDVec toDownload;

        const int KEEP_NUM = 5;

        char queryBuf[512];
        char* errMsg = nullptr;
        const char* FIRST_QUERY =
            "SELECT mirrors.id,file_size,link,file_path FROM mirrors"
            " LEFT OUTER JOIN to_download ON mirrors.id=to_download.id"
            " WHERE status=0"
            " ORDER BY priority DESC, use_count ASC"
            " LIMIT %d;";

        const char* UPDATE_STATUS_QUERY =
            "UPDATE to_download"
            "SET status=1"
            "WHERE id IN"
            "("
            "SELECT mirrors.id FROM mirrors"
            "LEFT OUTER JOIN to_download ON mirrors.id=to_download.id"
            "WHERE status=0"
            "ORDER BY priority DESC, use_count ASC"
            "LIMIT %d"
            ");";

        do {
            // proc messages

            auto currSize = SA::size(toDownload);
            int diff = KEEP_NUM - currSize;
            if (diff > 0) {
                sprintf(queryBuf,FIRST_QUERY,diff);
                res = sqlite3_exec(
                    conn,
                    queryBuf,
                    &downloadQueryCallback,
                    &toDownload,
                    &errMsg);

                if (res != 0) {
                    printf("Failed, pookie: %s\n",errMsg);
                }
                assert( res == 0 && "Should werk milky..." );

                sprintf(queryBuf,UPDATE_STATUS_QUERY,diff);
                res = sqlite3_exec(
                    conn,
                    queryBuf,
                    nullptr,
                    nullptr,
                    &errMsg);

                if (res != 0) {
                    printf("Failed, pookie: %s\n",errMsg);
                }
                assert( res == 0 && "Should werk milky..." );
            }

            TEMPLATIOUS_FOREACH(auto& i,toDownload) {
                if (nullptr == i._downloadJob) {
                    i._downloadJob =
                        std::make_shared< SingleDownloadJob >(
                            impl,
                            i._id
                        );
                }
            }

            _sem.wait();
        } while (SA::size(toDownload) > 0);

        isFinished = true;
    }

    std::string _path;
    StrongMsgPtr _fileWriter;
    StrongMsgPtr _fileDownloader;
    WeakMsgPtr _toNotify;
    sqlite3* _currentConnection;
    MessageCache _cache;
    StackOverflow::Semaphore _sem;
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

