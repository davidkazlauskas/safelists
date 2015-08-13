
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

    static void registerInFactory(templatious::DynVPackFactoryBuilder& bld);
};

struct GtkMainWindow : public Messageable {

    GtkMainWindow(Glib::RefPtr<Gtk::Builder>& bld) :
        _left(nullptr),
        _right(nullptr)
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
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
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

    void addNewButtonClicked() {
        auto msg = SF::vpack< MainWindowInterface::OutNewFileSignal >(nullptr);
        _cache.notify(msg);
    }

    std::unique_ptr< Gtk::Window > _wnd;
    Gtk::TreeView* _left;
    Gtk::TreeView* _right;
    Gtk::Button* _addNewBtn;
    ModelColumns _mdl;

    NotifierCache _cache;
};

struct GtkNewEntryDialog : public Messageable {
    void message(templatious::VirtualPack& msg) override {

    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "Single threaded messaging only." );
    }
};

static void registerInFactory(templatious::DynVPackFactoryBuilder& bld) {

}

templatious::DynVPackFactory makeVfactory() {
    templatious::DynVPackFactoryBuilder bld;

    LuaContext::registerPrimitives(bld);

    MainWindowInterface::registerInFactory(bld);

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
    mainWnd->initModel(asyncSqlite);
    ctx->doFile("lua/main.lua");
    app->run(mainWnd->getWindow(),argc,argv);
}

typedef templatious::TypeNodeFactory TNF;

#define ATTACH_NAMED_DUMMY(factory,name,type)   \
    factory.attachNode(name,TNF::makeDummyNode< type >(name))

void MainWindowInterface::registerInFactory(templatious::DynVPackFactoryBuilder& bld) {
    typedef MainWindowInterface MWI;
    ATTACH_NAMED_DUMMY(bld,"MWI_OutNewFileSignal",MWI::OutNewFileSignal);
}
