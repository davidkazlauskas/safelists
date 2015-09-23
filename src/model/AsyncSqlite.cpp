
#include <thread>
#include <sqlite3.h>

#include <templatious/FullPack.hpp>

#include <util/Semaphore.hpp>

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

// <3 static asserts
static_assert( SQLITE_VERSION_NUMBER >= 3008012, "Sqlite has to be at least 3.8.12 or higher." );

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
        _sem.notify();
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
        _sem.notify();
    }

    void mainLoop(const std::shared_ptr< AsyncSqliteImpl >& myself) {

        struct FinishGuard {
            FinishGuard(std::promise<void>* prom) :
                _prom(prom) {}

            ~FinishGuard() {
                _prom->set_value();
            }

            std::promise<void>* _prom;
        };

        FinishGuard g(&_finished);

        while (_keepGoing) {
            if (myself.unique()) {
                return;
            }
            _sem.wait();
            _cache.process(
                [=](templatious::VirtualPack& p) {
                    this->_handler->tryMatch(p);
                }
            );
        }
    }

private:
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;
    typedef AsyncSqlite AS;

    struct HolderSingleNum {
        HolderSingleNum(int& out,bool& success)
            : _out(out), _success(success) {}

        int& _out;
        bool& _success;
    };

    struct HolderSingleRow {
        HolderSingleRow(std::string& out)
            : _out(out), _rev(0) {}

        std::string& _out;
        int _rev;
    };

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

    static int sqliteFuncSingleOut(
        void* data,
        int cnt,
        char** value,
        char** header
    )
    {
        HolderSingleNum& hld = *reinterpret_cast<
            HolderSingleNum* >(data);
        hld._out = std::atoi(value[0]);
        hld._success = true;
        return 1;
    }

    static int sqliteFuncSingleRowOut(
        void* data,
        int cnt,
        char** value,
        char** header
    )
    {
        HolderSingleRow& hld = *reinterpret_cast<
            HolderSingleRow* >(data);
        assert( cnt > 0 && "Oh, cmon milky! Can't you select something?" );
        hld._out = value[0];
        for (int i = 1; i < cnt; ++i) {
            hld._out += "|";
            hld._out += value[i];
        }
        ++hld._rev;
        return 1;
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
                    int outCode = sqlite3_exec(
                        this->_sqlite,
                        query.c_str(),
                        snapshotBuilderCallback,
                        &bld,
                        &errmsg
                    );

                    assert( nullptr == errmsg );
                    assert( SQLITE_OK == outCode );
                    outSnap = bld.getSnapshot();
                }
            ),
            SF::virtualMatch< AS::Execute, const char* >(
                [=](AS::Execute,const char* query) {
                    char* errmsg = nullptr;
                    sqlite3_exec(this->_sqlite,query,nullptr,nullptr,&errmsg);
                    assert( nullptr == errmsg );
                }
            ),
            SF::virtualMatch< AS::Execute, const std::string >(
                [=](AS::Execute,const std::string& query) {
                    char* errmsg = nullptr;
                    sqlite3_exec(this->_sqlite,query.c_str(),nullptr,nullptr,&errmsg);
                    assert( nullptr == errmsg );
                }
            ),
            SF::virtualMatch< AS::Execute, const char*, AS::SqliteCallbackSimple >(
                [=](AS::Execute,const char* query, AS::SqliteCallbackSimple& sql) {
                    sqlite3_exec(this->_sqlite,query,&sqliteFuncExec,&sql,nullptr);
                }
            ),
            SF::virtualMatch< AS::OutSingleNum, const std::string, int, bool >(
                [=](AS::OutSingleNum, const std::string& query,int& out,bool& succeeded) {
                    HolderSingleNum h(out,succeeded);
                    sqlite3_exec(this->_sqlite,query.c_str(),&sqliteFuncSingleOut,&h,nullptr);
                    if (h._success) {
                        out = h._out;
                        succeeded = h._success;
                    }
                }
            ),
            SF::virtualMatch< AS::OutSingleRow, const std::string, std::string, bool >(
                [=](AS::OutSingleNum, const std::string& query,std::string& out,bool& succeeded) {
                    HolderSingleRow h(out);
                    sqlite3_exec(this->_sqlite,query.c_str(),&sqliteFuncSingleRowOut,&h,nullptr);
                    if (h._rev == 1) {
                        succeeded = true;
                    } else {
                        succeeded = false;
                    }
                }
            ),
            SF::virtualMatch<
                AsyncSqlite::ArbitraryOperation,
                std::function<void(sqlite3*)>
            >(
                [=](AsyncSqlite::ArbitraryOperation,
                    const std::function<void(sqlite3*)>& session)
                {
                    session(_sqlite);
                }
            ),
            SF::virtualMatch< AsyncSqlite::DummyWait >(
                [](AsyncSqlite::DummyWait) {}
            ),
            SF::virtualMatch< AsyncSqlite::Shutdown >(
                [=](AsyncSqlite::Shutdown) {
                    this->_keepGoing = false;
                    this->_sem.notify();
                }
            )
        );
    }

    bool _keepGoing;
    StackOverflow::Semaphore _sem;
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
        outPtr->mainLoop(outPtr);
    }).detach();

    return fut.get();
}

}

