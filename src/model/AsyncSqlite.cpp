
#include <thread>
#include <sqlite3.h>

#include <templatious/FullPack.hpp>

#include <util/Semaphore.hpp>
#include <util/ScopeGuard.hpp>
#include <util/GenericShutdownGuard.hpp>

#include "AsyncSqlite.hpp"
#include "TableSnapshot.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {
    struct AsyncSqliteImpl;
}

namespace {

int snapshotBuilderCallback(void* snapshotBuilder,int argc,char** argv,char** colName) {
    TableSnapshotBuilder& bld = *reinterpret_cast< TableSnapshotBuilder* >(snapshotBuilder);
    TEMPLATIOUS_0_TO_N(i,argc) {
        bld.setValue(i,argv[i]);
    }
    bld.commitRow();
    return 0;
}

typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;
typedef SafeLists::GracefulShutdownInterface GSI;

struct MyShutdownGuard : public Messageable {

    friend struct SafeLists::AsyncSqliteImpl;

    DUMMY_STRUCT(SetFuture);

    MyShutdownGuard() :
        _handler(genHandler()),
        _fut(_prom.get_future()) {}

    void message(templatious::VirtualPack& pack) override {
        _handler->tryMatch(pack);
    }

    void message(const StrongPackPtr& pack) override {
        assert( false && "Async message disabled." );
    }

    VmfPtr genHandler();

private:
    VmfPtr _handler;
    std::promise< void > _prom;
    std::future< void > _fut;
    std::weak_ptr< SafeLists::AsyncSqliteImpl > _master;
};

} // end of anon namespace

// <3 static asserts
static_assert( SQLITE_VERSION_NUMBER >= 3008012, "Sqlite has to be at least 3.8.12 or higher." );

namespace SafeLists {

struct AsyncSqliteImpl {

    AsyncSqliteImpl(const char* path) :
        _keepGoing(true),
        _handler(genHandler()),
        _path(path),
        _sqlite(nullptr),
        _myself(nullptr)
    {
    }

    ~AsyncSqliteImpl() {
        //printf("ASQL BITES THE DUST\n");
    }

    void enqueueMessage(const StrongPackPtr& pack) {
        _cache.enqueue(pack);
        _sem.notify();
    }

    void mainLoop(const std::shared_ptr< AsyncSqliteImpl >& myself) {
        _myself = &myself;
        int rc = sqlite3_open(_path.c_str(),&_sqlite);
        auto closer = SCOPE_GUARD_LC(
            auto cpy = _sqlite;
            _sqlite = nullptr;
            sqlite3_close(cpy);

            auto locked = _guard.lock();
            if (nullptr != locked) {
                auto msg = SF::vpack<
                    MyShutdownGuard::SetFuture >(nullptr);
                locked->message(msg);
            }
        );

        assert( rc == SQLITE_OK );

        while (_keepGoing) {
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
                    char* outErr = nullptr;
                    sqlite3_exec(this->_sqlite,query.c_str(),&sqliteFuncSingleOut,&h,&outErr);
                    if (h._success) {
                        out = h._out;
                        succeeded = h._success;
                    } else {
                        printf("AsyncSqlite out single num error: |%s|\n",outErr);
                    }
                }
            ),
            SF::virtualMatch< AS::OutSingleRow, const std::string, std::string, bool >(
                [=](AS::OutSingleNum, const std::string& query,std::string& out,bool& succeeded) {
                    HolderSingleRow h(out);
                    char *outErr = nullptr;
                    sqlite3_exec(this->_sqlite,query.c_str(),&sqliteFuncSingleRowOut,&h,&outErr);
                    if (h._rev == 1) {
                        succeeded = true;
                    } else {
                        succeeded = false;
                        if (nullptr != outErr) {
                            printf("AsyncSqlite (OutSingleRow) query: |%s|\n",query.c_str());
                            printf("AsyncSqlite (OutSingleRow) error: |%s|\n",outErr);
                        }
                    }
                }
            ),
            SF::virtualMatch< AS::OutAffected, const std::string, int >(
                [=](AS::OutSingleNum, const std::string& query,int& outAffected) {
                    char* errmsg = nullptr;
                    int res = sqlite3_exec(this->_sqlite,query.c_str(),nullptr,nullptr,&errmsg);
                    if (nullptr != errmsg) {
                        printf("Snap, query failed: %s\n",errmsg);
                    }
                    assert( res == 0 && errmsg == nullptr && "Snap, queries failin bro..." );
                    outAffected = sqlite3_changes(this->_sqlite);
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
            SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                [=](GSI::InRegisterItself,const StrongMsgPtr& ptr) {
                    auto handler = std::make_shared< MyShutdownGuard >();
                    assert( _myself != nullptr && "No null milky..." );
                    handler->_master = *_myself;
                    _guard = handler;
                    auto msg = SF::vpackPtr< GSI::OutRegisterItself, StrongMsgPtr >(
                        nullptr, handler
                    );
                    ptr->message(msg);
                }
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
    std::string _path;

    const std::shared_ptr< AsyncSqliteImpl >* _myself;
    std::weak_ptr< MyShutdownGuard > _guard;

    sqlite3* _sqlite;
};

struct AsyncSqliteProxy : public Messageable {
    void message(templatious::VirtualPack& pack) override {
        assert( false && "Synchronous messages are disabled." );
    }

    void message(const StrongPackPtr& pack) override {
        auto locked = _weakPtr.lock();
        assert( nullptr != locked &&
            "Not cool bro, shouldn't be destroyed...");
        locked->enqueueMessage(pack);
    }

    ~AsyncSqliteProxy() {
        auto locked = _weakPtr.lock();
        if (nullptr == locked) {
            return;
        }
        //assert( nullptr != locked
            //&& "I should have destroyed it..." );
        auto terminateMsg = SF::vpackPtr<
            AsyncSqlite::Shutdown
        >(nullptr);
        locked->enqueueMessage(terminateMsg);
    }

    // if things go well, this could be
    // ptimized to use raw pointer...
    std::weak_ptr< AsyncSqliteImpl > _weakPtr;
};

StrongMsgPtr AsyncSqlite::createNew(const char* name) {
    typedef std::promise< std::shared_ptr<AsyncSqliteImpl> > Prom;
    std::unique_ptr< Prom > promPtr(new Prom());
    auto fut = promPtr->get_future();

    auto proxy = std::make_shared< AsyncSqliteProxy >();

    auto promRaw = promPtr.get();

    std::string nameCpy = name;
    std::thread([=]() {
        auto outPtr = std::make_shared< AsyncSqliteImpl >(
            nameCpy.c_str());
        promRaw->set_value(outPtr);
        outPtr->mainLoop(outPtr);
    }).detach();

    proxy->_weakPtr = fut.get();
    return proxy;
}

}

namespace {

    VmfPtr MyShutdownGuard::genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< GSI::IsDead, bool >(
                [=](GSI::IsDead,bool& res) {
                    auto locked = _master.lock();
                    res = nullptr == locked;
                }
            ),
            SF::virtualMatch< GSI::ShutdownSignal >(
                [=](GSI::ShutdownSignal) {
                    auto locked = _master.lock();
                    if (locked == nullptr) {
                        _prom.set_value();
                    } else {
                        auto shutdownMsg = SF::vpackPtr<
                            SafeLists::AsyncSqlite::Shutdown
                        >(nullptr);
                        locked->enqueueMessage(shutdownMsg);
                    }
                }
            ),
            SF::virtualMatch< GSI::WaitOut >(
                [=](GSI::WaitOut) {
                    _fut.wait();
                }
            ),
            SF::virtualMatch< SetFuture >(
                [=](SetFuture) {
                    _prom.set_value();
                }
            )
        );
    }

}
