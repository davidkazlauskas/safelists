
#include <gtkmm.h>
#include <templatious/FullPack.hpp>
#include <LuaPlumbing/plumbing.hpp>

struct MyModel : public Gtk::TreeModel {

    Gtk::TreeModelFlags get_flags_vfunc() const override {
        return Gtk::TREE_MODEL_LIST_ONLY;
    }

    int get_n_columns_vfunc() const override {
        return 2;
    }

    GType get_column_type_vfunc(int index) const override {
        return G_TYPE_STRING;
    }

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

        _left->append_column_editable("cholo",_mdl.m_col_id);
        _left->append_column_editable("holo",_mdl.m_col_name);
    }

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
            add(m_col_id);
            add(m_col_name);
            add(m_col_foo);
            add(m_col_number);
            add(m_col_number_validated);
        }

        Gtk::TreeModelColumn<unsigned int> m_col_id;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<bool> m_col_foo;
        Gtk::TreeModelColumn<int> m_col_number;
        Gtk::TreeModelColumn<int> m_col_number_validated;
    };

    Gtk::Window& getWindow() {
        return *_wnd;
    }

    void message(templatious::VirtualPack& msg) override {
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
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
    auto mainWnd = std::make_shared< GtkMainWindow >(builder);
    app->run(mainWnd->getWindow(),argc,argv);
}

