
#include <templatious/FullPack.hpp>
#include <gtkmm.h>

#include "GenericGtkWidgetInterface.hpp"
#include "GenericGtkWidget.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef SafeLists::GenericGtkWidgetInterface GWI;

namespace SafeLists {

    typedef GenericMenuBarTrait GMBT;
    void setupMenuBarWithModel(Gtk::MenuShell& bar,const StrongMsgPtr& model);

    struct GenericGtkWidgetNode : public GenericStMessageable {
        typedef GenericWidgetTrait GWT;
        typedef GenericLabelTrait GLT;
        typedef GenericInputTrait GIT;
        typedef GenericButtonTrait GBT;
        typedef GenericNotebookTrait GNT;
        typedef GenericWindowTrait GWNT;

        typedef GenericGtkWidgetNodePrivateWindow PR_GWNT;

        GenericGtkWidgetNode(
            const std::shared_ptr<GenericGtkWidgetSharedState>& sharedState,
            Gtk::Widget* wgt
        ) : _weakRef(sharedState), _myWidget(wgt)
        {
            _myInheritanceLevel = regHandler(
                SF::virtualMatchFunctorPtr(
                    SF::virtualMatch< GWT::SetActive, const bool >(
                        [=](ANY_CONV,bool val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );
                            _myWidget->set_sensitive(val);
                        }
                    ),
                    SF::virtualMatch< GWT::SetVisible, const bool >(
                        [=](ANY_CONV,bool val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );
                            _myWidget->set_visible(val);
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
                    SF::virtualMatch< GIT::HookTextChangedEvent, int >(
                        [=](ANY_CONV,int& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Entry* cast = dynamic_cast< Gtk::Entry* >(_myWidget);
                            assert( nullptr != cast && "Cast to label failed." );

                            int outNum = locked->_hookId;
                            ++locked->_hookId;

                            cast->signal_changed().connect(
                                sigc::bind< int >(
                                    sigc::mem_fun(*this,
                                        &GenericGtkWidgetNode::editableChanged),
                                    outNum
                                )
                            );

                            val = outNum;
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
                    ),
                    SF::virtualMatch< GBT::SetButtonText, const std::string >(
                        [=](ANY_CONV,const std::string& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Button* cast = dynamic_cast< Gtk::Button* >(_myWidget);
                            assert( nullptr != cast && "Cast to label failed." );

                            cast->set_label(val.c_str());
                        }
                    ),
                    SF::virtualMatch< GNT::SetCurrentTab, const int >(
                        [=](ANY_CONV,int val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Notebook* cast = dynamic_cast< Gtk::Notebook* >(_myWidget);
                            assert( nullptr != cast && "Cast to label failed." );

                            cast->set_current_page(val);
                        }
                    ),
                    SF::virtualMatch< GWNT::SetWindowPosition, const std::string >(
                        [=](ANY_CONV,const std::string& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Window* cast = dynamic_cast< Gtk::Window* >(_myWidget);
                            assert( nullptr != cast && "Cast to window failed." );

                            Gtk::WindowPosition pos = Gtk::WindowPosition::WIN_POS_NONE;
                            if (val == "WIN_POS_CENTER") {
                                pos = Gtk::WindowPosition::WIN_POS_CENTER;
                            } else if (val == "WIN_POS_MOUSE") {
                                pos = Gtk::WindowPosition::WIN_POS_MOUSE;
                            } else if (val == "WIN_POS_CENTER_ALWAYS") {
                                pos = Gtk::WindowPosition::WIN_POS_CENTER_ALWAYS;
                            } else if (val == "WIN_POS_CENTER_ON_PARENT") {
                                pos = Gtk::WindowPosition::WIN_POS_CENTER_ON_PARENT;
                            } else if (val != "WIN_POS_NONE") {
                                assert( false && "Unknown window position option." );
                            }

                            cast->set_position(pos);
                        }
                    ),
                    SF::virtualMatch< GWNT::SetWindowTitle, const std::string >(
                        [=](ANY_CONV,const std::string& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Window* cast = dynamic_cast< Gtk::Window* >(_myWidget);
                            assert( nullptr != cast && "Cast to window failed." );

                            cast->set_title(val.c_str());
                        }
                    ),
                    SF::virtualMatch< GWNT::SetWindowParent, const StrongMsgPtr >(
                        [=](ANY_CONV,const StrongMsgPtr& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Window* cast = dynamic_cast< Gtk::Window* >(_myWidget);
                            assert( nullptr != cast && "Cast to window failed." );

                            auto query = SF::vpack< PR_GWNT::QueryWindow, Gtk::Window* >(
                                nullptr, nullptr
                            );
                            val->message(query);
                            assert( query.useCount() > 0 && "Query pack wasn't used." );

                            auto ptr = query.fGet<1>();
                            assert( nullptr != ptr && "Could not query window." );

                            cast->set_parent(*ptr);
                        }
                    ),
                    SF::virtualMatch< PR_GWNT::QueryWindow, Gtk::Window* >(
                        [=](ANY_CONV,Gtk::Window*& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::Window* cast = dynamic_cast< Gtk::Window* >(_myWidget);
                            assert( nullptr != cast && "Cast to window failed." );

                            val = cast;
                        }
                    ),
                    SF::virtualMatch< GMBT::SetModelStackless, StrongMsgPtr >(
                        [=](ANY_CONV,const StrongMsgPtr& val) {
                            auto locked = _weakRef.lock();
                            assert( nullptr != locked && "Parent object dead." );

                            Gtk::MenuBar* cast = dynamic_cast< Gtk::MenuBar* >(_myWidget);
                            assert( nullptr != cast && "Cast to MenuBar failed." );

                            setupMenuBarWithModel(*cast,val);
                        }
                    )
                )
            );
        }

    private:

        template <class... Args,class... Raw>
        void notifySingleThreaded(Raw&&... args) {
            auto locked = _weakRef.lock();
            assert( nullptr != locked && "Dead father?" );
            auto msg = SF::vpack< Args... >(
                std::forward<Raw>(args)...);
            locked->_cache.notify(msg);
        }

        void clicked(int num) {
            notifySingleThreaded< GBT::OutClickEvent, int >(
                nullptr, num
            );
        }

        void editableChanged(int num) {
            notifySingleThreaded< GIT::OutValueChanged, int >(
                nullptr, num
            );
        }

        std::weak_ptr< GenericGtkWidgetSharedState > _weakRef;
        Gtk::Widget* _myWidget;
        int _myInheritanceLevel;
    };

    void GenericGtkWidget::addToNotify(const StrongMsgPtr& ptr) {
        _sharedState->_cache.add(ptr);
    }

    void GenericGtkWidget::notifyObservers(templatious::VirtualPack& msg) {
        _sharedState->_cache.notify(msg);
    }

    GenericGtkWidget::GenericGtkWidget(Glib::RefPtr< Gtk::Builder >& builder,const char* rootName)
        : _builder(builder),
          _sharedState(std::make_shared< GenericGtkWidgetSharedState >()),
          _rootWidgetName(rootName),
          _onDrawHooked(false)
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
                ),
                SF::virtualMatch< GWI::SetNotifier, const StrongMsgPtr >(
                    [=](ANY_CONV,const StrongMsgPtr& res) {
                        this->_sharedState->_cache.add(res);
                    }
                )
            )
        );
    }

    void clearMenuBar(Gtk::MenuShell& bar) {
        auto children = bar.get_children();
        TEMPLATIOUS_FOREACH(auto& i,children) {
            bar.remove(*i);
        }
    }

    void onMenuClick(int i,WeakMsgPtr msgPtr) {
        auto locked = msgPtr.lock();
        if (nullptr != locked) {
            auto msg = SF::vpack< GMBT::OutIndexClicked, int >(
                nullptr, i
            );
            locked->message(msg);
        }
    }

    // returns whether iteration should stop
    void setupSingleMenu(Gtk::MenuShell& bar,const StrongMsgPtr& model) {
        auto queryNext = SF::vpack<
            GMBT::QueryNextNode,
            int, std::string, std::string
        >(
            nullptr, 0, "", ""
        );

        model->message(queryNext);
        while (queryNext.fGet<1>() != -1) {
            //printf("Queried: |%d|%s|%s|\n",
                //queryNext.fGet<1>(),
                //queryNext.fGet<2>().c_str(),
                //queryNext.fGet<3>().c_str());

            auto managed = Gtk::manage(
                new Gtk::MenuItem(queryNext.fGet<3>().c_str(),true));

            int copy = queryNext.fGet<1>();

            // go back one level
            if (copy == -3) {
                return;
            } else if (copy != -2) {
                WeakMsgPtr weak = model;
                managed->signal_activate().connect(
                    sigc::bind<1>(
                        sigc::bind<0>(
                            sigc::ptr_fun2(&onMenuClick)
                        ,copy)
                    ,weak)
                );
            } else {
                auto managedMenu = Gtk::manage(
                    new Gtk::Menu());
                managed->set_submenu(*managedMenu);
                setupSingleMenu(*managedMenu,model);
            }
            bar.append(*managed);

            model->message(queryNext);
        }
    }

    void setupMenuBarWithModel(Gtk::MenuShell& bar,const StrongMsgPtr& model) {
        clearMenuBar(bar);

        setupSingleMenu(bar,model);

        bar.show_all();
    }

}
