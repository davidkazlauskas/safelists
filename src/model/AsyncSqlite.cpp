
#include <thread>

#include <templatious/FullPack.hpp>

#include "AsyncSqlite.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

struct AsyncSqliteImpl : public Messageable {

    AsyncSqliteImpl() :
        _keepGoing(true),
        _handler(genHandler()) {}

    void message(templatious::VirtualPack& pack) {
        _g.assertThread();
        _handler->tryMatch(pack);
    }

    void message(const StrongPackPtr& pack) {
        _cache.enqueue(pack);
    }

    void mainLoop() {
        std::unique_lock< std::mutex > ul;
        while (_keepGoing) {
            _cv.wait(ul);
            _cache.process(
                [=](templatious::VirtualPack& p) {
                    this->_handler->tryMatch(p);
                }
            );
        }
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
    std::mutex _mtx;
    std::condition_variable _cv;
    MessageCache _cache;
    VmfPtr _handler;
    ThreadGuard _g;
};

StrongMsgPtr AsyncSqlite::createNew(const char* name) {
    std::promise< StrongMsgPtr > out;
    auto fut = out.get_future();

    std::thread([&]() {
        auto outPtr = std::make_shared< AsyncSqliteImpl >();
        out.set_value(outPtr);
    }).detach();

    return fut.get();
}

}

