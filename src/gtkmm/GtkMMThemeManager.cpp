
#include <util/GenericStMessageable.hpp>
#include "GtkMMThemeManager.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    const char* THEME_RESETTER =
        "/* @import this colorsheet to get the default values for every property.\n"
         "* This is useful when writing special CSS tests that should not be\n"
         "* inluenced by themes - not even the default ones.\n"
         "* Keep in mind that the output will be very ugly and not look like\n"
         "* anything GTK.\n"
         "* Also, when adding new style properties, please add them here.\n"
         "*/\n"
        "* {\n"
          "color: inherit;\n"
          "font-size: inherit;\n"
          "background-color: initial;\n"
          "font-family: inherit;\n"
          "font-style: inherit;\n"
          "font-variant: inherit;\n"
          "font-weight: inherit;\n"
          "text-shadow: inherit;\n"
          "icon-shadow: inherit;\n"
          "box-shadow: initial;\n"
          "margin-top: initial;\n"
          "margin-left: initial;\n"
          "margin-bottom: initial;\n"
          "margin-right: initial;\n"
          "padding-top: initial;\n"
          "padding-left: initial;\n"
          "padding-bottom: initial;\n"
          "padding-right: initial;\n"
          "border-top-width: initial;\n"
          "border-left-width: initial;\n"
          "border-bottom-width: initial;\n"
          "border-right-width: initial;\n"
          "border-top-left-radius: initial;\n"
          "border-top-right-radius: initial;\n"
          "border-bottom-right-radius: initial;\n"
          "border-bottom-left-radius: initial;\n"
          "border-top-style: initial;\n"
          "border-left-style: initial;\n"
          "border-bottom-style: initial;\n"
          "border-right-style: initial;\n"
          "background-clip: initial;\n"
          "background-origin: initial;\n"
          "border-top-color: initial;\n"
          "border-right-color: initial;\n"
          "border-bottom-color: initial;\n"
          "border-left-color: initial;\n"
          "background-repeat: initial;\n"
          "background-image: initial;\n"
          "border-image-source: initial;\n"
          "border-image-repeat: initial;\n"
          "border-image-slice: initial;\n"
          "border-image-width: initial;\n"
          "engine: initial;\n"
          "transition: initial;\n"
          "gtk-key-bindings: initial;\n"

          "-GtkWidget-focus-line-width: 0;\n"
          "-GtkWidget-focus-padding: 0;\n"
          "-GtkNotebook-initial-gap: 0;\n"
        "}\n";

    const char* RALEIGH_PATH = "org/gtk/libgtk/theme/Raleigh.css";
    const char* ADWAITA_PATH = "org/gtk/libgtk/theme/Adwaita.css";
    const char* ADWAITA_DARK_PATH = "org/gtk/libgtk/theme/Adwaita.css";

    struct GtkMMThemeManagerImpl : public GenericStMessageable {
        GtkMMThemeManagerImpl(Gtk::Window* wnd) :
            _wnd(wnd)
        {
            regHandler(genHandler());
            _resetter = Gtk::CssProvider::create();
            _resetter->load_from_data(THEME_RESETTER);
            _provider = Gtk::CssProvider::create();
            auto ctx = _wnd->get_style_context();
            auto screen = Gdk::Screen::get_default();
            ctx->add_provider_for_screen(
                screen,
                _resetter,
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
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
                            // load themes by resource
                        } else {
                            _provider->load_from_path(path);
                        }
                    }
                )
            );
        }

        Gtk::Window* _wnd;
        Glib::RefPtr< Gtk::CssProvider > _resetter;
        Glib::RefPtr< Gtk::CssProvider > _provider;
    };

    StrongMsgPtr GtkMMThemeManager::makeNew(Gtk::Window* mainWnd) {
        return std::make_shared< GtkMMThemeManagerImpl >(mainWnd);
    }

}
