
#include <util/GenericStMessageable.hpp>
#include "GtkMMThemeManager.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    struct GtkMMThemeManagerImpl : public GenericStMessageable {
        GtkMMThemeManagerImpl() {
            regHandler(genHandler());
        }

        VmfPtr genHandler() {
            typedef GtkMMThemeManager MNG;
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch< MNG::LoadTheme, const std::string >(
                    [](ANY_CONV,const std::string& path) {

                    }
                )
            );
        }
    };

    StrongMsgPtr GtkMMThemeManager::makeNew() {
        return nullptr;
    }

}
