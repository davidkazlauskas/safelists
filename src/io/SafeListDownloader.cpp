
#include <fstream>

#include <sqlite3.h>
#include <util/DumbHash.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Semaphore.hpp>
#include <util/GenericShutdownGuard.hpp>
#include <io/Interval.hpp>
#include <io/AsyncDownloader.hpp>
#include <io/RandomFileWriter.hpp>

#include <boost/filesystem.hpp>

#include "SafeListDownloader.hpp"
#include "SafeListDownloaderInternal.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {
    SafeLists::IntervalList readIListAndHash(
        const char* path,SafeLists::DumbHash256& hash)
    {
        std::ifstream target(path,std::ios::binary);
        if (target.is_open()) {
            char outHash[32];
            static_assert( sizeof(outHash) == 32,
                "Why do I even put these?" );
            target.read(outHash,sizeof(outHash));
            hash.setBytes(outHash);
            return SafeLists::readIntervalList(
                target
            );
        } else {
            assert( false && "Whoa, black magic [0], didn't expect that." );
        }
    }
}

namespace {
    namespace fs = boost::filesystem;

    void cleanPathLeftovers(const std::string& path) {
        std::string listPath = path + ".ilist";
        std::string tmpPath = path + ".ilist.tmp";
        fs::remove(listPath);
        fs::remove(tmpPath);
    }

    SafeLists::IntervalList listForPath(
        const std::string& path,
        int64_t size,
        SafeLists::DumbHash256& hash
    )
    {
        // frontend should prevent adding files
        // with these names (.ilist, .ilist.tmp)
        std::string listPath = path + ".ilist";
        std::string tmpPath = path + ".ilist.tmp";
        bool tmpExists = fs::exists(tmpPath.c_str());
        bool normalExists = fs::exists(listPath.c_str());

        // no ilist exists yet, assume new
        if (!normalExists && !tmpExists && size > 0) {
            SafeLists::DumbHash256 def;
            hash = def;
            return SafeLists::IntervalList(
                SafeLists::Interval(0,size)
            );
        }
        // other common case, atomic write succeded
        else if (normalExists && !tmpExists) {
            return readIListAndHash(listPath.c_str(),hash);
        }
        // atomic write of intervals attempted but
        // didn't finish, read from the uncorrupted
        // but remove temporary
        else if (normalExists && tmpExists) {
            fs::remove(tmpPath.c_str());
            return readIListAndHash(listPath.c_str(),hash);
        }
        // temporary write succeded and normal
        // removed, meaning, tmp finished successfully,
        // finish moving and read from moved.
        else if (!normalExists && tmpExists) {
            fs::rename(tmpPath.c_str(),listPath.c_str());
            return readIListAndHash(listPath.c_str(),hash);
        }

        SafeLists::DumbHash256 def;
        hash = def;
        // return an empty interval, we have no
        // idea about file sizes.
        return SafeLists::IntervalList(
            SafeLists::Interval()
        );
    }

    void writeIntervalListAtomic(
        const std::string& path,
        const SafeLists::IntervalList& theList,
        const SafeLists::DumbHash256& hash)
    {
        std::string tmpPath = path + ".ilist.tmp";
        std::string ilistPath = path + ".ilist";
        { // open write and close file
            char hashString[32];
            static_assert( sizeof(hashString) == 32,
                "YO SLICK! Biting off here!" );
            hash.toBytes(hashString);
            std::ofstream os(tmpPath.c_str(),std::ios::binary);
            os.write(hashString,sizeof(hashString));
            SafeLists::writeIntervalList(theList,os);
        }

        // only one normal is removed we can be
        // sured tmpPath was finished writing
        fs::remove(ilistPath.c_str());
        // move tmpPath to a normal path to be
        // read in case of power outage.
        fs::rename(tmpPath,ilistPath);
    }
}

typedef std::lock_guard< std::mutex > LGuard;

namespace SafeLists {

struct SafeListDownloaderImpl : public Messageable {
    friend struct SafeListDownloader;

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
        _keepGoing(true),
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

    ~SafeListDownloaderImpl() {
        //printf("DOWNLOAD BITES THE DUST\n");
    }

    bool nextUpdate(std::chrono::high_resolution_clock::time_point& point) {
        auto now = std::chrono::high_resolution_clock::now();
        int64_t millis = std::chrono::duration_cast<
            std::chrono::milliseconds
        >(now - point).count();
        const int UPATE_PERIODICITY_MS = 7000; // 7 seconds by default
        if (millis > UPATE_PERIODICITY_MS) {
            point += std::chrono::milliseconds(UPATE_PERIODICITY_MS);
            return true;
        }
        return false;
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
    DUMMY_STRUCT_NATIVE(FileNotFound);

    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< FinishedDownload >(
                [&](FinishedDownload) {
                    // mainly to invoke semaphore, do nothing
                }
            ),
            SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                [=](GSI::InRegisterItself, const StrongMsgPtr& ptr) {
                    auto handler = std::make_shared< GenericShutdownGuard >();
                    handler->setMaster(_myself);
                    auto msg = SF::vpackPtr< GSI::OutRegisterItself, StrongMsgPtr >(
                        nullptr, handler
                    );
                    assert( nullptr == _toNotifyShutdown.lock() );
                    _toNotifyShutdown = handler;
                    ptr->message(msg);
                }
            ),
            SF::virtualMatch< GenericShutdownGuard::ShutdownTarget >(
                [=](GenericShutdownGuard::ShutdownTarget) {
                    _keepGoing = false;
                }
            )
        );
    }

    struct ToDownloadList : public Messageable {
        ToDownloadList(const std::shared_ptr< SafeListDownloaderImpl >& session) :
            _session(session), _handler(genHandler()),
            _list(SafeLists::Interval()),
            _hasStarted(false), _hasEnded(false),
            _isResumed(false),
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
                ),
                SF::virtualMatch< AD::OutFileNotFound >(
                    [&](AD::OutFileNotFound) {
                        this->_hasEnded = true;
                        auto locked = _session.lock();
                        if (nullptr != locked) {
                            auto msg = SF::vpackPtr< FileNotFound >(nullptr);
                            locked->message(msg);
                        }
                    }
                )
            );
        }

        std::weak_ptr< SafeListDownloaderImpl > _session;
        int _id;
        int64_t _size;
        std::string _link;
        std::string _dumbHash256;
        std::string _path;
        std::string _absPath;
        SafeLists::DumbHash256 _hasher;
        SafeLists::DumbHash256 _hasherBackup;
        VmfPtr _handler;
        SafeLists::IntervalList _list;
        std::mutex _listMutex;
        bool _hasStarted;
        bool _hasEnded;
        bool _isResumed;
        int64_t _progressDone;
        int64_t _progressReported;
    };

    struct DownloadCacheItem {
        int _mirrorId;
        int64_t _size;
        std::string _url;
        std::string _path;
        std::string _dumbHash256;
        bool _isResumed;
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
        back._dumbHash256 = value[4] != nullptr ? value[4] : "";
        back._isResumed = false;
        //printf("Starting plucing... %s\n",newList->_path.c_str());
        return 0;
    }

    static int getTotalCallback(void* userdata,int column,char** values,char** headers) {
        int& ref = *reinterpret_cast<int*>(userdata);
        ref = std::atoi(values[0]);
        return 0;
    }

    void notifyTotalDownloads() {
        int out;
        char* errMsg = nullptr;
        sqlite3_exec(_currentConnection,
            "SELECT COUNT(*) FROM to_download WHERE status<2;",
            &getTotalCallback,
            &out,&errMsg);
        notifyObserver<
            SafeListDownloader::OutTotalDownloads,
            int
        >(nullptr,out);
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

        notifyTotalDownloads();

        std::vector< int > toMarkStarted;

        _lastUpdate = std::chrono::high_resolution_clock::now();

        auto intervalListUpdate = std::chrono::high_resolution_clock::now();

        auto implCpy = impl;
        auto writerCpy = this->_fileWriter;
        bool resumeSuccess =
            updateJobsToResumeDownloads(implCpy,toMarkStarted);
        if (!resumeSuccess) {
            updateJobs(implCpy,toMarkStarted);
        }
        scheduleJobs(writerCpy);
        markStartedInDb(toMarkStarted);
        do {
            if (SA::size(_jobs) > 0) {
                _sem.wait();
            }

            SA::clear(toMarkStarted);

            updateJobs(implCpy,toMarkStarted);
            scheduleJobs(writerCpy);
            markStartedInDb(toMarkStarted);
            jobStatusUpdate();
            clearDoneJobs();
            processMessages();

            bool shouldWriteIntervals = nextUpdate(intervalListUpdate);
            if (shouldWriteIntervals) {
                updateIntervalsForJobs();
            }

            if (!_keepGoing) {
                return;
            }
        } while (SA::size(_jobs) > 0 || SA::size(_jobCache) > 0);

        assert( 0 == _count && "FAILER!" );
        _isFinished = true;
    }

    void updateIntervalsForJobs() {
        TEMPLATIOUS_FOREACH(auto& i,_jobs) {
            if (!i->_hasEnded) {
                std::unique_lock< std::mutex > ul(i->_listMutex);
                auto clone = i->_list.clone();
                auto hashCopy = i->_hasherBackup;
                ul.unlock();

                if (!clone.isFilled()) {
                    writeIntervalListAtomic(i->_absPath,clone,hashCopy);
                }
            }
        }
    }

    void jobStatusUpdate() {
        TEMPLATIOUS_FOREACH(auto& i,_jobs) {
            int64_t done = i->_progressDone;
            int64_t reported = i->_progressReported;
            int64_t diff = done - reported;
            if (diff > 0) {
                int64_t size = i->_size;
                int id = i->_id;
                i->_progressReported = done;
                notifyObserver<
                    SafeListDownloader::OutProgressUpdate,
                    int, double, double, double
                >(nullptr,id,done,size,diff);
            }
        }
    }

    void clearDoneJobs() {
        typedef SafeListDownloader SLD;

        { // scope guard to close transaction
        sqlite3_exec(
            _currentConnection,
            "BEGIN;",
            nullptr,
            nullptr,
            nullptr);

        auto endTransaction = SCOPE_GUARD_LC(
            sqlite3_exec(
                _currentConnection,
                "COMMIT;",
                nullptr,
                nullptr,
                nullptr);
        );

        SM::traverse(
            [=](std::shared_ptr<ToDownloadList>& dl) {
                bool didEnd = dl->_hasEnded;
                if (didEnd) {
                    auto guard = SCOPE_GUARD_LC(
                        char buf[256];
                        sprintf(
                            buf,
                            "UPDATE to_download"
                            " SET status=2 WHERE id=%d;",
                            dl->_id
                        );

                        char* errMsg = nullptr;
                        int res = sqlite3_exec(
                            _currentConnection,buf,
                            nullptr,nullptr,&errMsg);

                        assert( res == 0 && errMsg == nullptr &&
                            "Oh, cmon!" );

                        dl = nullptr;
                    );

                    auto leftOverGuard = SCOPE_GUARD_LC(
                        cleanPathLeftovers(dl->_absPath);
                    );

                    --_count;
                    notifyObserver<
                        SLD::OutSingleDone, int
                    >(
                        nullptr, dl->_id
                    );

                    notifyObserver<
                        SLD::OutMirrorUsed, int,
                        std::string
                    >(
                        nullptr, dl->_id, dl->_link
                    );

                    char buf[128];
                    dl->_hasher.toString(buf);
                    if (dl->_dumbHash256 != buf) {
                        notifyObserver<
                            SLD::OutHashUpdate,
                            int,
                            std::string
                        >(nullptr,dl->_id,buf);
                    }

                    int64_t sizeDone = dl->_progressDone;
                    int64_t sizePrelim = dl->_size;
                    if (sizePrelim == -1) {
                        notifyObserver<
                            SLD::OutSizeUpdate,
                            int,
                            double
                        >(nullptr,dl->_id,sizeDone);
                    } else {
                        assert( sizeDone == sizePrelim &&
                            "Unhandled case, what if reported"
                            " size doesn't match actual?");
                    }
                }
            },
            _jobs
        );

        // sqlite end transaction
        }

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
        const char* REFILL_QUERY =
            "SELECT mirrors.id,file_size,link,file_path,file_hash_256 FROM mirrors"
            " LEFT OUTER JOIN to_download ON mirrors.id=to_download.id"
            " WHERE status=0"
            " ORDER BY priority DESC, use_count ASC"
            " LIMIT %d;";

        refillCache(impl,REFILL_QUERY);
        scheduleFromCache(impl,toMarkStarted);
    }

    bool updateJobsToResumeDownloads(
        std::shared_ptr< SafeListDownloaderImpl >& impl,
        std::vector<int>& toMarkStarted
    )
    {
        const char* REFILL_QUERY_RESUME =
            "SELECT mirrors.id,file_size,link,file_path,file_hash_256 FROM mirrors"
            " LEFT OUTER JOIN to_download ON mirrors.id=to_download.id"
            " WHERE status=1"
            " ORDER BY priority DESC, use_count ASC"
            " LIMIT %d;";

        assert( SA::size(_jobCache) == 0 && "WRONG" );

        refillCache(impl,REFILL_QUERY_RESUME);
        if (SA::size(_jobCache) > 0) {
            TEMPLATIOUS_FOREACH(auto& i,_jobCache) {
                i._isResumed = true;
            }
            scheduleFromCache(impl,toMarkStarted);
            return true;
        }

        return false;
    }

    void refillCache(std::shared_ptr< SafeListDownloaderImpl >& impl,
            const char* query)
    {
        int res = 0;
        const int CACHE_HIT_NUM = 128;

        char queryBuf[512];
        char* errMsg = nullptr;

        if (_currentCacheRevision < _sqliteRevision ||
            _jobCachePoint == SA::size(_jobCache))
        {
            SA::clear(_jobCache);
            sprintf(queryBuf,query,CACHE_HIT_NUM);
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
            newList->_dumbHash256 = cacheItem._dumbHash256;
            newList->_absPath = this->_sessionDir;
            newList->_absPath += newList->_path;
            newList->_isResumed = cacheItem._isResumed;

            if (!cacheItem._isResumed) {
                SA::add(toDrop,cacheItem._mirrorId);
            }

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

                SafeLists::DumbHash256 dh;
                auto intervals = listForPath(i->_absPath,i->_size,dh);
                i->_hasher = dh;
                if (!intervals.isEmpty()) {
                    int64_t downloadedCount = 0;
                    intervals.traverseFilled(
                        [&](const Interval& i) {
                            downloadedCount += i.size();
                            return true;
                        }
                    );
                    i->_progressDone = downloadedCount;
                }
                typedef AsyncDownloader AD;
                auto pathCopy = _sessionDir + i->_path;

                const int UPDATE_MILLISECONDS = 100;
                i->_list = intervals.clone();
                auto rawJob = i.get();
                auto job = SF::vpackPtr<
                    AD::ScheduleDownload,
                    std::string,
                    IntervalList,
                    ByteFunction,
                    std::weak_ptr< Messageable >
                >(
                    nullptr,
                    pathCopy,
                    std::move(intervals),
                    [=](const char* buf,int64_t pre,int64_t post) {
                        typedef SafeLists::RandomFileWriter RFW;
                        int64_t size = post - pre;
                        std::unique_ptr< char[] > outBuf(
                            new char[size]
                        );

                        auto rawBuf = outBuf.get();
                        memcpy(rawBuf,buf,size);
                        auto message = SF::vpackPtr<
                            RFW::WriteData,
                            std::string,
                            std::unique_ptr< char[] >,
                            int64_t,int64_t
                        >(nullptr,pathCopy,std::move(outBuf),pre,post);

                        // HASH IT FIRST
                        auto rng = SF::seqL<int64_t>(pre,post);
                        SM::traverse< true >(
                            [&](int which,int64_t curr) {
                                rawJob->_hasher.add(curr,rawBuf[which]);
                            },
                            rng
                        );

                        writer->message(message);
                        { // _list update scope
                            LGuard g(rawJob->_listMutex);
                            if (rawJob->_list.isDefined()) {
                                rawJob->_list.append(SafeLists::Interval(pre,post));
                            }
                            rawJob->_hasherBackup = rawJob->_hasher;
                        }
                        rawJob->_progressDone += size;
                        auto lu = this->_lastUpdate;
                        auto now = std::chrono::high_resolution_clock::now();
                        if (now > lu) {
                            lu += std::chrono::milliseconds(UPDATE_MILLISECONDS);
                            this->_lastUpdate = lu;
                            this->_sem.notify();
                        }
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

        if (!_keepGoing) {
            auto locked = _toNotifyShutdown.lock();
            if (nullptr != locked) {
                auto msg = SF::vpack<
                    GenericShutdownGuard::SetFuture >(nullptr);
                locked->message(msg);
            }
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
    bool _keepGoing;
    bool _isAsync;
    int _count;
    std::chrono::high_resolution_clock::time_point _lastUpdate;

    WeakMsgPtr _myself;
    WeakMsgPtr _toNotifyShutdown;
};

StrongMsgPtr SafeListDownloader::startNew(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify,
        bool notifyAsAsync
)
{
    auto res = SafeListDownloaderImpl::startSession(
        path,
        fileWriter,
        fileDownloader,
        toNotify,
        notifyAsAsync
    );
    res->_myself = res;
    return res;
}

}

