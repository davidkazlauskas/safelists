#ifndef GTKMMSESSIONWIDGET_SD2Z0QLJ
#define GTKMMSESSIONWIDGET_SD2Z0QLJ

#include <memory>
#include <gtkmm.h>

namespace SafeLists {

struct GtkSessionWidget {

    static std::shared_ptr< GtkSessionWidget > makeNew();
private:
    GtkSessionWidget(Glib::RefPtr<Gtk::Builder>& bld);
    ~GtkSessionWidget();
    Glib::RefPtr<Gtk::Builder> _container;

    Gtk::Box* _mainBox;
    Gtk::Label* _sessionLabel;
};

struct GtkSessionTab {

    static std::shared_ptr< GtkSessionTab > makeNew();
    Gtk::Notebook* getTabs();
private:
    GtkSessionTab(Glib::RefPtr<Gtk::Builder>& bld);
    Glib::RefPtr<Gtk::Builder> _container;
    Gtk::Notebook* _mainTab;
};

}

#endif /* end of include guard: GTKMMSESSIONWIDGET_SD2Z0QLJ */
