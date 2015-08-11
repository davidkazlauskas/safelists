
#include <iostream>
#include <templatious/FullPack.hpp>

#include "GtkMMRangerModel.hpp"

namespace SafeLists {

static_assert(sizeof(int) <= sizeof(void*),
              "We kinda assume that we can store int in void pointer, bro...");

const int LA_MAGICKA_ROWS = 1000;
const int LA_MAGICKA_COLUMNS = 3;

RangerTreeModel::RangerTreeModel(unsigned int columns_count)
    : Glib::ObjectBase(typeid(RangerTreeModel)),  // register a custom GType.
      Glib::Object(),  // The custom GType is actually registered here.
      m_stamp(1)  // When the model's stamp != the iterator's stamp then that
                  // iterator is invalid and should be ignored. Also, 0=invalid
{
    // The Column information that can be used with TreeView::append(),
    // TreeModel::iterator[], etc.
    m_listModelColumns.resize(columns_count);
    for (unsigned int column_number = 0; column_number < columns_count;
         ++column_number) {
        m_column_record.add(m_listModelColumns[column_number]);
    }
}

RangerTreeModel::~RangerTreeModel() {}

Glib::RefPtr<RangerTreeModel> RangerTreeModel::create() {
    return Glib::RefPtr<RangerTreeModel>(new RangerTreeModel);
}

Gtk::TreeModelFlags RangerTreeModel::get_flags_vfunc() const {
    return Gtk::TreeModelFlags(0);
}

int RangerTreeModel::get_n_columns_vfunc() const {
    return LA_MAGICKA_COLUMNS;
}

GType RangerTreeModel::get_column_type_vfunc(int index) const {
    if (index <= (int)m_listModelColumns.size())
        return m_listModelColumns[index].type();
    else
        return 0;
}

void RangerTreeModel::get_value_vfunc(const TreeModel::iterator& iter,
                                       int column,
                                       Glib::ValueBase& value) const {
    if (check_treeiter_validity(iter)) {
        if (column <= (int)m_listModelColumns.size()) {
            // Get the correct ValueType from the Gtk::TreeModel::Column's type,
            // so we don't have to repeat it here:
            typeModelColumn::ValueType value_specific;
            value_specific.init(
                typeModelColumn::ValueType::value_type());  // TODO: Is there
                                                            // any way to avoid
                                                            // this step?

            // Or, instead of asking the compiler for the TreeModelColumn's
            // ValueType:
            // Glib::Value< Glib::ustring > value_specific;
            // value_specific.init( Glib::Value< Glib::ustring >::value_type()
            // ); //TODO: Is there any way to avoid this step?

            auto num = get_data_row_iter_from_tree_row_iter(iter);
            if (num != LA_MAGICKA_ROWS) {
                _ranger->process();
                std::string out;
                _ranger->getData(num,column,out);

                value_specific.set(out.c_str());  // The compiler would complain if
                                             // the type was wrong.
                value.init(
                    Glib::Value<Glib::ustring>::value_type());  // TODO: Is
                                                                // there any way
                                                                // to avoid this
                                                                // step? Can't
                                                                // it copy the
                                                                // type as well
                                                                // as the value?
                value = value_specific;
            }
        }
    }
}

bool RangerTreeModel::iter_next_vfunc(const iterator& iter,
                                       iterator& iter_next) const {
    if (check_treeiter_validity(iter)) {
        // initialize the iterator:
        iter_next = iterator();
        iter_next.set_stamp(m_stamp);

        // Get the current row:
        void* udata = iter.gobj()->user_data;
        int row_index = *reinterpret_cast<int*>(&udata);

        // Make the iter_next GtkTreeIter represent the next row:
        row_index++;
        if (row_index < LA_MAGICKA_ROWS) {
            iter_next.gobj()->user_data = reinterpret_cast<void*>(row_index);

            return true;  // success
        }
    } else
        iter_next = iterator();  // Set is as invalid, as the TreeModel
                                 // documentation says that it should be.

    return false;  // There is no next row.
}

bool RangerTreeModel::iter_children_vfunc(const iterator& parent,
                                           iterator& iter) const {
    return iter_nth_child_vfunc(parent, 0, iter);
}

bool RangerTreeModel::iter_has_child_vfunc(const iterator& iter) const {
    return (iter_n_children_vfunc(iter) > 0);
}

int RangerTreeModel::iter_n_children_vfunc(const iterator& iter) const {
    if (!check_treeiter_validity(iter)) return 0;

    return 0;  // There are no children
}

int RangerTreeModel::iter_n_root_children_vfunc() const {
    int rows = _ranger->numRows();
    return rows;
}

bool RangerTreeModel::iter_nth_child_vfunc(const iterator& parent, int /* n */,
                                            iterator& iter) const {
    if (!check_treeiter_validity(parent)) {
        iter = iterator();  // Set is as invalid, as the TreeModel documentation
                            // says that it should be.
        return false;
    }

    iter = iterator();  // Set is as invalid, as the TreeModel documentation
                        // says that it should be.
    return false;  // There are no children.
}

bool RangerTreeModel::iter_nth_root_child_vfunc(int n, iterator& iter) const {
    if (n < LA_MAGICKA_ROWS) {
        iter = iterator();  // clear the input parameter.
        iter.set_stamp(m_stamp);

        // Store the row_index in the GtkTreeIter:
        // See also iter_next_vfunc()

        int row_index = n;

        // This will be deleted in the GlueList destructor, when old iterators
        // are marked as invalid.
        iter.gobj()->user_data = reinterpret_cast<void*>(row_index);
        return true;
    }

    return false;  // There are no children.
}

bool RangerTreeModel::iter_parent_vfunc(const iterator& child,
                                         iterator& iter) const {
    if (!check_treeiter_validity(child)) {
        iter = iterator();  // Set is as invalid, as the TreeModel documentation
                            // says that it should be.
        return false;
    }

    iter = iterator();  // Set is as invalid, as the TreeModel documentation
                        // says that it should be.
    return false;  // There are no children, so no parents.
}

Gtk::TreeModel::Path RangerTreeModel::get_path_vfunc(
    const iterator& /* iter */) const {
    // TODO:
    return Path();
}

bool RangerTreeModel::get_iter_vfunc(const Path& path, iterator& iter) const {
    unsigned sz = path.size();
    if (!sz) {
        iter = iterator();  // Set is as invalid, as the TreeModel documentation
                            // says that it should be.
        return false;
    }

    if (sz > 1)  // There are no children.
    {
        iter = iterator();  // Set is as invalid, as the TreeModel documentation
                            // says that it should be.
        return false;
    }

    // This is a new GtkTreeIter, so it needs the current stamp value.
    // See the comment in the constructor.
    iter = iterator();  // clear the input parameter.
    iter.set_stamp(m_stamp);

    // Store the row_index in the GtkTreeIter:
    // See also iter_next_vfunc()
    // TODO: Store a pointer to some more complex data type such as a
    // typeListOfRows::iterator.

    int row_index = path[0];
    // Store the GlueItem in the GtkTreeIter.
    // This will be deleted in the GlueList destructor,
    // which will be called when the old GtkTreeIters are marked as invalid,
    // when the stamp value changes.
    iter.gobj()->user_data = reinterpret_cast<void*>(row_index);

    return true;
}

Gtk::TreeModelColumn<Glib::ustring>& RangerTreeModel::get_model_column(
    int column) {
    return m_listModelColumns[column];
}

int RangerTreeModel::get_data_row_iter_from_tree_row_iter(
    const iterator& iter) const {
    // Don't call this on an invalid iter.
    void* udata = iter.gobj()->user_data;
    int row_index = *reinterpret_cast<int*>(&udata);
    if (row_index > LA_MAGICKA_ROWS)
        return LA_MAGICKA_ROWS;
    else
        return row_index;  // TODO: Performance.
}

bool RangerTreeModel::check_treeiter_validity(const iterator& iter) const {
    // Anything that modifies the model's structure should change the model's
    // stamp,
    // so that old iters are ignored.
    return m_stamp == iter.get_stamp();
}

// screwSnakeCase
void RangerTreeModel::setRanger(const std::shared_ptr< SqliteRanger >& ranger) {
    _ranger = ranger;
}

int RangerTreeModel::iterToRow(const iterator& iter) const {
    return get_data_row_iter_from_tree_row_iter(iter);
}

void RangerTreeModel::appendColumns(Gtk::TreeView& view,const char** names) {
    TEMPLATIOUS_0_TO_N(i,LA_MAGICKA_COLUMNS) {
        view.append_column(names[i],get_model_column(i));
    }
}

SqliteRanger& RangerTreeModel::getRanger() {
    return *_ranger;
}

}
