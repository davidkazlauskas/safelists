
#include <boost/filesystem.hpp>

#include <util/Semaphore.hpp>
#include <util/GenericShutdownGuard.hpp>
#include <util/ScopeGuard.hpp>

#include "RandomFileWriter.hpp"
#include "RandomFileWriterImpl.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef SafeLists::GracefulShutdownInterface GSI;


namespace {

    namespace fs = boost::filesystem;

    int writeDataToFileWDir(const std::string& pathStr,const std::string& data) {
        fs::path path(pathStr.c_str());
        auto parentDir = path.parent_path();
        if (!fs::exists(parentDir)) {
            fs::create_directories(parentDir);
        }

        auto file = ::fopen(pathStr.c_str(),"w");
        if (nullptr == file) {
            return 1;
        }

        auto g = SCOPE_GUARD_LC(
            ::fclose(file);
        );

        ::fwrite(data.c_str(),sizeof(char),data.size(),file);

        return 0;
    }
}

namespace SafeLists {

struct RandomFileWriterImpl : public Messageable {
    RandomFileWriterImpl() :
        _writeCache(16), // default
        _handler(genHandler()),
        _keepGoing(true)
    {}

    ~RandomFileWriterImpl() {
        //printf("FILE WRITER SHIRTDOWN\n");
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _msgCache.enqueue(msg);
        _sem.notify();
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "Synchronous messages disabled for RandomFileWriterImpl." );
    }

    static std::shared_ptr< RandomFileWriterImpl > spinNew() {
        auto result = std::make_shared< RandomFileWriterImpl >();
        result->_myself = result;
        auto cpy = result; // being explicit... probably more than needed
        std::thread(
            [=]() {
                cpy->processLoop(cpy);
            }
        ).detach();
        return result;
    }

private:
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        typedef RandomFileWriter RFW;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch<
                RFW::WriteData,
                std::string,
                std::unique_ptr< char[] >,
                int64_t,
                int64_t
            >(
                [=](RFW::WriteData,const std::string& path,
                    const std::unique_ptr< char[] >& data,
                    int64_t start,int64_t end)
                {
                auto writer = _writeCache.getItem(path.c_str(),-1);
                writer->write(data.get(),start,end-start);
                }
            ),
            SF::virtualMatch< RFW::ClearCache >(
                [=](RFW::ClearCache) {
                    this->_writeCache.clear();
                }
            ),
            SF::virtualMatch< RFW::ClearCache, std::string >(
                [=](RFW::ClearCache,const std::string& path) {
                    this->_writeCache.dropItem(path.c_str());
                }
            ),
            SF::virtualMatch< RFW::WaitWrites >(
                [](RFW::WaitWrites) {}
            ),
            SF::virtualMatch< RFW::DoesFileExist, const std::string, bool >(
                [](RFW::DoesFileExist,const std::string& path,bool& out) {
                    out = boost::filesystem::exists(path.c_str());
                }
            ),
            SF::virtualMatch< RFW::DeleteFile, const std::string >(
                [](RFW::DeleteFile,const std::string& path) {
                    boost::filesystem::remove(path.c_str());
                }
            ),
            SF::virtualMatch<
                RFW::WriteStringToFileWDir,
                const std::string,
                const std::string,
                int
            >(
                [](ANY_CONV,const std::string& path,const std::string& data,int& outerr) {
                    outerr = writeDataToFileWDir(path,data);
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
                    this->_keepGoing = false;
                }
            )
        );
    }

    void processLoop(const std::shared_ptr< RandomFileWriterImpl >& impl) {
        while (_keepGoing && !impl.unique()) {
            _sem.wait();
            _msgCache.process(
                [=](templatious::VirtualPack& pack) {
                    this->_handler->tryMatch(pack);
                }
            );
        }

        auto locked = _notifyExit.lock();
        if (nullptr != locked) {
            auto msg = SF::vpack<
                GenericShutdownGuard::SetFuture >(nullptr);
            locked->message(msg);
        }
    }

    RandomFileWriteCache _writeCache;
    StackOverflow::Semaphore _sem;
    MessageCache _msgCache;
    VmfPtr _handler;
    WeakMsgPtr _myself;
    WeakMsgPtr _notifyExit;
    bool _keepGoing;
};

// singleton
StrongMsgPtr RandomFileWriter::make() {
    static auto theOne = RandomFileWriterImpl::spinNew();
    return theOne;
}

}
