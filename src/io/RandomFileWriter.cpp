
#include "RandomFileWriter.hpp"

namespace SafeLists {

struct RandomFileWriterImpl : public Messageable {
    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        // todo
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "Synchronous messages disabled for RandomFileWriterImpl." );
    }
};

// singleton
StrongMsgPtr RandomFileWriter::make() {
    return nullptr;
}

}
