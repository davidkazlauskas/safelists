
#include <templatious/FullPack.hpp>
#include <gtkmm.h>

#include "GenericGtkWidgetInterface.hpp"
#include "GenericGtkWidget.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef SafeLists::GenericGtkWidgetInterface GWI;

namespace SafeLists {

    struct GenericGtkWidgetNode : public GenericStMessageable {
        typedef GenericWidgetTrait GWT;
        typedef GenericLabelTrait GLT;
        typedef GenericInputTrait GIT;
        typedef GenericButtonTrait GBT;

        GenericGtkWidgetNode(
            const std::shared_ptr<GenericGtkWidgetSharedState>& sharedState,
            Gtk::Widget* wgt
        ) : _weakRef(sharedState), _myWidget(wgt)
        {
            regHandler(
                SF::virtualMatchFunctorPtr(
                    SF::virtualMatch< GWT::SetActive, const bool >(
                        [=](ANY_CONV,bool val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );
                            _myWidget->set_sensitive(val);
                        }
                    ),
                    SF::virtualMatch< GLT::SetValue, const std::string >(
                        [=](ANY_CONV,const std::string& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Label* cast = dynamic_cast< Gtk::Label* >(_myWidget);
                            assert( nullptr != cast && "Cast to label failed." );

                            cast->set_text(val.c_str());
                        }
                    ),
                    SF::virtualMatch< GIT::QueryValue, std::string >(
                        [=](ANY_CONV,std::string& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Entry* cast = dynamic_cast< Gtk::Entry* >(_myWidget);
                            assert( nullptr != cast && "Cast to label failed." );

                            val = cast->get_text();
                        }
                    ),
                    SF::virtualMatch< GIT::SetValue, const std::string >(
                        [=](ANY_CONV,const std::string& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Entry* cast = dynamic_cast< Gtk::Entry* >(_myWidget);
                            assert( nullptr != cast && "Cast to label failed." );

                            cast->set_text(val.c_str());
                        }
                    ),
                    SF::virtualMatch< GBT::HookClickEvent, int >(
                        [=](ANY_CONV,int& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Button* cast = dynamic_cast< Gtk::Button* >(_myWidget);
                            assert( nullptr != cast && "Cast to label failed." );

                            int outNum = locked->_hookId;
                            ++locked->_hookId;

                            cast->signal_clicked().connect(
                                sigc::bind< int >(
                                    sigc::mem_fun(*this,
                                        &GenericGtkWidgetNode::clicked),
                                    outNum
                                )
                            );

                            val = outNum;
                        }
                    )
                )
            );
        }

    private:

        void clicked(int num) {
            auto locked = _weakRef.lock();
            assert( nullptr != locked && "Dead father?" );
            auto lockedMsg = locked->_toNotify.lock();
            if (nullptr != lockedMsg) {
                auto msg = SF::vpack< GBT::OutClickEvent, int >(
                    nullptr, num
                );
                lockedMsg->message(msg);
            }
        }

        std::weak_ptr< GenericGtkWidgetSharedState > _weakRef;
        Gtk::Widget* _myWidget;
    };

    GenericGtkWidget::GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder)
        : _builder(builder), _sharedState(std::make_shared< GenericGtkWidgetSharedState >())
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

                        auto result = std::make_shared< GenericGtkWidgetNode >(_sharedState,wgt);
                        out = result;
                    }
                )
            )
        );
    }

}
