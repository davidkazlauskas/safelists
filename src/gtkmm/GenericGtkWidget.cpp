
#include <templatious/FullPack.hpp>
#include <gtkmm.h>

#include "GenericGtkWidgetInterface.hpp"
#include "GenericGtkWidget.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef GenericGtkWidgetInterface GWI;

namespace SafeLists {

    struct GenericGtkLabel : public GenericStMessageable {

        GenericGtkLabel(
            const std::shared_ptr<int>& dummy,
            Gtk::Widget* wgt
        ) : _weakRef(dummy), _myWidget(wgt)
        {}

    private:
        std::weak_ptr< int > _weakRef;
        Gtk::Widget* _myWidget;
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
                        Gtk::Widget* wgt = nullptr;
                        _builder->get_widget(str.c_str(),wgt);
                        assert( wgt != nullptr && "Could not get widget." );

                        auto result = std::make_shared< GenericGtkLabel >(_int,wgt);
                        out = result;
                    }
                )
            )
        );
    }

}
