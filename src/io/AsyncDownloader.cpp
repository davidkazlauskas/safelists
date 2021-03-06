
#include <cstring>
#include <future>

#include <templatious/FullPack.hpp>
#include <io/Interval.hpp>
#include <util/GenericShutdownGuard.hpp>
#include <util/ScopeGuard.hpp>
#include <safe_file_downloader.h>

#include "AsyncDownloader.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    size_t dummySize() {
        return 256 * 256;
    }

    std::unique_ptr< const char[] > prepDummyBuffer() {
        std::unique_ptr< const char[] > result;
        auto dummySz = dummySize();
        auto original = new char[dummySz];
        result.reset( original );
        TEMPLATIOUS_0_TO_N( i, dummySz ) {
            original[i] = '7';
        }
        return result;
    }
}

namespace SafeLists {

    typedef std::unique_ptr< templatious::VirtualMatchFunctor > Handler;
    typedef AsyncDownloader AD;
    typedef std::function< bool(const char*,int64_t,int64_t) > ByteFunction;
    typedef std::weak_ptr< Messageable > WeakMsgPtr;


    struct AsyncDownloaderSafeNet : public Messageable {
        AsyncDownloaderSafeNet() :
            _handler(genHandler()),
            _safehandle(nullptr),
            _shutdown(false)
        {}

        ~AsyncDownloaderSafeNet() {
            shutdown();
        }

        // this is for sending message across threads
        void message(const std::shared_ptr< templatious::VirtualPack >& msg) {
            _handler->tryMatch(*msg);
        }

        // this is for sending stack allocated (faster)
        // if we know we're on the same thread as GUI
        void message(templatious::VirtualPack& msg) {
            assert( false && "Synchronous messages disabled for this object." );
        }

        static StrongMsgPtr spinupNew() {
            auto result = std::make_shared< AsyncDownloaderSafeNet >();
            result->_myself = result;

            result->_safehandle = ::safe_file_downloader_new();

            return result;
        }

        void messageLoop(const std::shared_ptr< AsyncDownloaderSafeNet >& myself) {
            while (!_shutdown) {
                if (myself.unique()) {
                    shutdown();
                    break;
                }

                // do stuff
            }

            auto locked = _notifyExit.lock();
            if (nullptr != locked) {
                auto msg = SF::vpack<
                    GenericShutdownGuard::SetFuture >(nullptr);
                locked->message(msg);
            }
        }

        struct ScheduleDownloadCell {
            ScheduleDownloadCell(
                const std::string& path,
                IntervalList&& interval,
                const ByteFunction& func,
                const std::weak_ptr< Messageable >& wmsg,
                int64_t chunkSizes = 1024 * 64)
                :
                    _path(path),
                    _counter(std::move(interval)),
                    _func(func),
                    _wmsg(wmsg),
                    _maxChunkSize(chunkSizes)
            {}

            std::string _path;
            IntervalList _counter;
            ByteFunction _func;
            std::weak_ptr< Messageable > _wmsg;
            int64_t _maxChunkSize;

            template <class... Sig,class... Args>
            bool notify(Args&&... args) {
                auto tryLock = this->_wmsg.lock();
                if (nullptr == tryLock) {
                    return false;
                }
                auto msg = SF::vpack< Sig... >(
                    std::forward<Args>(args)...);
                tryLock->message(msg);
            }

            static int32_t bufferfunc(
                void* userdata,
                int64_t start,
                int64_t end,
                const uint8_t* buf)
            {
                ScheduleDownloadCell& cell =
                    *reinterpret_cast< ScheduleDownloadCell* >(userdata);

                const char* conv = reinterpret_cast< const char* >(buf);

                cell._counter.append(Interval(start,end));
                bool res = cell._func(conv,start,end);

                return res ? 0 : 1;
            }

            static int32_t nextIntervalFunc(
                void* userdata,
                int64_t* start,
                int64_t* end)
            {
                ScheduleDownloadCell& cell =
                    *reinterpret_cast< ScheduleDownloadCell* >(userdata);

                auto& theList = cell._counter;
                Interval next;
                theList.traverseEmpty(
                    [&](const Interval& i) {
                        next = i;
                        return false;
                    }
                );

                *start = next.start();

                if (next.size() > cell._maxChunkSize) {
                    *end = next.start() + cell._maxChunkSize;
                } else {
                    *end = next.end();
                }

                return 0;
            }

            static void arbitraryMessageFunc(
                void* userdata,
                int32_t type,
                void* optinfo)
            {
                ScheduleDownloadCell& cell =
                    *reinterpret_cast< ScheduleDownloadCell* >(userdata);

                switch (type) {
                    case SAFE_DOWNLOADER_MSG_DOWNLOAD_SUCCESS:
                        cell.notify<AD::OutDownloadFinished>(nullptr);
                        break;
                    case SAFE_DOWNLOADER_MSG_FILE_NOT_FOUND:
                        cell.notify<AD::OutFileNotFound>(nullptr);
                        break;
                    case SAFE_DOWNLOADER_MSG_IS_DONE:
                        {
                        int32_t* theAns = reinterpret_cast< int32_t* >(optinfo);
                        *theAns = cell._counter.isFilled() ? 1 : 0;
                        }
                        break;
                    case SAFE_DOWNLOADER_MSG_FILE_SIZE_FOUND:
                        {
                        int64_t theSize = *reinterpret_cast< const int64_t* >(optinfo);
                        cell.notify<AD::OutSizeKnown,int64_t>(nullptr,theSize);
                        if (cell._counter.range().size() != theSize) {
                            cell._counter = IntervalList(Interval(0,theSize));
                        }
                        }
                        break;
                }
            }

            static void destructor(void* userdata) {
                ScheduleDownloadCell* cell =
                    reinterpret_cast< ScheduleDownloadCell* >(userdata);
                delete cell;
            }
        };

        void scheduleDownload(
            const std::string& path,
            IntervalList&& interval,
            const ByteFunction& func,
            const std::weak_ptr< Messageable >& wmsg)
        {
            std::unique_ptr< ScheduleDownloadCell > p(
                new ScheduleDownloadCell(
                    path,std::move(interval),func,wmsg)
            );

            ::safe_file_downloader_args args;
            args.userdata = p.get();
            args.userdata_buffer_func =
                &ScheduleDownloadCell::bufferfunc;
            args.userdata_next_range_func =
                &ScheduleDownloadCell::nextIntervalFunc;
            args.userdata_arbitrary_message_func =
                &ScheduleDownloadCell::arbitraryMessageFunc;
            args.userdata_destructor =
                &ScheduleDownloadCell::destructor;
            args.path = p.get()->_path.c_str();

            assert( nullptr != _safehandle && "No handle, cholo..." );

            ::safe_file_downloader_schedule(_safehandle,&args);

            p.release();
        }

        void shutdown() {
            _shutdown = true;
            if (nullptr != _safehandle) {
                ::safe_file_downloader_cleanup(
                    _safehandle,nullptr,nullptr);
                _safehandle = nullptr;
            }
        }

        Handler genHandler() {
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch<
                    AD::ScheduleDownload,
                    const std::string,
                    IntervalList,
                    ByteFunction,
                    std::weak_ptr< Messageable >
                >(
                    [=](AD::ScheduleDownload,
                        const std::string& url,
                        IntervalList& interval,
                        const ByteFunction& func,
                        const std::weak_ptr< Messageable >& wmsg)
                    {
                        scheduleDownload(url,std::move(interval),func,wmsg);
                    }
                ),
                SF::virtualMatch<
                    AD::ScheduleDownload,
                    const std::string,
                    Interval,
                    ByteFunction,
                    std::weak_ptr< Messageable >
                >(
                    [=](AD::ScheduleDownload,
                        const std::string& url,
                        const Interval& interval,
                        const ByteFunction& func,
                        const std::weak_ptr< Messageable >& wmsg)
                    {
                        IntervalList list(interval);
                        scheduleDownload(url,std::move(list),func,wmsg);
                    }
                ),
                SF::virtualMatch< AD::Shutdown >(
                    [=](AD::Shutdown) {
                        this->shutdown();
                    }
                )
                //,
                //SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                    //[=](GSI::InRegisterItself,const StrongMsgPtr& ptr) {
                        //auto handler = std::make_shared< GenericShutdownGuard >();
                        //handler->setMaster(_myself);
                        //auto msg = SF::vpackPtr< GSI::OutRegisterItself, StrongMsgPtr >(
                            //nullptr, handler
                        //);
                        //assert( nullptr == _notifyExit.lock() );
                        //_notifyExit = handler;
                        //ptr->message(msg);
                    //}
                //),
                //SF::virtualMatch< GenericShutdownGuard::ShutdownTarget >(
                    //[=](GenericShutdownGuard::ShutdownTarget) {
                        //this->shutdown();
                    //}
                //)
            );
        }

    private:
        MessageCache _cache;
        WeakMsgPtr _myself;
        WeakMsgPtr _notifyExit;
        Handler _handler;
        void* _safehandle;
        bool _shutdown;
    };

    struct AsyncDownloaderImitationImpl : public Messageable {

        static const int DOWNLOAD_PERIODICITY_MS;

        AsyncDownloaderImitationImpl() :
            _shutdown(false),
            _handler(genHandler()),
            _downloadSpeedBytesPerSec(1024 * 1024 * 1), // 1 MB/sec default
            _lastPumpStart(0),
            _lastDebt(0),
            _downloadRevision(0)
        {}

        ~AsyncDownloaderImitationImpl() {
            //printf("ASYNC DL IMITATION BITES DUST\n");
        }

        // this is for sending message across threads
        void message(const std::shared_ptr< templatious::VirtualPack >& msg) {
            _cache.enqueue(msg);
        }

        // this is for sending stack allocated (faster)
        // if we know we're on the same thread as GUI
        void message(templatious::VirtualPack& msg) {
            assert( false && "Synchronous messages disabled for this object." );
        }

        static StrongMsgPtr spinupNew() {
            auto result = std::make_shared< AsyncDownloaderImitationImpl >();
            result->_myself = result;

            std::unique_ptr< std::promise<void> > prom( new std::promise<void> );
            auto rawProm = prom.get();
            auto future = prom->get_future();
            std::weak_ptr< AsyncDownloaderImitationImpl > weak = result;
            std::thread(
                [=]() {
                    auto locked = weak.lock();
                    rawProm->set_value();
                    result->messageLoop(locked);
                }
            ).detach();

            future.wait();

            return result;
        }

    private:

        static std::unique_ptr< const char[] > s_dummyBuffer;

        struct DownloadJobImitation {

            DownloadJobImitation(
                IntervalList&& toDownload,
                const ByteFunction& byteFunc,
                const WeakMsgPtr& otherNotify
            ) : _priority(0),
                _downloadRevision(0),
                _remaining(std::move(toDownload)),
                _byteFunc(byteFunc),
                _otherNotify(otherNotify),
                _stopAsked(false)
            {}

            // download and return if finished
            bool download(int64_t bytes,const char* dummyBuffer) {
                if (isDone()) {
                    return true;
                }

                _remaining.randomEmptyIntervals(bytes,
                    [&](const Interval& interval) {
                        _remaining.append(interval);
                        _stopAsked |= !_byteFunc(dummyBuffer,interval.start(),interval.end());
                        if (_stopAsked) {
                            return false;
                        }
                        return true;
                    }
                );
                bool done = isDone();
                if (done) {
                    auto notifyLock = _otherNotify.lock();
                    if (nullptr != notifyLock) {
                        auto msg = SF::vpack<
                            AsyncDownloader::OutDownloadFinished
                        >(nullptr);
                        notifyLock->message(msg);
                    }
                }
                return done;
            }

            bool isDone() const {
                return !_stopAsked && _remaining.isFilled();
            }

            void setPriority(int priority) {
                _priority = priority;
            }

            int getPriority() const {
                return _priority;
            }

            void tagRevision(int64_t revision) {
                _downloadRevision = revision;
            }

            int64_t getRevision() const {
                return _downloadRevision;
            }

            static bool compareTwo(
                const std::unique_ptr< DownloadJobImitation >& left,
                const std::unique_ptr< DownloadJobImitation >& right)
            {
                if (left->_priority < right->_priority) {
                    return true;
                }

                if (left->_downloadRevision > right->_downloadRevision) {
                    return true;
                }

                return false;
            }

        private:
            int _priority;
            int64_t _downloadRevision;
            IntervalList _remaining;
            ByteFunction _byteFunc;
            WeakMsgPtr _otherNotify;
            bool _stopAsked;
        };

        typedef std::unique_ptr< DownloadJobImitation > ImitationPtr;

        void messageLoop(const std::shared_ptr< AsyncDownloaderImitationImpl >& myself) {
            while (!_shutdown) {
                if (myself.unique()) {
                    shutdown();
                    break;
                }

                auto preDownload = std::chrono::high_resolution_clock::now();
                _cache.process(
                    [=](templatious::VirtualPack& p) {
                        this->_handler->tryMatch(p);
                    }
                );
                downloadRoutine(preDownload);
                auto postDownload = std::chrono::high_resolution_clock::now();
                auto millisecondsPassed = std::chrono::duration_cast<
                    std::chrono::milliseconds
                >(postDownload - preDownload).count();
                auto diff = DOWNLOAD_PERIODICITY_MS - millisecondsPassed;
                if (diff > 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(diff)
                    );
                }
            }

            auto locked = _notifyExit.lock();
            if (nullptr != locked) {
                auto msg = SF::vpack<
                    GenericShutdownGuard::SetFuture >(nullptr);
                locked->message(msg);
            }
        }

        void downloadRoutine(
            const std::chrono::high_resolution_clock::time_point& sessionStart
        )
        {
            if (SA::size(_imitationVector) == 0) {
                // nothing to do
                return;
            }

            static auto referencePoint = std::chrono::high_resolution_clock::now();
            auto deadline = sessionStart + std::chrono::milliseconds( DOWNLOAD_PERIODICITY_MS );
            int64_t thisPumpStart = std::chrono::duration_cast<
                std::chrono::milliseconds
            >(sessionStart - referencePoint).count();
            int64_t toDeliver = byteTargetForSpeed();
            if (_lastDebt > 0) {
                toDeliver += _lastDebt;
                _lastDebt = 0;
            }

            struct PriorityCounter {
                int _priority;
                int _count;
            };

            std::vector< PriorityCounter > pcVec;
            auto addToPriorityVec =
                [&](int priority) {
                    auto flt = SF::filter(pcVec,
                        [=](const PriorityCounter& counter) {
                            return counter._priority == priority;
                        });
                    auto beg = SA::begin(flt);
                    if (beg != SA::end(flt)) {
                        ++beg->_count;
                    } else {
                        PriorityCounter toAdd;
                        toAdd._priority = priority;
                        toAdd._count = 1;
                        SA::add(pcVec,toAdd);
                    }
                };

            auto now = std::chrono::high_resolution_clock::now();
            while (now < deadline && toDeliver > 0) {

                SM::sortS(_imitationVector,DownloadJobImitation::compareTwo);
                SA::clear(pcVec);

                TEMPLATIOUS_FOREACH(auto& i,_imitationVector) {
                    addToPriorityVec(i->getPriority());
                }

                SM::sortS(pcVec,
                    [](const PriorityCounter& left,const PriorityCounter& right) {
                        return left._priority < right._priority;
                    }
                );

                TEMPLATIOUS_FOREACH(const auto& i,pcVec) {
                    int64_t thisPriorityShares =
                        SA::size(pcVec) == 1 ? toDeliver :
                        (toDeliver > 256 ? toDeliver / 2 : toDeliver);
                    int64_t thisPrioritySharesCpy = thisPriorityShares;
                    int64_t averageSlice = thisPriorityShares / i._count;

                    auto priorityFlt = SF::filter(_imitationVector,
                        [&](const ImitationPtr& j) {
                            return j->getPriority() == i._priority;
                        }
                    );

                    TEMPLATIOUS_FOREACH(const auto& j,priorityFlt) {
                        int64_t thisSlice = averageSlice;
                        thisPrioritySharesCpy -= thisSlice;
                        if (thisPrioritySharesCpy < thisSlice) {
                            thisSlice += thisPrioritySharesCpy;
                            thisPrioritySharesCpy = 0;
                        }
                        j->tagRevision(incRevision());

                        while (thisSlice > 0) {
                            auto currDownload = thisSlice > dummySize() ?
                                dummySize() : thisSlice;
                            thisSlice -= currDownload;
                            j->download(currDownload,s_dummyBuffer.get());
                        }
                    }

                    toDeliver -= thisPriorityShares;
                }

                SA::clear(
                    SF::filter(
                        _imitationVector,
                        [](const ImitationPtr& i) {
                            return i->isDone();
                        }
                    )
                );

                now = std::chrono::high_resolution_clock::now();
            }
        }

        int64_t byteTargetForSpeed() {
            return (_downloadSpeedBytesPerSec * DOWNLOAD_PERIODICITY_MS) / 1000;
        }

        void shutdown() {
            _shutdown = true;
        }

        int64_t incRevision() {
            return ++_downloadRevision;
        }

        Handler genHandler() {
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch<
                    AD::ScheduleDownload,
                    const std::string,
                    IntervalList,
                    ByteFunction,
                    std::weak_ptr< Messageable >
                >(
                    [=](AD::ScheduleDownload,
                        const std::string& url,
                        IntervalList& interval,
                        const ByteFunction& func,
                        const std::weak_ptr< Messageable >& wmsg)
                    {
                        if (!interval.isDefined()) {
                            auto locked = wmsg.lock();
                            if (nullptr == locked) {
                                return;
                            }
                            IntervalList predefined(Interval(0,77777));
                            auto toNotify = SF::vpack< AD::OutSizeKnown, int64_t >(
                                nullptr, predefined.range().size()
                            );
                            locked->message(toNotify);

                            ImitationPtr newImitation(
                                new DownloadJobImitation(std::move(predefined),func,wmsg) );
                            SA::add(_imitationVector,std::move(newImitation));
                            return;
                        }
                        ImitationPtr newImitation(
                            new DownloadJobImitation(std::move(interval),func,wmsg) );
                        SA::add(_imitationVector,std::move(newImitation));
                    }
                ),
                SF::virtualMatch<
                    AD::ScheduleDownload,
                    const std::string,
                    Interval,
                    ByteFunction,
                    std::weak_ptr< Messageable >
                >(
                    [=](AD::ScheduleDownload,
                        const std::string& url,
                        const Interval& interval,
                        const ByteFunction& func,
                        const std::weak_ptr< Messageable >& wmsg)
                    {
                        IntervalList list(interval);
                        ImitationPtr newImitation(
                            new DownloadJobImitation(std::move(list),func,wmsg) );
                        SA::add(_imitationVector,std::move(newImitation));
                    }
                ),
                SF::virtualMatch< AD::Shutdown >(
                    [=](AD::Shutdown) {
                        this->shutdown();
                    }
                ),
                SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                    [=](GSI::InRegisterItself,const StrongMsgPtr& ptr) {
                        auto handler = std::make_shared< GenericShutdownGuard >();
                        handler->setMaster(_myself);
                        auto msg = SF::vpackPtr< GSI::OutRegisterItself, StrongMsgPtr >(
                            nullptr, handler
                        );
                        assert( nullptr == _notifyExit.lock() );
                        _notifyExit = handler;
                        ptr->message(msg);
                    }
                ),
                SF::virtualMatch< GenericShutdownGuard::ShutdownTarget >(
                    [=](GenericShutdownGuard::ShutdownTarget) {
                        this->shutdown();
                    }
                )
            );
        }

        MessageCache _cache;
        bool _shutdown;
        Handler _handler;
        std::vector< ImitationPtr > _imitationVector;
        int _downloadSpeedBytesPerSec;

        int64_t _lastPumpStart;
        int64_t _lastDebt;
        int64_t _downloadRevision;

        WeakMsgPtr _myself;
        WeakMsgPtr _notifyExit;
    };

    const int AsyncDownloaderImitationImpl::DOWNLOAD_PERIODICITY_MS = 100; // 100 milliseconds

    StrongMsgPtr AsyncDownloader::createNew(const char* type) {
        if (0 == strcmp(type,"imitation")) {
            return AsyncDownloaderImitationImpl::spinupNew();
        }
        if (0 == strcmp(type,"safenetwork")) {
            return AsyncDownloaderSafeNet::spinupNew();
        }
        return nullptr;
    }

    std::unique_ptr< const char[] > AsyncDownloaderImitationImpl::s_dummyBuffer = prepDummyBuffer();

}
