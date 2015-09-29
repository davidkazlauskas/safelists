
#include <templatious/FullPack.hpp>
#include "catch.hpp"

#include <util/GracefulShutdownGuard.hpp>

TEMPLATIOUS_TRIPLET_STD;

struct Sleep100MsShutdownClass : public Messageable {

    Sleep100MsShutdownClass(const Sleep100MsShutdownClass&) = delete;
    Sleep100MsShutdownClass(Sleep100MsShutdownClass&&) = delete;

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _cache.enqueue(msg);
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "..." );
    }

private:
    Sleep100MsShutdownClass();

    MessageCache _cache;
};

TEST_CASE("graceful_shutdown_guard_test","[graceful_shutdown]") {
    using namespace SafeLists;

    auto g = GracefulShutdownGuard::makeNew();

}

