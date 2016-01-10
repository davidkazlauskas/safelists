
#include <util/GenericStMessageable.hpp>
#include <util/ProgramArgs.hpp>
#include "GlobalConsts.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    int lookupGlobal(const std::string& path,std::string& out) {
        if (path == "userdatapath") {
            out = SafeLists::userDataPath();
        } else if (path == "settingspath") {
            auto udata = SafeLists::userDataPath();
            udata += "/settings.json";
        } else {
            return 1;
        }

        return 0;
    }

}

namespace SafeLists {

    struct GlobalConstsImpl : public GenericStMessageable {
        GlobalConstsImpl() {
            regHandler(genHandler());
        }

        VmfPtr genHandler() {
            typedef GlobalConsts GLC;

            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch<
                    GLC::LookupString, const std::string,
                    std::string, int
                >(
                    [](ANY_CONV,const std::string& path,std::string& out,int& err) {
                        err = lookupGlobal(path,out);
                    }
                )
            );
        }
    };

    StrongMsgPtr GlobalConsts::getConsts() {
        static auto instance = std::make_shared< GlobalConstsImpl >();
        return instance;
    }

}
