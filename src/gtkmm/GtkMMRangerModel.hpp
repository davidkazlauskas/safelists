
#ifndef GTKMMRANGERMODEL_NV9C9SC1
#define GTKMMRANGERMODEL_NV9C9SC1

#include <gtkmm.h>
#include <model/SqliteRanger.hpp>

namespace SafeLists {

class RangerTreeModel : public Glib::Object, public Gtk::TreeModel {
protected:
    // Create a TreeModel with @a columns_count number of columns, each of type
    // Glib::ustring.
    RangerTreeModel(unsigned int columns_count = 10);
    virtual ~RangerTreeModel();

public:
    static Glib::RefPtr<RangerTreeModel> create();

    Gtk::TreeModelColumn<Glib::ustring>& get_model_column(int column);

    // screwSnakeCase
    void setRanger(std::unique_ptr< SqliteRanger >&& ranger);

protected:
    // Overrides:
    virtual Gtk::TreeModelFlags get_flags_vfunc() const;
    virtual int get_n_columns_vfunc() const;
    virtual GType get_column_type_vfunc(int index) const;
    virtual void get_value_vfunc(const TreeModel::iterator& iter, int column,
                                 Glib::ValueBase& value) const;

    bool iter_next_vfunc(const iterator& iter, iterator& iter_next) const;

    // TODO: Make sure that we make all of these const when we have made them
    // all const in the TreeModel:
    virtual bool iter_children_vfunc(const iterator& parent,
                                     iterator& iter) const;
    virtual bool iter_has_child_vfunc(const iterator& iter) const;
    virtual int iter_n_children_vfunc(const iterator& iter) const;
    virtual int iter_n_root_children_vfunc() const;
    virtual bool iter_nth_child_vfunc(const iterator& parent, int n,
                                      iterator& iter) const;
    virtual bool iter_nth_root_child_vfunc(int n, iterator& iter) const;
    virtual bool iter_parent_vfunc(const iterator& child, iterator& iter) const;
    virtual Path get_path_vfunc(const iterator& iter) const;
    virtual bool get_iter_vfunc(const Path& path, iterator& iter) const;

private:
    typedef std::vector<Glib::ustring>
        typeRow;  // X columns, all of type string.
    typedef std::vector<typeRow> typeListOfRows;  // Y rows.
    std::unique_ptr< SqliteRanger > _ranger;

    // Allow the GlueList inner class to access the declaration of the GlueItem
    // inner class.
    // SUN's Forte compiler complains about this.

    int get_data_row_iter_from_tree_row_iter(const iterator& iter) const;
    bool check_treeiter_validity(const iterator& iter) const;

    // Column information:
    ColumnRecord m_column_record;

    typedef Gtk::TreeModelColumn<Glib::ustring> typeModelColumn;
    // Usually you would have different types for each column -
    // then you would want a vector of pointers to the model columns.
    typedef std::vector<typeModelColumn> typeListOfModelColumns;
    typeListOfModelColumns m_listModelColumns;

    int m_stamp;  // When the model's stamp and the TreeIter's stamp are equal,
                  // the TreeIter is valid.
};

}

#endif /* end of include guard: GTKMMRANGERMODEL_NV9C9SC1 */
