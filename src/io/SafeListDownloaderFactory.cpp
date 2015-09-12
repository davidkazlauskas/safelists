
#include <sqlite3.h>

#include <templatious/FullPack.hpp>
#include <io/SafeListDownloader.hpp>

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
