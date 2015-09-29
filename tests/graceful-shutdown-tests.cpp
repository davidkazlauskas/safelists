
#include <templatious/FullPack.hpp>
#include "catch.hpp"

#include <util/GracefulShutdownGuard.hpp>

TEMPLATIOUS_TRIPLET_STD;

TEST_CASE("graceful_shutdown_guard_test","[graceful_shutdown]") {
    using namespace SafeLists;

    auto g = GracefulShutdownGuard::makeNew();

}

