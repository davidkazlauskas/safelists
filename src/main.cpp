
#include <iostream>
#include <gtkmm.h>
#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicPackCreator.hpp>
#include <LuaPlumbing/plumbing.hpp>
#include <gtkmm/GtkMMRangerModel.hpp>

TEMPLATIOUS_TRIPLET_STD;

struct MainWindowInterface {
    // emitted when new file creation
    // is requested.
    // In lua: MWI_OutNewFileSignal
    DUMMY_STRUCT(OutNewFileSignal);

    // emit to attach listener
    // In lua: MWI_InAttachListener
    // Signature: < InAttachListener, StrongMsgPtr >
    DUMMY_STRUCT(InAttachListener);

    // emit to set tree model from snapshot
    // Signature: < InSetTreeData, TableSnapshot >
    DUMMY_STRUCT(InSetTreeData);

    static void registerInFactory(templatious::DynVPackFactoryBuilder& bld);
};

#define ASYNC_OUT_SNAP_SIGNATURE \
    ASql::ExecuteOutSnapshot, \
    std::string, \
    std::vector< std::string >, \
    TableSnapshot

struct MainModel : public Messageable {

    struct MainModelInterface {

        // use to load folder tree
        // Signature: <
        //     InLoadFolderTree,
        //     StrongMsgPtr (async sqlite),
        //     StrongMsgPtr (notify)
        // >
        DUMMY_STRUCT(InLoadFolderTree);

        static void registerInFactory(templatious::DynVPackFactoryBuilder& bld);
    };

    MainModel() : _messageHandler(genHandler()) {}

    void message(templatious::VirtualPack& msg) override {
        _messageHandler->tryMatch(msg);
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "You are not prepared." );
    }

private:
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        typedef MainModelInterface MMI;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< MMI::InLoadFolderTree,
                              StrongMsgPtr,
                              StrongMsgPtr >
            ([=](
                MMI::InLoadFolderTree,
                const StrongMsgPtr& asyncSqlite,
                const StrongMsgPtr& toNotify)
            {
                typedef SafeLists::AsyncSqlite ASql;
                std::vector< std::string > headers({"id","name","parent"});
                std::weak_ptr< Messageable > weakNotify = toNotify;
                auto message = SF::vpackPtrWCallback<
                    ASYNC_OUT_SNAP_SIGNATURE
                >(
                    [=](const TEMPLATIOUS_VPCORE< ASYNC_OUT_SNAP_SIGNATURE >& sig) {
                        auto locked = weakNotify.lock();
                        if (nullptr == locked && sig.fGet<3>().isEmpty()) {
                            return;
                        }

                        auto& snapref = const_cast< TableSnapshot& >(sig.fGet<3>());

                        typedef MainWindowInterface MWI;
                        auto outMsg = SF::vpackPtr< MWI::InSetTreeData, TableSnapshot >(
                            nullptr, std::move(snapref)
                        );
                        locked->message(outMsg);
                    },
                    nullptr,"SELECT dir_id, dir_name, dir_parent FROM directories;",
                    std::move(headers),TableSnapshot());

                asyncSqlite->message(message);
            })
        );
    }

    VmfPtr _messageHandler;
};

struct GtkMainWindow : public Messageable {

    GtkMainWindow(Glib::RefPtr<Gtk::Builder>& bld) :
        _left(nullptr),
        _right(nullptr),
        _messageHandler(genHandler())
    {
        Gtk::Window* outWnd = nullptr;
        bld->get_widget("window1",outWnd);
        _wnd.reset( outWnd );

        bld->get_widget("treeview1",_right);
        bld->get_widget("treeview3",_left);
        bld->get_widget("addNewBtn",_addNewBtn);

        auto mdl = SafeLists::RangerTreeModel::create();

        _addNewBtn->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkMainWindow::addNewButtonClicked)
        );

        _wnd->signal_draw().connect(
            sigc::mem_fun(*this,&GtkMainWindow::onDraw)
        );
    }

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
            add(m_col_name);
            add(m_col_size);
            add(m_col_hash);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<int> m_col_size;
        Gtk::TreeModelColumn<Glib::ustring> m_col_hash;
    };

    Gtk::Window& getWindow() {
        return *_wnd;
    }

    void message(templatious::VirtualPack& msg) override {
        _messageHandler->tryMatch(msg);
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "You are not prepared." );
    }

    void initModel(const std::shared_ptr< Messageable >& asyncSqlite) {
        auto shared = SafeLists::SqliteRanger::makeRanger(
            asyncSqlite,
            "SELECT file_name,file_size,file_hash_sha256 FROM files LIMIT %d OFFSET %d;",
            "SELECT COUNT(*) FROM files;",
            3,
            [](int row,int col,const char* value) {
                //std::cout << "PULLED OUT: " << row << ":" << col << " " << value << std::endl;
            },
            [](int row,int col,std::string& str) {
                str = "Loading...";
            }
        );
        shared->updateRows();
        shared->setRange(0,256*256);
        shared->waitRows();

        auto mdl = SafeLists::RangerTreeModel::create();
        mdl->setRanger(shared);

        const char* columns[] = {"one","two","three"};
        _right->set_model(mdl);
        mdl->appendColumns(*_right,columns);
    }

private:

    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        typedef MainWindowInterface MWI;
        typedef GenericMesseagableInterface GMI;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< MWI::InAttachListener, StrongMsgPtr >(
                [=](MWI::InAttachListener,const StrongMsgPtr& ptr) {
                    this->_notifierCache.add(ptr);
                }
            ),
            SF::virtualMatch<
                GMI::InAttachToEventLoop, std::function<bool()>
            >(
                [=](GMI::InAttachToEventLoop,std::function<bool()>& func) {
                    this->_callbackCache.attach(func);
                }
            ),
            SF::virtualMatch<
                MWI::InSetTreeData, TableSnapshot
            >(
                [=](MWI::InSetTreeData,TableSnapshot& snapshot) {
                    printf("RECEIVED!\n");
                }
            )
        );
    }

    bool onDraw(const Cairo::RefPtr<Cairo::Context>& cr) {
        _callbackCache.process();
        return false;
    }

    void addNewButtonClicked() {
        auto msg = SF::vpack< MainWindowInterface::OutNewFileSignal >(nullptr);
        _notifierCache.notify(msg);
    }

    std::unique_ptr< Gtk::Window > _wnd;
    Gtk::TreeView* _left;
    Gtk::TreeView* _right;
    Gtk::Button* _addNewBtn;
    ModelColumns _mdl;

    VmfPtr _messageHandler;

    NotifierCache _notifierCache;
    CallbackCache _callbackCache;
};

struct GtkNewEntryDialog : public Messageable {
    void message(templatious::VirtualPack& msg) override {

    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "Single threaded messaging only." );
    }
};

templatious::DynVPackFactory makeVfactory() {
    templatious::DynVPackFactoryBuilder bld;

    LuaContext::registerPrimitives(bld);

    MainWindowInterface::registerInFactory(bld);
    MainModel::MainModelInterface::registerInFactory(bld);

    return bld.getFactory();
}

static templatious::DynVPackFactory* vFactory() {
    static auto fact = makeVfactory();
    return &fact;
}

int main(int argc,char** argv) {
    auto app = Gtk::Application::create(argc,argv);

    auto ctx = LuaContext::makeContext("lua/plumbing.lua");
    ctx->setFactory(vFactory());

    auto builder = Gtk::Builder::create();
    builder->add_from_file("uischemes/main.glade");
    auto asyncSqlite = SafeLists::AsyncSqlite::createNew("exampleData/example2.safelist");
    auto mainWnd = std::make_shared< GtkMainWindow >(builder);
    auto mainModel = std::make_shared< MainModel >();
    mainWnd->initModel(asyncSqlite);
    ctx->addMesseagableWeak("mainWindow",mainWnd);
    ctx->addMesseagableWeak("mainModel",mainModel);
    ctx->addMesseagableWeak("asyncSqliteCurrent",asyncSqlite);
    ctx->doFile("lua/main.lua");
    app->run(mainWnd->getWindow(),argc,argv);
}

typedef templatious::TypeNodeFactory TNF;

#define ATTACH_NAMED_DUMMY(factory,name,type)   \
    factory.attachNode(name,TNF::makeDummyNode< type >(name))

void MainWindowInterface::registerInFactory(templatious::DynVPackFactoryBuilder& bld) {
    typedef MainWindowInterface MWI;
    ATTACH_NAMED_DUMMY(bld,"MWI_OutNewFileSignal",MWI::OutNewFileSignal);
    ATTACH_NAMED_DUMMY(bld,"MWI_InAttachListener",MWI::InAttachListener);
}

void MainModel::MainModelInterface::registerInFactory(templatious::DynVPackFactoryBuilder& bld) {
    typedef MainModel::MainModelInterface MMI;
    ATTACH_NAMED_DUMMY(bld,"MMI_InLoadFolderTree",MMI::InLoadFolderTree);
}
