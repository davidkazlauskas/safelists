
#include <templatious/FullPack.hpp>
#include <glibmm/weakref.h>

#include "GenericGtkWidgetInterface.hpp"
#include "GenericGtkWidget.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef GenericGtkWidgetInterface GWI;

namespace SafeLists {

    struct GenericGtkLabel : public GenericStMessageable {

        GenericGtkLabel(
            const std::shared_ptr<int>& dummy,
            GenericGtkWidget* wgt
        ) : _weakRef(dummy), _raw(wgt)
        {}

    private:
        std::weak_ptr< int > _weakRef;
        GenericGtkWidget* _raw;
    };

    GenericGtkWidget::GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder)
        : _builder(builder), _int(std::make_shared<int>(7))
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
