
#include <templatious/FullPack.hpp>
#include <glibmm/weakref.h>

#include "GenericGtkWidgetInterface.hpp"
#include "GenericGtkWidget.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef GenericGtkWidgetInterface GWI;

namespace SafeLists {

    struct GenericGtkLabel : public GenericStMessageable {

    private:
        std::weak_ptr< GenericGtkWidget > _weakParent;
    };

    GenericGtkWidget::GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder)
        : _builder(builder)
    {
        regHandler(
            SF::virtualMatchFunctorPtr(
                SF::virtualMatch<
                    GWI::GetWidgetFromTree,
                    const std::string,
                    StrongMsgPtr
                >(
                    [=](ANY_CONV,const std::string& str,StrongMsgPtr& out) {

                    }
                )
            )
        );
    }

}
