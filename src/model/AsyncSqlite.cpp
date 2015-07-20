
#include "AsyncSqlite.hpp"

namespace SafeLists {

struct AsyncSqliteImpl : public Messageable {

    void message(templatious::VirtualPack& pack) {
    }

    void message(const StrongPackPtr& pack) {
        _cache.enqueue(pack);
    }

private:
    MessageCache _cache;
};

StrongMsgPtr AsyncSqlite::createNew(const char* name) {
    return nullptr;
}

}

