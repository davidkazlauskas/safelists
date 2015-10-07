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
    WeakMsgPtr _toNotify;
    int _hookId;
};

struct GenericGtkWidget : public GenericStMessageable {
    GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder);

private:
    Glib::RefPtr< Gtk::Builder > _builder;
    std::shared_ptr< GenericGtkWidgetSharedState > _sharedState;
};

}

#endif /* end of include guard: GENERICGTKWIDGET_EB02WCAA */
