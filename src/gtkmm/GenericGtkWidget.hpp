#ifndef GENERICGTKWIDGET_EB02WCAA
#define GENERICGTKWIDGET_EB02WCAA

#include <util/GenericStMessageable.hpp>
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
    GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder);

    void addToNotify(const StrongMsgPtr& ptr);
protected:
    void notifyObservers(templatious::VirtualPack& msg);
private:
    Glib::RefPtr< Gtk::Builder > _builder;
    std::shared_ptr< GenericGtkWidgetSharedState > _sharedState;
};

}

#endif /* end of include guard: GENERICGTKWIDGET_EB02WCAA */
