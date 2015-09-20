#include "AsyncSqliteFactory.hpp"

#include <model/AsyncSqlite.hpp>

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    struct AsyncSqliteFactoryImpl : public Messageable {
        AsyncSqliteFactoryImpl() : _handler(genHandler())
        {}

        void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
            assert( false && "Thou shall not use AsyncSqliteFactory asynchronously." );
        }

        void message(templatious::VirtualPack& msg) override {
            _handler->tryMatch(msg);
        }

        VmfPtr genHandler() {
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch<
                    AsyncSqliteFactory::CreateNew,
                    const std::string,
                    StrongMsgPtr
                >(
                    [=](AsyncSqliteFactory::CreateNew,
                        const std::string& path,
                        StrongMsgPtr& out)
                    {
                        out = AsyncSqlite::createNew(path.c_str());
                    }
                )
            );
        }

        VmfPtr _handler;
    };

    StrongMsgPtr AsyncSqliteFactory::createNew() {
        return std::make_shared< AsyncSqliteFactoryImpl >();
    }

}

