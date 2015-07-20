
#include "AsyncSqlite.hpp"

namespace SafeLists {

struct AsyncSqliteImpl : public Messageable {

};

StrongMsgPtr AsyncSqlite::createNew(const char* name) {
    return nullptr;
}

}

