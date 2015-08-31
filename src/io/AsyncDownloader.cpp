
#include <cstring>
#include <future>

#include <templatious/FullPack.hpp>
#include <io/Interval.hpp>

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

    struct AsyncDownloaderImitationImpl : public Messageable {

        static const int DOWNLOAD_PERIODICITY_MS = 100; // 100 milliseconds

        AsyncDownloaderImitationImpl() :
            _shutdown(false),
            _downloadSpeedBytesPerSec(1024 * 1024 * 1), // 1 MB/sec default
            _lastPumpStart(0),
            _lastDebt(0),
            _downloadRevision(0)
        {}

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

            std::thread(
                [=]() {
                    result->messageLoop();
                }
            ).detach();

            return result;
        }

    private:
        typedef std::unique_ptr< templatious::VirtualMatchFunctor > Handler;

        typedef std::function< bool(const char*,int64_t,int64_t) > ByteFunction;
        typedef std::weak_ptr< Messageable > WeakMsgPtr;

        static std::unique_ptr< const char[] > s_dummyBuffer;

        struct DownloadJobImitation {

            DownloadJobImitation(
                const Interval& toDownload,
                const ByteFunction& byteFunc,
                const WeakMsgPtr& otherNotify
            ) : _priority(0),
                _downloadRevision(0),
                _remaining(toDownload),
                _byteFunc(byteFunc),
                _otherNotify(otherNotify)
            {}

            // download and return if finished
            bool download(int64_t bytes,const char* dummyBuffer) {
                _remaining.randomEmptyIntervals(bytes,
                    [&](const Interval& interval) {
                        _byteFunc(dummyBuffer,interval.start(),interval.end());
                        return true;
                    }
                );
                return isDone();
            }

            bool isDone() const {
                return _remaining.isFilled();
            }

            void setPriority(int priority) {
                _priority = priority;
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
        };

        typedef std::unique_ptr< DownloadJobImitation > ImitationPtr;

        void messageLoop() {
            while (!_shutdown) {
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
        }

        void downloadRoutine(
            const std::chrono::high_resolution_clock::time_point& sessionStart
        )
        {
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

            auto now = std::chrono::high_resolution_clock::now();
            while (now < deadline && toDeliver > 0) {

                SM::sortS(_imitationVector,DownloadJobImitation::compareTwo);

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
            typedef AsyncDownloader AD;
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch<
                    AD::ScheduleDownload,
                    Interval,
                    ByteFunction,
                    std::weak_ptr< Messageable >
                >(
                    [=](AD::ScheduleDownload,
                        const Interval& interval,
                        const ByteFunction& func,
                        const std::weak_ptr< Messageable >& wmsg)
                    {
                        ImitationPtr newImitation(
                            new DownloadJobImitation(interval,func,wmsg) );
                        SA::add(_imitationVector,std::move(newImitation));
                    }
                ),
                SF::virtualMatch< AD::Shutdown >(
                    [=](AD::Shutdown) {
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
    };

    StrongMsgPtr AsyncDownloader::createNew(const char* type) {
        if (0 == strcmp(type,"imitation")) {
            return AsyncDownloaderImitationImpl::spinupNew();
        }
        return nullptr;
    }

    std::unique_ptr< const char[] > AsyncDownloaderImitationImpl::s_dummyBuffer = prepDummyBuffer();

}
