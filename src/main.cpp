
#include <gtkmm.h>
#include <templatious/FullPack.hpp>
#include <LuaPlumbing/plumbing.hpp>
#include <gtkmm/GtkMMRangerModel.hpp>

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

        //_left->append_column_editable("cholo",_mdl.m_col_id);
        //_left->append_column_editable("holo",_mdl.m_col_name);

        auto mdl = SafeLists::RangerTreeModel::create();


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
        std::unique_ptr< SafeLists::SqliteRanger > ranger(
            new SafeLists::SqliteRanger(
                asyncSqlite,
                "SELECT file_name,file_size,file_hash FROM files LIMIT %d OFFSET %d;",
                3,
                [](int row,int col,const char* value) {

                },
                [](int row,int col,std::string& str) {

                }
            )
        );
        auto mdl = SafeLists::RangerTreeModel::create();
        mdl->setRanger(std::move(ranger));

        Gtk::TreePath start,end;

        _right->append_column("one",mdl->get_model_column(0));
        _right->append_column("two",mdl->get_model_column(1));
        _right->append_column("three",mdl->get_model_column(2));
        _right->set_model(mdl);
        _right->get_visible_range(start,end);
        auto startI = mdl->get_iter(start);
        auto endI = mdl->get_iter(end);

        int rangeStart = mdl->iterToRow(startI);
        int rangeEnd = mdl->iterToRow(endI);

        printf("<-- %d %d -->",rangeStart,rangeEnd);
    }

private:
    std::unique_ptr< Gtk::Window > _wnd;
    Gtk::TreeView* _left;
    Gtk::TreeView* _right;
    ModelColumns _mdl;
};

int main(int argc,char** argv) {
    auto app = Gtk::Application::create(argc,argv);

    auto builder = Gtk::Builder::create();
    builder->add_from_file("uischemes/main.glade");
    auto asyncSqlite = SafeLists::AsyncSqlite::createNew("exampleData/example2.safelist");
    auto mainWnd = std::make_shared< GtkMainWindow >(builder);
    mainWnd->initModel(asyncSqlite);
    app->run(mainWnd->getWindow(),argc,argv);
}

