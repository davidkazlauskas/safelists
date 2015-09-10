#ifndef GTKMMSESSIONWIDGET_SD2Z0QLJ
#define GTKMMSESSIONWIDGET_SD2Z0QLJ

#include <memory>
#include <gtkmm.h>
#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct GtkSessionWidget {

    static std::shared_ptr< GtkSessionWidget > makeNew();
    ~GtkSessionWidget();
    Gtk::Box* getMainBox();
private:
    GtkSessionWidget(Glib::RefPtr<Gtk::Builder>& bld);
    Glib::RefPtr<Gtk::Builder> _container;

    Gtk::Box* _mainBox;
    Gtk::Label* _sessionLabel;
};

struct GtkSessionTab {

    struct ModelInterface {
        // Query count of download sessions in model
        // Signature:
        // < QueryCount, int (out number) >
        DUMMY_REG(QueryCount,"DLMDL_QueryCount");
    };

    static std::shared_ptr< GtkSessionTab > makeNew();
    Gtk::Notebook* getTabs();
    ~GtkSessionTab();
    void addSession(const std::shared_ptr<GtkSessionWidget>& widget);
    void setModel(const StrongMsgPtr& model);
    void fullModelUpdate();
private:
    GtkSessionTab(Glib::RefPtr<Gtk::Builder>& bld);

    std::vector< std::shared_ptr<GtkSessionWidget > > _sessions;
    Glib::RefPtr<Gtk::Builder> _container;
    Gtk::Notebook* _mainTab;
    WeakMsgPtr _weakModel;
};

}

#endif /* end of include guard: GTKMMSESSIONWIDGET_SD2Z0QLJ */
