
#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicVirtualMatchFunctor.hpp>
#include <io/AsyncDownloader.hpp>
#include <io/Interval.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    bool ensureExpected(int64_t size,const char* buffer) {
        TEMPLATIOUS_0_TO_N(i,size) {
            if (buffer[i] != '7') {
                return false;
            }
        }
        return true;
    }
}

TEST_CASE("async_downloader_dummy","[async_downloader]") {
    typedef SafeLists::AsyncDownloader AD;
    auto downloader = AD::createNew("imitation");
    typedef SafeLists::Interval Int;

    std::unique_ptr< std::promise<void> > prom( new std::promise<void> );
    auto future = prom->get_future();
    auto handler =
        std::make_shared< SafeLists::MessageableMatchFunctor >(
            SF::virtualMatchFunctorPtr(
                SF::virtualMatch< AD::OutDownloadFinished >(
                    [&](AD::OutDownloadFinished) {
                        prom->set_value();
                    }
                )
            )
        );


    std::unique_ptr< bool > success( new bool(true) );
    std::unique_ptr< Int > toDownload(new Int(0,1024 * 1024)); // 1MB
    Int copy(*toDownload);
    auto downloadJob = SF::vpackPtr<
        AD::ScheduleDownload,
        Int,
        std::function< bool(const char*,int64_t,int64_t) >,
        std::weak_ptr< Messageable >
    >(
        nullptr,
        copy,
        [&](const char* buffer,int64_t start,int64_t end) {
            auto size = end - start;
            *success &= ensureExpected(size,buffer);
        },
        handler
    );

    downloader->message(downloadJob);

    future.wait();

    REQUIRE( *success );
}

