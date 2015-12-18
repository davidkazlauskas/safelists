#ifndef GENERICGTKWIDGET_EB02WCAA
#define GENERICGTKWIDGET_EB02WCAA

#include <util/GenericStMessageable.hpp>
#include <util/AutoReg.hpp>
#include <gtkmm.h>

namespace SafeLists {

struct GenericGtkWidgetSharedState {
    GenericGtkWidgetSharedState() :
        _hookId(0)
    {}

    GenericGtkWidgetSharedState(const GenericGtkWidgetSharedState&) = delete;
    GenericGtkWidgetSharedState(GenericGtkWidgetSharedState&&) = delete;

    friend struct GenericGtkWidget;
    friend struct GenericGtkWidgetNode;

private:
    NotifierCache _cache;
    int _hookId;
};

struct GenericGtkWidget : public GenericStMessageableWCallbacks {
    GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder,const char* rootName);

    void addToNotify(const StrongMsgPtr& ptr);
protected:
    void notifyObservers(templatious::VirtualPack& msg);
private:
    Glib::RefPtr< Gtk::Builder > _builder;
    std::shared_ptr< GenericGtkWidgetSharedState > _sharedState;
    std::string _rootWidgetName;
    bool _onDrawHooked;
};

struct GenericGtkWidgetNodePrivateWindow {
    // Query Gtk::Window*
    // Signature:
    // < QueryWindow, Gtk::Window* (out) >
    DUMMY_STRUCT_NATIVE(QueryWindow);
};

}

#endif /* end of include guard: GENERICGTKWIDGET_EB02WCAA */
