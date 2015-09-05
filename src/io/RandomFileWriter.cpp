
#include <util/Semaphore.hpp>

#include "RandomFileWriter.hpp"
#include "RandomFileWriterImpl.hpp"

namespace SafeLists {

struct RandomFileWriterImpl : public Messageable {
    RandomFileWriterImpl() : _writeCache(16) // default
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

    void processLoop(const std::shared_ptr< RandomFileWriterImpl >& impl) {

    }

    RandomFileWriteCache _writeCache;
    StackOverflow::Semaphore _sem;
    MessageCache _msgCache;
};

// singleton
StrongMsgPtr RandomFileWriter::make() {
    return nullptr;
}

}
