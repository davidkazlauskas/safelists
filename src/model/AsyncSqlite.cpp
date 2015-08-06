
#include <thread>
#include <sqlite3.h>

#include <templatious/FullPack.hpp>

#include "AsyncSqlite.hpp"
#include "TableSnapshot.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

int snapshotBuilderCallback(void* snapshotBuilder,int argc,char** argv,char** colName) {
    TableSnapshotBuilder& bld = *reinterpret_cast< TableSnapshotBuilder* >(snapshotBuilder);
    TEMPLATIOUS_0_TO_N(i,argc) {
        bld.setValue(i,argv[i]);
    }
    bld.commitRow();
    return 0;
}

} // end of anon namespace

namespace SafeLists {

struct AsyncSqliteImpl : public Messageable {

    AsyncSqliteImpl(const char* path) :
        _keepGoing(true),
        _handler(genHandler())
    {
        int rc = sqlite3_open(path,&_sqlite);
        assert( rc == SQLITE_OK );
    }

    ~AsyncSqliteImpl() {
        _keepGoing = false;
        _cv.notify_one();
        auto fut = _finished.get_future();
        fut.get();
        sqlite3_close(_sqlite);
    }

    void message(templatious::VirtualPack& pack) {
        _g.assertThread();
        _handler->tryMatch(pack);
    }

    void message(const StrongPackPtr& pack) {
        _cache.enqueue(pack);
        _cv.notify_one();
    }

    void mainLoop() {
        std::unique_lock< std::mutex > ul(_mtx);
        while (_keepGoing) {
            _cv.wait(ul);
            _cache.process(
                [=](templatious::VirtualPack& p) {
                    this->_handler->tryMatch(p);
                }
            );
        }
        _finished.set_value();
    }

private:
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;
    typedef AsyncSqlite AS;

    static int sqliteFuncExec(
        void* data,
        int cnt,
        char** header,
        char** value
    )
    {
        AS::SqliteCallbackSimple& cb = *reinterpret_cast<
            AS::SqliteCallbackSimple* >(data);
        cb(cnt,header,value);
        return 0;
    }

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch<
                AS::ExecuteOutSnapshot,
                std::string,
                std::vector< std::string >,
                TableSnapshot
            >(
                [=](AS::ExecuteOutSnapshot,
                    const std::string& query,
                    const std::vector< std::string >& headers,
                    TableSnapshot& outSnap)
                {
                    templatious::StaticBuffer< const char*, 32 > buf;
                    auto vHead = buf.getStaticVector();

                    SA::addCustom(
                        vHead,
                        [](const std::string& str) {
                            return str.c_str();
                        },
                        headers
                    );

                    TableSnapshotBuilder bld(vHead.size(),vHead.rawBegin());
                    char* errmsg = nullptr;
                    sqlite3_exec(
                        this->_sqlite,
                        query.c_str(),
                        snapshotBuilderCallback,
                        &bld,
                        &errmsg
                    );

                    assert( nullptr == errmsg );
                    outSnap = bld.getSnapshot();
                }
            ),
            SF::virtualMatch< AS::Execute, const char* >(
                [=](AS::Execute,const char* query) {
                    sqlite3_exec(this->_sqlite,query,nullptr,nullptr,nullptr);
                }
            ),
            SF::virtualMatch< AS::Execute, const char*, AS::SqliteCallbackSimple >(
                [=](AS::Execute,const char* query, AS::SqliteCallbackSimple& sql) {
                    sqlite3_exec(this->_sqlite,query,&sqliteFuncExec,&sql,nullptr);
                }
            ),
            SF::virtualMatch< AsyncSqlite::Shutdown >(
                [=](AsyncSqlite::Shutdown) {
                    this->_keepGoing = false;
                    this->_cv.notify_one();
                }
            ),
            SF::virtualMatch< AsyncSqlite::DummyWait >(
                [](AsyncSqlite::DummyWait) {}
            )
        );
    }

    bool _keepGoing;
    std::mutex _mtx;
    std::condition_variable _cv;
    MessageCache _cache;
    VmfPtr _handler;
    ThreadGuard _g;
    std::promise<void> _finished;

    sqlite3* _sqlite;
};

StrongMsgPtr AsyncSqlite::createNew(const char* name) {
    std::promise< StrongMsgPtr > out;
    auto fut = out.get_future();

    std::thread([&]() {
        auto outPtr = std::make_shared< AsyncSqliteImpl >(name);
        out.set_value(outPtr);
        outPtr->mainLoop();
    }).detach();

    return fut.get();
}

}

