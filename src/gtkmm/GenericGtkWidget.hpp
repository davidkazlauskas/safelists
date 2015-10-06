#ifndef GENERICGTKWIDGET_EB02WCAA
#define GENERICGTKWIDGET_EB02WCAA

#include <util/GenericStMessageable.hpp>
#include <gtkmm.h>

namespace SafeLists {

struct GenericGtkWidget : public GenericStMessageable {
    GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder);

private:
    Glib::RefPtr< Gtk::Builder > _builder;
};

}

#endif /* end of include guard: GENERICGTKWIDGET_EB02WCAA */
