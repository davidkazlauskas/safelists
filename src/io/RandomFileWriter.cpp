
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
        std::thread(
            [=]() {

            }
        ).detach();
        return result;
    }

private:
    RandomFileWriteCache _writeCache;
    StackOverflow::Semaphore _sem;
    MessageCache _msgCache;
};

// singleton
StrongMsgPtr RandomFileWriter::make() {
    return nullptr;
}

}
