
#include <gtkmm.h>
#include <util/GenericStMessageable.hpp>
#include "GtkMMThemeManager.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    struct GtkMMThemeManagerImpl : public GenericStMessageable {
        GtkMMThemeManagerImpl(Gtk::Window* wnd) :
            _wnd(wnd)
        {
            regHandler(genHandler());
        }

        VmfPtr genHandler() {
            typedef GtkMMThemeManager MNG;
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch< MNG::LoadTheme, const std::string >(
                    [=](ANY_CONV,const std::string& path) {
                        auto css = Gtk::CssProvider::create();
                        css->load_from_path(path);
                        auto screen = Gdk::Screen::get_default();
                        auto ctx = _wnd->get_style_context();
                        ctx->add_provider_for_screen(
                            screen,
                            css,
                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                    }
                )
            );
        }

        Gtk::Window* _wnd;
    };

    StrongMsgPtr GtkMMThemeManager::makeNew() {
        return nullptr;
    }

}
