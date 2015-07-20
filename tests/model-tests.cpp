
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <model/IndexedListView.hpp>
#include <model/AsyncSqlite.hpp>

using namespace SafeLists;

TEST_CASE("model_string_snapshot","[model]") {
}

TEST_CASE("model_basic_sqlite","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");
}

