
#include <sqlite3.h>

#include <templatious/FullPack.hpp>
#include <io/SafeListDownloader.hpp>
#include <util/ScopeGuard.hpp>

#include "SafeListDownloaderFactory.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    const char* DL_SESSION_SCHEMA =
        "id INTEGER PRIMARY KEY,    "
        "   link text,              "
        "   use_count INT           "
        ");                         "
        "CREATE TABLE to_download ( "
        "   id INTEGER PRIMARY KEY, "
        "   file_path text,         "
        "   file_size INT,          "
        "   file_hash_256 text,     "
        "   status INT DEFAULT 0,   "
        "   priority INT DEFAULT 10 "
        ");                         ";

    const char* DL_SELECT_ABS_PATHS =
        "SELECT file_id, path_name, file_size, file_hash_256    "
        "FROM files "
        "LEFT OUTER JOIN    "
        "(  "
        "   WITH RECURSIVE  "
        "   children(d_id,path_name) AS (   "
        "      SELECT dir_id,dir_name FROM directories WHERE dir_name='root' AND dir_id=1   "
        "      UNION ALL    "
        "      SELECT dir_id,children.path_name || '/' || dir_name  "
        "      FROM directories JOIN children ON directories.dir_parent=children.d_id   "
        "   ) SELECT d_id,path_name FROM children   "
        ")  "
        "ON dir_id=d_id; ";

    const char* DL_INSERT_TO_SESSION =
        "INSERT INTO to_download "
        " (id,file_path,file_size,file_hash_256,status,priority)"
        " VALUES (?1,?2,?3,?4,?5,?6);";

    struct DlSessionData {
        sqlite3* _conn;
        sqlite3_stmt* _statement;
    };

    int insertDownloadSessionCallback(void* data,int size,char** values,char** headers) {
        assert( 4 == size && "Huh?" );
        DlSessionData& dldata = *reinterpret_cast<DlSessionData*>(data);
        sqlite3* sess = dldata._conn;
        sqlite3_stmt* stmt = dldata._statement;

        sqlite3_bind_text(stmt,1,values[0],-1,nullptr);
        sqlite3_bind_text(stmt,2,values[1],-1,nullptr);
        sqlite3_bind_text(stmt,3,values[2],-1,nullptr);
        sqlite3_bind_text(stmt,4,values[3],-1,nullptr);
        sqlite3_bind_int64(stmt,5,0);
        sqlite3_bind_int64(stmt,6,10);

        auto stat = sqlite3_step(stmt);
        assert( stat == SQLITE_DONE && "OH CMON!" );

        return 0;
    }

    // returns in memory database
    // which needs to be saved to file.
    sqlite3* createDownloadSession(sqlite3* connection) {
        sqlite3* result = nullptr;
        sqlite3_open(":memory:",&result);
        assert( nullptr != result && "Aww, cmon, I really need to handle this?" );
        char* err = nullptr;
        int res = sqlite3_exec(result,DL_SESSION_SCHEMA,nullptr,nullptr,&err);
        if (res != 0 && nullptr != err) {
            printf("Sqlite err: %s\n",err);
            assert( false && "Mitch, please!" );
        }

        auto scopeGuard = SafeLists::makeScopeGuard(
            [&]() {
                sqlite3_close(result);
            }
        );

        sqlite3_stmt* statement = nullptr;
        res = sqlite3_prepare(
            result,
            DL_INSERT_TO_SESSION,
            strlen(DL_INSERT_TO_SESSION),
            &statement,
            nullptr
        );
        assert( 0 == res && "You're kidding?" );

        DlSessionData data;
        data._conn = result;
        data._statement = statement;

        auto freeStmt = SafeLists::makeScopeGuard(
            [&]() {
                sqlite3_finalize(statement);
            }
        );

        sqlite3_exec(result,"BEGIN;",nullptr,nullptr,&err);
        res = sqlite3_exec(connection,DL_SELECT_ABS_PATHS,&insertDownloadSessionCallback,&data,&err);
        assert( res == 0 &&  nullptr != err && "BOO!" );
        sqlite3_exec(result,"END;",nullptr,nullptr,&err);

        scopeGuard.dismiss();

        return result;
    }
}

namespace SafeLists {

struct SafeListDownloaderFactoryImpl : public Messageable {

    SafeListDownloaderFactoryImpl() : _handler(genHandler())
    {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false &&
            "Asynchronous messages disabled for SafeListDownloaderFactoryImpl." );
    }

    void message(templatious::VirtualPack& msg) override {
        _handler->tryMatch(msg);
    }

private:
    VmfPtr genHandler() {
        typedef SafeListDownloaderFactory SLDF;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch<
                SLDF::InNewAsync,
                std::string, // path
                StrongMsgPtr, // to notify
                StrongMsgPtr // out object
            >(
                [](SLDF::InNewAsync,
                    const std::string& path,
                    const StrongMsgPtr& toNotify,
                    StrongMsgPtr& output)
                {
                }
            ),
            SF::virtualMatch<
                SLDF::CreateSession,
                StrongMsgPtr,
                StrongMsgPtr,
                std::string
            >(
                [](SLDF::CreateSession,
                   StrongMsgPtr& asyncSqlite,
                   StrongMsgPtr& notifyWhenDone,
                   const std::string& path)
                {
                    WeakMsgPtr notify = notifyWhenDone;

                    typedef std::function<void(sqlite3*)> Sig;

                    auto asyncMessage = SF::vpackPtrWCallback<
                        Sig
                    >(
                        [=](const TEMPLATIOUS_VPCORE<Sig>& pack) {
                            auto locked = notify.lock();
                            if (nullptr == locked) {
                                return;
                            }

                            auto notifyMsg = SF::vpackPtr<
                                SLDF::OutCreateSessionDone
                            >(nullptr);
                            locked->message(notifyMsg);
                        },
                        [=](sqlite3* connection) {
                            sqlite3* memSession = createDownloadSession(connection);
                        }
                    );
                }
            )
        );
    }

    VmfPtr _handler;
};

StrongMsgPtr SafeListDownloaderFactory::createNew() {
    return std::make_shared< SafeListDownloaderFactoryImpl >();
}

}
