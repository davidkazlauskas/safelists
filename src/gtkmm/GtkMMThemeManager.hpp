
#ifndef GTKMMTHEMEMANAGER_S0SW9GS4
#define GTKMMTHEMEMANAGER_S0SW9GS4

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

    struct GtkMMThemeManager {

        // Load theme of choice
        // Signature:
        // < LoadTheme, std::string (name) >
        DUMMY_REG(LoadTheme,"THM_LoadTheme");

        // Load manifests of available themes
        // Signature:
        // < ReadManifests, StrongMsgPtr (notify) >
        DUMMY_REG(ReadManifests,"THM_ReadManifests");

        // Messaged out when manifest is loaded
        // Signature:
        // < Out_ManifestLoaded, std::string (absolute css path),
        //   std::string (theme title), std::string (shortpath) >
        DUMMY_REG(Out_ManifestLoaded,"THM_RM_OutManifestLoaded");

        // Messaged out when all manifests are read.
        // Signature:
        // < Out_LoadingDone >
        DUMMY_REG(Out_LoadingDone,"THM_RM_OutLoadingDone");

        StrongMsgPtr makeNew(Gtk::Window* mainWindow);
    };

}

#endif /* end of include guard: GTKMMTHEMEMANAGER_S0SW9GS4 */

