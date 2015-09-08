
#include <fstream>

#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicVirtualMatchFunctor.hpp>
#include <boost/filesystem.hpp>
#include <io/AsyncDownloader.hpp>
#include <io/SafeListDownloader.hpp>
#include <io/Interval.hpp>
#include <io/RandomFileWriter.hpp>

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

    StrongMsgPtr testDownloader() {
        typedef SafeLists::AsyncDownloader AD;
        static auto imitation = AD::createNew("imitation");
        return imitation;
    }
}

TEST_CASE("async_downloader_dummy","[async_downloader]") {
    auto downloader = testDownloader();
    typedef SafeLists::Interval Int;
    typedef SafeLists::IntervalList IntList;
    typedef SafeLists::AsyncDownloader AD;

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
    std::unique_ptr< IntList > toDownload(new IntList(Int(0,1024 * 1024))); // 1MB
    IntList copy = toDownload->clone();
    auto downloadJob = SF::vpackPtr<
        AD::ScheduleDownload,
        IntList,
        std::function< bool(const char*,int64_t,int64_t) >,
        std::weak_ptr< Messageable >
    >(
        nullptr,
        std::move(copy),
        [&](const char* buffer,int64_t start,int64_t end) {
            auto size = end - start;
            *success &= ensureExpected(size,buffer);
            toDownload->append(Int(start,end));
            return true;
        },
        handler
    );

    downloader->message(downloadJob);

    future.wait();

    *success &= toDownload->isFilled();

    REQUIRE( *success );
}

namespace {
    bool isFileGood(const char* path,int64_t size) {
        std::ifstream in(path,std::ios::binary);
        int64_t count = 0;
        char ch;
        while (in.get(ch)) {
            if (ch != '7')
                return false;
            ++count;
        }
        return count == size;
    }
}

TEST_CASE("safelist_downloader_example_download","[safelist_downloader]") {
    const char* dlPath = "downloadtest1";
    std::string dlPathAbs = dlPath;
    dlPathAbs += "/safelist_session";
    namespace fs = boost::filesystem;
    fs::remove_all(dlPath);
    fs::create_directory(dlPath);
    fs::copy_file("exampleData/dlsessions/2/safelist_session",dlPathAbs);

    typedef SafeLists::SafeListDownloader SLD;

    std::unique_ptr< std::promise<void> > promPtr(
        new std::promise<void>()
    );
    auto rawCopy = promPtr.get();
    auto future = promPtr->get_future();
    auto downloader = testDownloader();
    auto writer = SafeLists::RandomFileWriter::make();
    auto notifier = std::make_shared<
        SafeLists::MessageableMatchFunctor
    >(
        SF::virtualMatchFunctorPtr(
            SF::virtualMatch< SLD::OutDone >(
                [=](SLD::OutDone) {
                    rawCopy->set_value();
                }
            )
        )
    );

    auto handle =
        SLD::startNew(dlPathAbs.c_str(),writer,downloader,notifier);

    future.wait();
    auto finishWriting = SF::vpackPtrCustom<
        templatious::VPACK_WAIT,
        SafeLists::RandomFileWriter::WaitWrites
    >(nullptr);
    writer->message(finishWriting);
    finishWriting->wait();

    bool result = true;
    result &= isFileGood("downloadtest1/fileA",1048576);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileB",2097152);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileC",349525);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileD",2446675);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileE",5941925);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fldA/fileF",9437175);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fldA/fileG",77777);
    REQUIRE( result );
}
