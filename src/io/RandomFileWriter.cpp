
#include "RandomFileWriter.hpp"
#include "RandomFileWriterImpl.hpp"

namespace SafeLists {

struct RandomFileWriterImpl : public Messageable {
    RandomFileWriterImpl() : _cache(16) // default
    {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        // todo
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "Synchronous messages disabled for RandomFileWriterImpl." );
    }

    static std::shared_ptr< RandomFileWriterImpl > spinNew() {
        auto result = std::make_shared< RandomFileWriterImpl >();
        return result;
    }

private:
    RandomFileWriteCache _cache;
};

// singleton
StrongMsgPtr RandomFileWriter::make() {
    return nullptr;
}

}
