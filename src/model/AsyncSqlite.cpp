
#include <templatious/FullPack.hpp>

#include "AsyncSqlite.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

struct AsyncSqliteImpl : public Messageable {

    AsyncSqliteImpl() : _keepGoing(true) {}

    void message(templatious::VirtualPack& pack) {
        _handler->tryMatch(pack);
    }

    void message(const StrongPackPtr& pack) {
        _cache.enqueue(pack);
    }

private:
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< AsyncSqlite::Shutdown >(
                [=](AsyncSqlite::Shutdown) {
                    this->_keepGoing = false;
                }
            )
        );
    }

    bool _keepGoing;
    MessageCache _cache;
    VmfPtr _handler;
};

StrongMsgPtr AsyncSqlite::createNew(const char* name) {
    return nullptr;
}

}

