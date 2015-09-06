
#include <fstream>

#include <sqlite3.h>
#include <util/DumbHash.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Semaphore.hpp>
#include <io/Interval.hpp>
#include <io/AsyncDownloader.hpp>
#include <io/RandomFileWriter.hpp>

#include "SafeListDownloader.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    SafeLists::IntervalList listForPath(
        const std::string& path,
        int64_t size
    )
    {
        std::string listPath = path + ".ilist";
        std::ifstream target(listPath.c_str());
        if (!target.is_open() && size > 0) {
            return SafeLists::IntervalList(
                SafeLists::Interval(0,size)
            );
        }

        return SafeLists::IntervalList(
            SafeLists::Interval()
        );
    }
}

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
        _toNotify(toNotify),
        _handler(genHandler())
    {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _cache.enqueue(msg);
        _sem.notify();
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
    DUMMY_STRUCT_NATIVE(FinishedDownload);

    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< FinishedDownload >(
                [&](FinishedDownload) {
                    // mainly to invoke semaphore, do nothing
                }
            )
        );
    }

    struct ToDownloadList : public Messageable {
        ToDownloadList(const StrongMsgPtr& session) :
            _session(session), _handler(genHandler()),
            _hasStarted(false), _hasEnded(false)
        {}

        void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
            assert( false && "Async disabled for ToDownloadList." );
        }

        void message(templatious::VirtualPack& msg) override {
            _handler->tryMatch(msg);
        }

        typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

        VmfPtr genHandler() {
            typedef SafeLists::AsyncDownloader AD;
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch< AD::OutDownloadFinished >(
                    [&](AD::OutDownloadFinished) {
                        this->_hasEnded = true;
                        auto locked = _session.lock();
                        if (nullptr != locked) {
                            auto msg = SF::vpackPtr< FinishedDownload >(nullptr);
                            locked->message(msg);
                        }
                    }
                )
            );
        }

        WeakMsgPtr _session;
        int _id;
        int64_t _size;
        std::string _link;
        std::string _path;
        VmfPtr _handler;
        bool _hasStarted;
        bool _hasEnded;
    };

    static int downloadQueryCallback(void* userdata,int column,char** value,char** header) {
        auto& self = *reinterpret_cast<
            std::shared_ptr<SafeListDownloaderImpl>* >(userdata);
        TDVec& list = self->_jobs;
        auto newList = std::make_shared< ToDownloadList >(self);
        SA::add(list,newList);
        auto& back = list.back();
        newList->_id = std::atoi(value[0]);
        newList->_size = std::stoi(value[1]);
        newList->_link = std::atoi(value[2]);
        newList->_path = std::atoi(value[3]);
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
                auto locked = this->_toNotify.lock();
                if (nullptr == locked) {
                    auto msg = SF::vpack<
                        SafeListDownloader::OutDone >(nullptr);
                    locked->message(msg);
                }
                sqlite3_close(conn);
                this->_currentConnection = nullptr;
                if (isFinished) {
                    std::remove(_path.c_str());
                }
            }
        );

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
            " SET status=1"
            " WHERE id IN"
            " ("
            " SELECT mirrors.id FROM mirrors"
            " LEFT OUTER JOIN to_download ON mirrors.id=to_download.id"
            " WHERE status=0"
            " ORDER BY priority DESC, use_count ASC"
            " LIMIT %d"
            " );";

        auto implCpy = impl;
        auto writerCpy = this->_fileWriter;
        do {
            _cache.process(
                [&](templatious::VirtualPack& pack) {
                    this->_handler->tryMatch(pack);
                }
            );

            updateRemaining();

            auto currSize = SA::size(_jobs);
            int diff = KEEP_NUM - currSize;
            if (diff > 0) {
                sprintf(queryBuf,FIRST_QUERY,diff);
                res = sqlite3_exec(
                    conn,
                    queryBuf,
                    &downloadQueryCallback,
                    &implCpy,
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

            TEMPLATIOUS_FOREACH(auto& i,_jobs) {
                if (!i->_hasStarted) {

                    typedef std::function<
                        bool(const char*,int64_t,int64_t)
                    > ByteFunction;

                    auto intervals = listForPath(i->_path,i->_size);
                    typedef AsyncDownloader AD;
                    auto pathCopy = i->_path;

                    auto job = SF::vpackPtr<
                        AD::ScheduleDownload,
                        IntervalList,
                        ByteFunction,
                        std::weak_ptr< Messageable >
                    >(
                        nullptr,
                        std::move(intervals),
                        [=](const char* buf,int64_t pre,int64_t post) {
                            typedef SafeLists::RandomFileWriter RFW;
                            int64_t size = post - pre;
                            std::unique_ptr< char[] > outBuf(
                                new char[size]
                            );

                            memcpy(outBuf.get(),buf,size);
                            auto message = SF::vpackPtr<
                                RFW::WriteData,
                                std::string,
                                std::unique_ptr< char[] >,
                                int64_t,int64_t
                            >(nullptr,pathCopy,std::move(outBuf),pre,post);
                            writerCpy->message(message);
                            return true;
                        },
                        i
                    );

                    i->_hasStarted = true;
                    // TODO: send the message,
                    // do something with buffer
                }
            }

            _sem.wait();
        } while (SA::size(_jobs) > 0);

        isFinished = true;
    }

    void updateRemaining() {
        SA::clear(
            SF::filter(
                _jobs,
                [](const std::shared_ptr<ToDownloadList>& dl) {
                    return dl->_hasEnded;
                }
            )
        );
    }

    typedef std::vector< std::shared_ptr<ToDownloadList> > TDVec;

    std::string _path;
    StrongMsgPtr _fileWriter;
    StrongMsgPtr _fileDownloader;
    WeakMsgPtr _toNotify;
    sqlite3* _currentConnection;
    MessageCache _cache;
    StackOverflow::Semaphore _sem;
    TDVec _jobs;
    VmfPtr _handler;
};

StrongMsgPtr SafeListDownloader::startNew(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify
)
{
    return SafeListDownloaderImpl::startSession(
        path,
        fileWriter,
        fileDownloader,
        toNotify
    );
}

}

