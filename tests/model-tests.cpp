
#define CATCH_CONFIG_MAIN
#include <templatious/FullPack.hpp>

#include "catch.hpp"

#include <model/IndexedListView.hpp>
#include <model/AsyncSqlite.hpp>
#include <model/TableSnapshot.hpp>

TEMPLATIOUS_TRIPLET_STD;

using namespace SafeLists;

TEST_CASE("model_basic_sqlite","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        static const char* query =
            "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
            "INSERT INTO Friends(Name) VALUES ('Tom');"
            "INSERT INTO Friends(Name) VALUES ('Rebecca');"
            "INSERT INTO Friends(Name) VALUES ('Jim');"
            "INSERT INTO Friends(Name) VALUES ('Roger');"
            "INSERT INTO Friends(Name) VALUES ('Robert');";

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, query
        );
        msg->message(pack);

        pack->wait();
    }

    {
        static const char* query =
            "SELECT Name FROM FRIENDS;";

        std::vector< std::string > friendos;

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*, std::function<void(int,char**,char**)>
        >(
            nullptr, query,
            [&](int cnt,char** values,char** headers) {
                SA::add(friendos,values[0]);
            }
        );
        msg->message(pack);

        pack->wait();

        SM::sort( friendos );

        REQUIRE( friendos[0] == "Jim" );
        REQUIRE( friendos[1] == "Rebecca" );
        REQUIRE( friendos[2] == "Robert" );
        REQUIRE( friendos[3] == "Roger" );
        REQUIRE( friendos[4] == "Tom" );
    }
}

TEST_CASE("model_table_snapshot","[model]") {
    const char* names[] = {"first name","last name"};
    TableSnapshotBuilder bld(2,names);

    bld.setValue(0,"mickey");
    bld.setValue(1,"mouse");
    bld.commitRow();

    bld.setValue(0,"some");
    bld.setValue(1,"name");
    bld.commitRow();

    auto snap = bld.getSnapshot();
}

