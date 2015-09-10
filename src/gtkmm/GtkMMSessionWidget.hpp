#ifndef GTKMMSESSIONWIDGET_SD2Z0QLJ
#define GTKMMSESSIONWIDGET_SD2Z0QLJ

#include <memory>
#include <gtkmm.h>

namespace SafeLists {

struct GtkSessionWidget {

    static std::shared_ptr< GtkSessionWidget > makeNew();
private:
    GtkSessionWidget(Glib::RefPtr<Gtk::Builder>& bld);
};

}

#endif /* end of include guard: GTKMMSESSIONWIDGET_SD2Z0QLJ */
