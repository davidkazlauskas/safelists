
#include <util/Semaphore.hpp>

#include "RandomFileWriter.hpp"
#include "RandomFileWriterImpl.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

struct RandomFileWriterImpl : public Messageable {
    RandomFileWriterImpl() :
        _writeCache(16), // default
        _handler(genHandler())
    {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _msgCache.enqueue(msg);
        _sem.notify();
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "Synchronous messages disabled for RandomFileWriterImpl." );
    }

    static std::shared_ptr< RandomFileWriterImpl > spinNew() {
        auto result = std::make_shared< RandomFileWriterImpl >();
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
            SF::virtualMatch< RFW::ClearCache >(
                [=](RFW::ClearCache) {
                    this->_writeCache.clear();
                }
            )
        );
    }

    void processLoop(const std::shared_ptr< RandomFileWriterImpl >& impl) {
        while (!impl.unique()) {
            _sem.wait();

        }
    }

    RandomFileWriteCache _writeCache;
    StackOverflow::Semaphore _sem;
    MessageCache _msgCache;
    VmfPtr _handler;
};

// singleton
StrongMsgPtr RandomFileWriter::make() {
    return nullptr;
}

}
