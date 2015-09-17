
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
        const StrongMsgPtr& toNotify,
        bool notifyAsAsync
    ) :
        _path(path),
        _fileWriter(fileWriter),
        _fileDownloader(fileDownloader),
        _toNotify(toNotify),
        _handler(genHandler()),
        _sqliteRevision(0),
        _currentCacheRevision(-1),
        _jobCachePoint(0),
        _isFinished(false),
        _isAsync(notifyAsAsync),
        _count(0)
    {
        // session directory
        _sessionDir = path;
        // screw microsoft
        auto pos = _sessionDir.find_last_of('/');
        // with slash at the end
        _sessionDir.erase(pos + 1);
    }

    template <class... Types,class... Args>
    void notifyObserver(Args&&... args) {
        static_assert(sizeof...(Types) == sizeof...(Args),
            "Type mismatch cholo.");
        auto locked = _toNotify.lock();
        if (nullptr == locked) {
            return;
        }

        if (!_isAsync) {
            auto msg = SF::vpack<Types...>(
                std::forward<Args>(args)...);
            locked->message(msg);
        } else {
            auto msg = SF::vpackPtr<Types...>(
                std::forward<Args>(args)...);
            locked->message(msg);
        }
    }

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
        const StrongMsgPtr& toNotify,
        bool notifyAsAsync)
    {
        auto result = std::make_shared< SafeListDownloaderImpl >(
            path,
            fileWriter,
            fileDownloader,
            toNotify,
            notifyAsAsync);
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
        ToDownloadList(const std::shared_ptr< SafeListDownloaderImpl >& session) :
            _session(session), _handler(genHandler()),
            _list(SafeLists::Interval()),
            _hasStarted(false), _hasEnded(false),
            _progressDone(0),
            _progressReported(0)
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
                        //printf("Plucked twanger! %s\n",_path.c_str());
                        assert(
                            !this->_list.isDefined() || this->_list.isFilled() &&
                            "Whoa, cholo, lunch didn't finish yet?"
                        );
                        this->_hasEnded = true;
                        auto locked = _session.lock();
                        if (nullptr != locked) {
                            typedef SafeLists::RandomFileWriter RFW;
                            auto msg = SF::vpackPtr< FinishedDownload >(nullptr);
                            auto clearCache = SF::vpackPtr<
                                RFW::ClearCache,
                                std::string
                            >(nullptr,this->_absPath);

                            locked->message(msg);
                            locked->_fileWriter->message(clearCache);
                        }
                    }
                )
            );
        }

        std::weak_ptr< SafeListDownloaderImpl > _session;
        int _id;
        int64_t _size;
        std::string _link;
        std::string _path;
        std::string _absPath;
        VmfPtr _handler;
        SafeLists::IntervalList _list;
        bool _hasStarted;
        bool _hasEnded;
        int64_t _progressDone;
        int64_t _progressReported;
    };

    struct DownloadCacheItem {
        int _mirrorId;
        int64_t _size;
        std::string _url;
        std::string _path;
    };

    static int downloadQueryCallback(void* userdata,int column,char** value,char** header) {
        auto& self = *reinterpret_cast<
            std::shared_ptr<SafeListDownloaderImpl>* >(userdata);
        CacheVec& list = self->_jobCache;
        SA::add(list,DownloadCacheItem());
        auto& back = list.back();
        back._mirrorId = std::atoi(value[0]);
        back._size = std::atoi(value[1]);
        back._url = value[2];
        back._path = value[3];
        //printf("Starting plucing... %s\n",newList->_path.c_str());
        return 0;
    }

    void mainLoop(const std::shared_ptr< SafeListDownloaderImpl >& impl) {
        sqlite3* conn = nullptr;
        int res = sqlite3_open(_path.c_str(),&conn);
        if (0 != res) {
            assert( false &&
                "Todo, message to notify that session doesn't exist.");
        }

        _currentConnection = conn;
        auto sqliteGuard = SCOPE_GUARD_LC(
            this->cleanup();
        );

        std::vector< int > toMarkStarted;

        auto implCpy = impl;
        auto writerCpy = this->_fileWriter;
        updateJobs(implCpy,toMarkStarted);
        scheduleJobs(writerCpy);
        markStartedInDb(toMarkStarted);
        do {
            _sem.wait();

            SA::clear(toMarkStarted);

            updateJobs(implCpy,toMarkStarted);
            scheduleJobs(writerCpy);
            markStartedInDb(toMarkStarted);
            jobStatusUpdate();
            clearDoneJobs();
            processMessages();
        } while (SA::size(_jobs) > 0 || SA::size(_jobCache) > 0);

        assert( 0 == _count && "FAILER!" );
        _isFinished = true;
    }

    void jobStatusUpdate() {
        TEMPLATIOUS_FOREACH(auto& i,_jobs) {
            int64_t done = i->_progressDone;
            int64_t reported = i->_progressReported;
            if (done > reported) {
                int64_t size = i->_size;
                int id = i->_id;
                i->_progressReported = done;
                notifyObserver<
                    SafeListDownloader::OutProgressUpdate,
                    int, double, double
                >(nullptr,id,done,size);
            }
        }
    }

    void clearDoneJobs() {
        typedef SafeListDownloader SLD;

        SM::traverse(
            [=](std::shared_ptr<ToDownloadList>& dl) {
                bool didEnd = dl->_hasEnded;
                if (didEnd) {
                    --_count;
                    notifyObserver<
                        SLD::OutSingleDone, int
                    >(
                        nullptr, dl->_id
                    );
                    dl = nullptr;
                }
            },
            _jobs
        );

        SA::clear(
            SF::filter(
                _jobs,
                [](const std::shared_ptr<ToDownloadList>& dl) {
                    return nullptr == dl;
                }
            )
        );
    }

    void markStartedInDb(std::vector<int>& toMarkStarted) {
        // TODO: optimize for multiple ids... maybe?
        // Probably doesn't matter...
        const char* MARK_STARTED_STRING =
            "UPDATE to_download"
            " SET status=1"
            " WHERE id=%d;";
        char buf[512];
        int res = 0;
        char* errMsg = nullptr;
        sqlite3_exec(
            _currentConnection,"BEGIN;",
            nullptr,nullptr,&errMsg);
        TEMPLATIOUS_FOREACH(int i,toMarkStarted) {
            sprintf(buf,MARK_STARTED_STRING,i);
            res = sqlite3_exec(
                _currentConnection,
                buf,
                nullptr,
                nullptr,
                &errMsg);
            if (res != 0) {
                printf("Failed, pookie: %s\n",errMsg);
            }
            assert( res == 0 && "Should werk milky..." );
        }
        sqlite3_exec(
            _currentConnection,"COMMIT;",
            nullptr,nullptr,&errMsg);
    }

    void updateJobs(
        std::shared_ptr< SafeListDownloaderImpl >& impl,
        std::vector<int>& toMarkStarted)
    {
        refillCache(impl);
        scheduleFromCache(impl,toMarkStarted);
    }

    void refillCache(std::shared_ptr< SafeListDownloaderImpl >& impl) {
        int res = 0;
        const int CACHE_HIT_NUM = 128;

        char queryBuf[512];
        char* errMsg = nullptr;

        const char* FIRST_QUERY =
            "SELECT mirrors.id,file_size,link,file_path FROM mirrors"
            " LEFT OUTER JOIN to_download ON mirrors.id=to_download.id"
            " WHERE status=0"
            " ORDER BY priority DESC, use_count ASC"
            " LIMIT %d;";

        if (_currentCacheRevision < _sqliteRevision ||
            _jobCachePoint == SA::size(_jobCache))
        {
            SA::clear(_jobCache);
            sprintf(queryBuf,FIRST_QUERY,CACHE_HIT_NUM);
            res = sqlite3_exec(
                _currentConnection,
                queryBuf,
                &downloadQueryCallback,
                &impl,
                &errMsg);
            _currentCacheRevision = _sqliteRevision;
            _jobCachePoint = 0;
        }
    }

    void scheduleFromCache(
        std::shared_ptr< SafeListDownloaderImpl >& self,
        std::vector<int>& toDrop)
    {
        const int KEEP_NUM = 25;
        auto currSize = SA::size(_jobs);
        int diff = KEEP_NUM - currSize;
        while (diff > 0 && _jobCachePoint < SA::size(_jobCache)) {
            TDVec& list = this->_jobs;
            auto newList = std::make_shared< ToDownloadList >(self);
            SA::add(list,newList);

            auto& cacheItem = _jobCache[_jobCachePoint];
            ++_jobCachePoint;
            newList->_id = cacheItem._mirrorId;
            newList->_size = cacheItem._size;
            newList->_link = cacheItem._url;
            newList->_path = cacheItem._path;
            newList->_absPath = this->_sessionDir;
            newList->_absPath += newList->_path;

            SA::add(toDrop,cacheItem._mirrorId);

            currSize = SA::size(_jobs);
            diff = KEEP_NUM - currSize;
        }
    }

    void scheduleJobs(std::shared_ptr< Messageable >& writer) {
        TEMPLATIOUS_FOREACH(auto& i,_jobs) {
            if (!i->_hasStarted) {

                typedef std::function<
                    bool(const char*,int64_t,int64_t)
                > ByteFunction;

                auto intervals = listForPath(i->_path,i->_size);
                typedef AsyncDownloader AD;
                auto pathCopy = _sessionDir + i->_path;

                i->_list = intervals.clone();
                auto rawJob = i.get();
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
                        writer->message(message);
                        if (rawJob->_list.isDefined()) {
                            rawJob->_list.append(SafeLists::Interval(pre,post));
                        }
                        rawJob->_progressDone += size;
                        return true;
                    },
                    i
                );

                i->_hasStarted = true;
                int theId = i->_id;
                _fileDownloader->message(job);
                notifyObserver<
                    SafeListDownloader::OutStarted,
                    int, std::string
                >(nullptr,theId,i->_path);
                ++_count;
            }
        }
    }

    int processMessages() {
        return _cache.process(
            [&](templatious::VirtualPack& pack) {
                this->_handler->tryMatch(pack);
            }
        );
    }

    void cleanup() {
        auto closeCpy = _currentConnection;
        this->_currentConnection = nullptr;

        sqlite3_close(closeCpy);
        if (_isFinished) {
            notifyObserver<SafeListDownloader::OutDone>(nullptr);
            std::remove(_path.c_str());
        }
    }

    typedef std::vector< std::shared_ptr<ToDownloadList> > TDVec;
    typedef std::vector< DownloadCacheItem > CacheVec;

    std::string _path;
    std::string _sessionDir;
    StrongMsgPtr _fileWriter;
    StrongMsgPtr _fileDownloader;
    WeakMsgPtr _toNotify;
    sqlite3* _currentConnection;
    MessageCache _cache;
    StackOverflow::Semaphore _sem;
    TDVec _jobs;
    CacheVec _jobCache;
    VmfPtr _handler;
    int _sqliteRevision;
    int _currentCacheRevision;
    int _jobCachePoint;
    bool _isFinished;
    bool _isAsync;
    int _count;
};

StrongMsgPtr SafeListDownloader::startNew(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify,
        bool notifyAsAsync
)
{
    return SafeListDownloaderImpl::startSession(
        path,
        fileWriter,
        fileDownloader,
        toNotify,
        notifyAsAsync
    );
}

}

