
#include <util/GenericStMessageable.hpp>
#include "GtkMMThemeManager.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    struct GtkMMThemeManagerImpl : public GenericStMessageable {
        GtkMMThemeManagerImpl(Gtk::Window* wnd) :
            _wnd(wnd)
        {
            regHandler(genHandler());
            _provider = Gtk::CssProvider::create();
            auto ctx = _wnd->get_style_context();
            auto screen = Gdk::Screen::get_default();
            ctx->add_provider_for_screen(
                screen,
                _provider,
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }

        VmfPtr genHandler() {
            typedef GtkMMThemeManager MNG;
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch< MNG::LoadTheme, const std::string >(
                    [=](ANY_CONV,const std::string& path) {
                        assert( !path.is_empty() && "Some path must be supplied." );
                        if (path[0] == '@') {
                            // load themes by name
                        } else {
                            _provider->load_from_path(path);
                        }
                    }
                )
            );
        }

        Gtk::Window* _wnd;
        Glib::RefPtr< Gtk::CssProvider > _provider;
    };

    StrongMsgPtr GtkMMThemeManager::makeNew(Gtk::Window* mainWnd) {
        return std::make_shared< GtkMMThemeManagerImpl >(mainWnd);
    }

}
