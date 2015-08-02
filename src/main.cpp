
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

    typedef Gtk::TreeModel::iterator Iterator;
    bool iter_next_vfunc(const Iterator& iter,Iterator& iterNext) const override {
        return false;
    }

    bool get_iter_vfunc(const Path& path,Iterator& iter) const override {
        return false;
    }

    bool iter_children_vfunc(const iterator& parent, iterator& iter) const override {
        return false;
    }

    bool iter_parent_vfunc(const iterator& child, iterator& iter) const override {
        return false;
    }

    bool iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const override {
        return false;
    }

    bool iter_nth_root_child_vfunc(int n, iterator& iter) const override {
        return false;
    }

    bool iter_has_child_vfunc(const iterator& iter) const override {
        return false;
    }

    int iter_n_children_vfunc(const iterator& iter) const override {
        return 0;
    }

    int iter_n_root_children_vfunc() const override {
        return 0;
    }

    void ref_node_vfunc(const iterator& iter) const override {
    }

    void unref_node_vfunc(const iterator& iter) const override {
    }

    TreeModel::Path	get_path_vfunc(const iterator& iter) const override {
        return TreeModel::Path();
    }

    void get_value_vfunc(const iterator& iter, int column, Glib::ValueBase& value) const override {
    }

    void set_value_impl(const iterator& row, int column, const Glib::ValueBase& value) override {
    }

    void get_value_impl(const iterator& row, int column, Glib::ValueBase& value) const override {
    }

    void on_row_changed(const TreeModel::Path& path, const TreeModel::iterator& iter) override {
    }

    void on_row_inserted(const TreeModel::Path& path, const TreeModel::iterator& iter) override {
    }

    void on_row_has_child_toggled(const TreeModel::Path& path, const TreeModel::iterator& iter) override {
    }

    void on_row_deleted(const TreeModel::Path& path) override {
    }

    void on_rows_reordered(const TreeModel::Path& path, const TreeModel::iterator& iter, int* new_order) override {
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

