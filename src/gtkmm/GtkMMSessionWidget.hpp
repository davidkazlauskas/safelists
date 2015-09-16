#ifndef GTKMMSESSIONWIDGET_SD2Z0QLJ
#define GTKMMSESSIONWIDGET_SD2Z0QLJ

#include <memory>
#include <gtkmm.h>
#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct GtkDownloadItem {
    static std::shared_ptr< GtkDownloadItem > makeNew();
    ~GtkDownloadItem();
    Gtk::Box* getMainBox();
private:
    GtkDownloadItem(Glib::RefPtr<Gtk::Builder>& bld);

    Glib::RefPtr<Gtk::Builder> _container;
    Gtk::Box* _mainBox;
    Gtk::Label* _theLabel;
    Gtk::ProgressBar* _theProgressBar;
};

struct GtkSessionWidget {

    static std::shared_ptr< GtkSessionWidget > makeNew();
    ~GtkSessionWidget();
    Gtk::Box* getMainBox();
    void setDownloadBoxCount(int number);
private:
    GtkSessionWidget(Glib::RefPtr<Gtk::Builder>& bld);
    Glib::RefPtr<Gtk::Builder> _container;
    std::vector< std::shared_ptr<GtkDownloadItem> > _dlItems;

    Gtk::Box* _mainBox;
    Gtk::ListBox* _sessionList;
    Gtk::Label* _sessionLabel;
};

struct GtkSessionTab : public Messageable {

    struct ModelInterface {
        // Query count of download sessions in model
        // Signature:
        // < QueryCount, int (out number) >
        DUMMY_REG(QueryCount,"DLMDL_QueryCount");

        // Query count of downloads for single session in model
        // Signature:
        // < QuerySessionDownloadCount, int (sess num), int (out count) >
        DUMMY_REG(QuerySessionDownloadCount,"DLMDL_QuerySessionDownloadCount");

        // Perform full gui update
        // Signature:
        // < InFullUpdate >
        DUMMY_REG(InFullUpdate,"DLMDL_InFullUpdate");
    };

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override;
    void message(templatious::VirtualPack& msg) override;

    static std::shared_ptr< GtkSessionTab > makeNew();
    Gtk::Notebook* getTabs();
    ~GtkSessionTab();
    void addSession(const std::shared_ptr<GtkSessionWidget>& widget);
    void setModel(const StrongMsgPtr& model);
    void fullModelUpdate();
private:
    GtkSessionTab(Glib::RefPtr<Gtk::Builder>& bld);

    VmfPtr genHandler();

    std::vector< std::shared_ptr<GtkSessionWidget > > _sessions;
    Glib::RefPtr<Gtk::Builder> _container;
    Gtk::Notebook* _mainTab;
    WeakMsgPtr _weakModel;
    VmfPtr _handler;
};

}

#endif /* end of include guard: GTKMMSESSIONWIDGET_SD2Z0QLJ */
