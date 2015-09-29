
#include <util/Semaphore.hpp>
#include <util/GracefulShutdownInterface.hpp>

#include "RandomFileWriter.hpp"
#include "RandomFileWriterImpl.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef SafeLists::GracefulShutdownInterface GSI;

namespace SafeLists {
    struct RandomFileWriterImpl;
}

namespace {

typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;
typedef SafeLists::GracefulShutdownInterface GSI;

struct MyShutdownGuard : public Messageable {

    friend struct SafeLists::RandomFileWriterImpl;

    DUMMY_STRUCT(SetFuture);
    DUMMY_STRUCT(ShutdownWriter);

    MyShutdownGuard() :
        _handler(genHandler()),
        _fut(_prom.get_future()) {}

    void message(templatious::VirtualPack& pack) override {
        _handler->tryMatch(pack);
    }

    void message(const StrongPackPtr& pack) override {
        assert( false && "Async message disabled." );
    }

    void setPromise() {
        _prom.set_value();
    }

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< GSI::IsDead, bool >(
                [=](GSI::IsDead,bool& res) {
                    auto locked = _master.lock();
                    res = nullptr == locked;
                }
            ),
            SF::virtualMatch< GSI::ShutdownSignal >(
                [=](GSI::ShutdownSignal) {
                    auto locked = _master.lock();
                    if (locked == nullptr) {
                        _prom.set_value();
                    } else {
                        auto shutdownMsg = SF::vpackPtr<
                            ShutdownWriter
                        >(nullptr);
                        locked->message(shutdownMsg);
                    }
                }
            ),
            SF::virtualMatch< GSI::WaitOut >(
                [=](GSI::WaitOut) {
                    _fut.wait();
                }
            ),
            SF::virtualMatch< SetFuture >(
                [=](SetFuture) {
                    _prom.set_value();
                }
            )
        );
    }

private:
    VmfPtr _handler;
    std::promise< void > _prom;
    std::future< void > _fut;
    WeakMsgPtr _master;
};

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
            SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                [=](GSI::InRegisterItself,const StrongMsgPtr& ptr) {
                    auto handler = std::make_shared< MyShutdownGuard >();
                    handler->_master = _myself;
                    auto msg = SF::vpackPtr< GSI::OutRegisterItself, StrongMsgPtr >(
                        nullptr, handler
                    );
                    assert( nullptr == _notifyExit.lock() );
                    _notifyExit = handler;
                    ptr->message(msg);
                }
            ),
            SF::virtualMatch< MyShutdownGuard::ShutdownWriter >(
                [=](MyShutdownGuard::ShutdownWriter) {
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
                MyShutdownGuard::SetFuture >(nullptr);
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
