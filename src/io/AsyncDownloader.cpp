
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
        AsyncDownloaderImitationImpl() :
            _shutdown(false),
            _downloadSpeedBytesPerSec(1024 * 1024 * 1) // 1 MB/sec default
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
        typedef std::function< void() > OnFinishFunction;
        typedef std::weak_ptr< Messageable > WeakMsgPtr;

        static std::unique_ptr< const char[] > s_dummyBuffer;

        struct DownloadJobImitation {

            DownloadJobImitation(
                const Interval& toDownload,
                const ByteFunction& byteFunc,
                const OnFinishFunction& onFinishFunc
            ) : _priority(0),
                _downloadRevision(0),
                _remaining(toDownload),
                _byteFunc(byteFunc),
                _onFinish(onFinishFunc)
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

        private:
            int _priority;
            int64_t _downloadRevision;
            IntervalList _remaining;
            ByteFunction _byteFunc;
            OnFinishFunction _onFinish;
            WeakMsgPtr _otherNotify;
        };

        typedef std::unique_ptr< DownloadJobImitation > ImitationPtr;

        void messageLoop() {
            while (!_shutdown) {
                _cache.process(
                    [=](templatious::VirtualPack& p) {
                        this->_handler->tryMatch(p);
                    }
                );
                downloadRoutine();
            }
        }

        void downloadRoutine() {

        }

        void shutdown() {
            _shutdown = true;
        }

        Handler genHandler() {
            typedef AsyncDownloader AD;
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch<
                    AD::ScheduleDownload,
                    ByteFunction,
                    std::weak_ptr< Messageable >
                >(
                    [=](AD::ScheduleDownload,const ByteFunction& func,
                        const std::weak_ptr< Messageable >& wmsg)
                    {

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
    };

    StrongMsgPtr AsyncDownloader::createNew(const char* type) {
        if (0 == strcmp(type,"imitation")) {
            return AsyncDownloaderImitationImpl::spinupNew();
        }
        return nullptr;
    }

    std::unique_ptr< const char[] > AsyncDownloaderImitationImpl::s_dummyBuffer = prepDummyBuffer();

}
