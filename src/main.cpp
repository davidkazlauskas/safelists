
#include <iostream>
#include <gtkmm.h>
#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicPackCreator.hpp>
#include <LuaPlumbing/plumbing.hpp>
#include <gtkmm/GtkMMRangerModel.hpp>
#include <util/AutoReg.hpp>
#include <gtkmm/GtkMMSessionWidget.hpp>

TEMPLATIOUS_TRIPLET_STD;

struct MainWindowInterface {
    // emitted when new file creation
    // is requested.
    // In lua: MWI_OutNewFileSignal
    DUMMY_REG(OutNewFileSignal,"MWI_OutNewFileSignal");

    // emitted when move button is
    // pressed.
    // In lua: MWI_OutMoveButtonClicked
    DUMMY_REG(OutMoveButtonClicked,"MWI_OutMoveButtonClicked");

    // emitted when new file creation
    // is requested.
    // In lua: MWI_OutDirChangedSignal
    DUMMY_REG(OutDirChangedSignal,"MWI_OutDirChangedSignal");

    // emitted when delete dir button clicked
    // is requested.
    // In lua: MWI_OutDeleteDirButtonClicked
    DUMMY_REG(OutDeleteDirButtonClicked,"MWI_OutDeleteDirButtonClicked");

    // emitted when new directory button clicked
    // In lua: MWI_OutNewDirButtonClicked
    DUMMY_REG(OutNewDirButtonClicked,"MWI_OutNewDirButtonClicked");

    // emit to attach listener
    // In lua: MWI_InAttachListener
    // Signature: < InAttachListener, StrongMsgPtr >
    DUMMY_REG(InAttachListener,"MWI_InAttachListener");

    // emit to set tree model from snapshot
    // Signature: < InSetTreeData, TableSnapshot >
    DUMMY_STRUCT(InSetTreeData);

    // emit to set tree model from snapshot
    // Signature: < InSetFileData, TableSnapshot >
    DUMMY_STRUCT(InSetFileData);

    // set current status text
    // Signature: < InSetStatusText, std::string >
    DUMMY_REG(InSetStatusText,"MWI_InSetStatusText");

    // display specified id in the tree
    // Signature: < InSelectDirIdInTree, int >
    DUMMY_REG(InSelectDirIdInTree,"MWI_InSelectDirIdInTree");

    // Move prev item on selection stack
    // down below current item.
    // Signature: < InMoveChildUnderParent, int (error) >
    // errors:
    //  0 - ok
    //  1 - to move is parent to child
    //  2 - iterators same
    DUMMY_REG(InMoveChildUnderParent,"MWI_InMoveChildUnderParent");

    // Erase selected item
    // Signature: < InDeleteSelectedDir >
    DUMMY_REG(InDeleteSelectedDir,"MWI_InDeleteSelectedDir");

    // Erase selected item
    // Signature: < InRevealDownloads, bool (value) >
    DUMMY_REG(InRevealDownloads,"MWI_InRevealDownloads");

    // query current directory id
    // Signature: < QueryCurrentDirId, int (output) >
    DUMMY_REG(QueryCurrentDirId,"MWI_QueryCurrentDirId");

    // query current directory name
    // Signature: < QueryCurrentDirName, std::string (output) >
    DUMMY_REG(QueryCurrentDirName,"MWI_QueryCurrentDirName");
};

void registerSqliteInFactory(templatious::DynVPackFactoryBuilder& bld);

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
        DUMMY_REG(InLoadFolderTree,"MMI_InLoadFolderTree");

        // use to load folder tree
        // Signature: <
        //     InLoadFolderTree,
        //     StrongMsgPtr (async sqlite),
        //     StrongMsgPtr (notify)
        // >
        DUMMY_REG(InLoadFileList,"MMI_InLoadFileList");
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

    void handleLoadFolderTree(const StrongMsgPtr& asyncSqlite,const StrongMsgPtr& toNotify) {
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
            nullptr,
                "WITH RECURSIVE "
                "children(d_id,d_name,d_parent) AS ( "
                "   SELECT dir_id,dir_name,dir_parent FROM directories WHERE dir_name='root' AND dir_id=1 "
                "   UNION ALL "
                "   SELECT dir_id,dir_name,dir_parent "
                "   FROM directories JOIN children ON directories.dir_parent=children.d_id "
                ") SELECT d_id,d_name,d_parent FROM children; ",
            std::move(headers),TableSnapshot());

        asyncSqlite->message(message);
    }

    void handleLoadFileList(int id,const StrongMsgPtr& asyncSqlite,const StrongMsgPtr& toNotify) {
        typedef SafeLists::AsyncSqlite ASql;
        std::vector< std::string > headers(
            {"file_id","dir_id","file_name","file_size","file_hash"});
        std::weak_ptr< Messageable > weakNotify = toNotify;
        char queryBuf[512];
        sprintf(queryBuf,
            "SELECT file_id,dir_id,file_name,file_size,file_hash_256 FROM files"
            " WHERE dir_id=%d;",
            id);
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
                auto outMsg = SF::vpackPtr< MWI::InSetFileData, int, TableSnapshot >(
                    nullptr, id, std::move(snapref)
                );
                locked->message(outMsg);
            },
            nullptr,
            queryBuf,
            std::move(headers),TableSnapshot()
        );

        asyncSqlite->message(message);
    }

    VmfPtr genHandler() {
        typedef MainModelInterface MMI;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< MMI::InLoadFolderTree,
                              StrongMsgPtr,
                              StrongMsgPtr >
            ([=](
                MMI::InLoadFileList,
                const StrongMsgPtr& asyncSqlite,
                const StrongMsgPtr& toNotify) {
                this->handleLoadFolderTree(asyncSqlite,toNotify);
            }),
            SF::virtualMatch< MMI::InLoadFileList,
                              int, // (id)
                              StrongMsgPtr,
                              StrongMsgPtr >
            ([=](
                MMI::InLoadFileList,
                int id,
                const StrongMsgPtr& asyncSqlite,
                const StrongMsgPtr& toNotify) {
                this->handleLoadFileList(id,asyncSqlite,toNotify);
            })
        );
    }

    VmfPtr _messageHandler;
};



struct GtkMainWindow : public Messageable {

    GtkMainWindow(Glib::RefPtr<Gtk::Builder>& bld) :
        _left(nullptr),
        _right(nullptr),
        _messageHandler(genHandler()),
        _lastSelectedDirId(-1)
    {
        Gtk::Window* outWnd = nullptr;
        bld->get_widget("window1",outWnd);
        _wnd.reset( outWnd );

        bld->get_widget("treeview1",_right);
        bld->get_widget("treeview3",_left);
        bld->get_widget("addNewBtn",_addNewBtn);
        bld->get_widget("moveButton",_moveDirBtn);
        bld->get_widget("deleteDirButton",_deleteDirBtn);
        bld->get_widget("statusBarLabel",_statusBar);
        bld->get_widget("newDirectoryButton",_newDirBtn);
        bld->get_widget("reavealerSessions",_revealerSessions);

        _sessionTab = SafeLists::GtkSessionTab::makeNew();
        _revealerSessions->add(*_sessionTab->getTabs());

        _addNewBtn->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkMainWindow::addNewButtonClicked)
        );

        _moveDirBtn->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkMainWindow::moveButtonClicked)
        );

        _deleteDirBtn->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkMainWindow::deleteDirButtonClicked)
        );

        _newDirBtn->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkMainWindow::newDirButtonClicked)
        );

        _wnd->signal_draw().connect(
            sigc::mem_fun(*this,&GtkMainWindow::onDraw)
        );

        createDirModel();
        createFileModel();

        _selectionStack.resize(2);
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
        _messageCache.enqueue(msg);
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
                    setTreeModel(snapshot);
                }
            ),
            SF::virtualMatch<
                MWI::InSetFileData, int, TableSnapshot
            >(
                [=](MWI::InSetFileData,int id,TableSnapshot& snapshot) {
                    setFileModel(id,snapshot);
                }
            ),
            SF::virtualMatch< MWI::InSetStatusText, const std::string >(
                [=](MWI::InSetStatusText,const std::string& text) {
                    this->_statusBar->set_text(text.c_str());
                }
            ),
            SF::virtualMatch< MWI::QueryCurrentDirId, int >(
                [=](MWI::QueryCurrentDirId,int& outId) {
                    auto selection = _dirSelection->get_selected();
                    if (nullptr != selection) {
                        auto row = *selection;
                        outId = row[_dirColumns.m_colId];
                    } else {
                        outId = -1;
                    }
                }
            ),
            SF::virtualMatch< MWI::QueryCurrentDirName, std::string >(
                [=](MWI::QueryCurrentDirName,std::string& outstr) {
                    auto selection = _dirSelection->get_selected();
                    if (nullptr != selection) {
                        auto row = *selection;
                        Glib::ustring str = row[_dirColumns.m_colName];
                        outstr = str.c_str();
                    } else {
                        outstr = "[unselected]";
                    }
                }
            ),
            SF::virtualMatch< MWI::InSelectDirIdInTree, const int >(
                [=](MWI::InSelectDirIdInTree,int id) {
                    Gtk::TreeModel::iterator iter;
                    auto children = _dirStore->children();
                    bool found = findIdIter(id,iter,children);
                    if (found) {
                        // doesn't work, freeze for some reason
                        //_dirSelection->select(iter);
                    }
                }
            ),
            SF::virtualMatch< MWI::InMoveChildUnderParent, int >(
                [=](MWI::InMoveChildUnderParent,int& out) {
                    auto& toMove = _selectionStack[0];
                    auto& parent = _selectionStack[1];
                    if (toMove == parent) {
                        // dunno how this might happen but whatever...
                        out = 2;
                        return;
                    }

                    if (hasIterUnder(toMove,parent)) {
                        out = 1;
                        return;
                    }

                    auto toMoveRow = *toMove;
                    DirRow dr;
                    getDirRow(dr,toMoveRow);
                    _dirStore->erase(toMove);
                    auto newPlace = *_dirStore->append(parent->children());
                    setDirRow(dr,newPlace);
                    out = 0;
                }
            ),
            SF::virtualMatch< MWI::InDeleteSelectedDir >(
                [=](MWI::InDeleteSelectedDir) {
                    auto& toErase = _selectionStack[1];
                    _dirStore->erase(toErase);
                }
            ),
            SF::virtualMatch< MWI::InRevealDownloads, bool >(
                [=](MWI::InDeleteSelectedDir,bool value) {
                    _revealerSessions->set_reveal_child(value);
                }
            )
        );
    }

    bool hasIterUnder(Gtk::TreeModel::iterator& parent,Gtk::TreeModel::iterator& child) {
        auto childRow = *child;
        int childId = childRow[_dirColumns.m_colId];
        auto children = parent->children();
        Gtk::TreeModel::iterator outIter;
        bool result = findIdIter(childId,outIter,children);
        return result;
    }

    bool findIdIter(
        int id,
        Gtk::TreeModel::iterator& outIter,
        Gtk::TreeModel::Children& children)
    {
        auto beg = children.begin();
        auto end = children.end();
        for (; beg != end; ++beg) {
            auto current = *beg;
            if (current[_dirColumns.m_colId] == id) {
                outIter = beg;
                return true;
            }
            auto currentChildren = current.children();
            if (!currentChildren.empty()) {
                bool innerSearch = findIdIter(id,outIter,currentChildren);
                if (innerSearch) {
                    return true;
                }
            }
        }
        return false;
    }

    struct DirectoryTreeColumns : public Gtk::TreeModel::ColumnRecord {
        DirectoryTreeColumns() {
            add(m_colId);
            add(m_colName);
            add(m_colParent);
        }

        Gtk::TreeModelColumn<int> m_colId;
        Gtk::TreeModelColumn<int> m_colParent;
        Gtk::TreeModelColumn<Glib::ustring> m_colName;
    };

    struct FileTreeColumns : public Gtk::TreeModel::ColumnRecord {
        FileTreeColumns() {
            add(m_fileId);
            add(m_dirId);
            add(m_fileName);
            add(m_fileSize);
            add(m_fileHash);
        }

        Gtk::TreeModelColumn<int> m_fileId;
        Gtk::TreeModelColumn<int> m_dirId;
        Gtk::TreeModelColumn<Glib::ustring> m_fileName;
        Gtk::TreeModelColumn<int> m_fileSize;
        Gtk::TreeModelColumn<Glib::ustring> m_fileHash;
    };

    struct DirRow {
        int _id;
        int _parent;
        std::string _name;
    };

    void setDirRow(const DirRow& r,Gtk::TreeModel::Row& mdlRow) {
        mdlRow[_dirColumns.m_colName] = r._name;
        mdlRow[_dirColumns.m_colId] = r._id;
        mdlRow[_dirColumns.m_colParent] = r._parent;
    }

    void getDirRow(DirRow& r,const Gtk::TreeModel::Row& mdlRow) {
        Glib::ustring name = mdlRow[_dirColumns.m_colName];
        r._name = name.c_str();
        r._id = mdlRow[_dirColumns.m_colId];
        r._parent = mdlRow[_dirColumns.m_colParent];
    }

    // receiving id, dir name, dir parent
    void setTreeModel(TableSnapshot& snapshot) {
        std::vector< DirRow > vecRow;
        vecRow.reserve( 256 );
        DirRow r;
        snapshot.traverse(
            [&](int row,int column,const char* value,const char* header) {
                switch (column) {
                case 0:
                    r._id = std::atoi(value);
                    break;
                case 1:
                    r._name = value;
                    break;
                case 2:
                    r._parent = std::atoi(value);
                    SA::add(vecRow,r);
                    break;
                }
                return true;
            }
        );

        _dirStore->clear();
        std::unordered_map< int, Gtk::TreeModel::Row > rows;
        rows.reserve( 1024 );

        assert( SA::size(vecRow) > 0 );
        // first should be root
        auto& first = vecRow[0];
        assert( first._id == 1 && first._name == "root"
                && first._parent == -1
                && "Should be root..." );

        auto row = *(_dirStore->append());
        setDirRow(first,row);
        rows.insert( std::pair<int,Gtk::TreeModel::Row>(
            first._id,row) );

        auto range = SF::seqL<int>(1,SA::size(vecRow));
        TEMPLATIOUS_FOREACH(auto ir,range) {
            auto& i = vecRow[ir];
            auto find = rows.find(i._parent);
            assert( find != rows.end() );

            auto newRow = *(_dirStore->append(find->second->children()));
            setDirRow(i,newRow);
            rows.insert(std::pair<int,Gtk::TreeModel::Row>(
                i._id,newRow));
        }
    }

    void setFileModel(int id,TableSnapshot& snapshot) {
        struct Row {
            int _id;
            int _dirId;
            long _size;
            std::string _name;
            std::string _hash256;
        };

        if (id != _lastSelectedDirId) {
            return;
        }

        auto setRow =
            [=](const Row& r,Gtk::TreeModel::Row& mdlRow) {
                mdlRow[_fileColumns.m_fileId] = r._id;
                mdlRow[_fileColumns.m_dirId] = r._dirId;
                mdlRow[_fileColumns.m_fileName] = r._name;
                mdlRow[_fileColumns.m_fileSize] = r._size;
                mdlRow[_fileColumns.m_fileHash] = r._hash256;
            };

        Row r;
        _fileStore->clear();
        snapshot.traverse(
            [&](int row,int column,const char* value,const char* header) {
                switch (column) {
                case 0:
                    r._id = std::atoi(value);
                    break;
                case 1:
                    r._dirId = std::atoi(value);
                    break;
                case 2:
                    r._name = value;
                    break;
                case 3:
                    r._size = std::atol(value);
                    break;
                case 4:
                    r._hash256 = value;
                    auto row = *(_fileStore->append());
                    setRow(r,row);
                    break;
                }
                return true;
            }
        );
    }

    bool onDraw(const Cairo::RefPtr<Cairo::Context>& cr) {
        _callbackCache.process();
        _messageCache.process(
            [&](templatious::VirtualPack& pack) {
                this->_messageHandler->tryMatch(pack);
            }
        );
        return false;
    }

    void directoryToViewChanged() {
        auto iter = _dirSelection->get_selected();
        pushToSelectionStack(iter);
        if (nullptr != iter) {
            auto row = *iter;
            int id = row[_dirColumns.m_colId];
            _lastSelectedDirId = id;
            auto msg = SF::vpack< MainWindowInterface::OutDirChangedSignal, int >(
                nullptr, id
            );
            _notifierCache.notify(msg);
        }
    }

    void addNewButtonClicked() {
        auto msg = SF::vpack< MainWindowInterface::OutNewFileSignal >(nullptr);
        _notifierCache.notify(msg);
    }

    void moveButtonClicked() {
        auto msg = SF::vpack< MainWindowInterface::OutMoveButtonClicked >(nullptr);
        _notifierCache.notify(msg);
    }

    void deleteDirButtonClicked() {
        auto msg = SF::vpack< MainWindowInterface::OutDeleteDirButtonClicked >(nullptr);
        _notifierCache.notify(msg);
    }

    void newDirButtonClicked() {
        auto msg = SF::vpack< MainWindowInterface::OutNewDirButtonClicked >(nullptr);
        _notifierCache.notify(msg);
    }

    void createDirModel() {
        _dirStore = Gtk::TreeStore::create(_dirColumns);
        _left->append_column( "Name", _dirColumns.m_colName );
        _left->set_model(_dirStore);
        _dirSelection = _left->get_selection();
        _dirSelection->set_mode(Gtk::SELECTION_SINGLE);
        _dirSelection->signal_changed().connect(sigc::mem_fun(
            *this,&GtkMainWindow::directoryToViewChanged));
    }

    void createFileModel() {
        _fileStore = Gtk::ListStore::create(_fileColumns);
        _right->append_column("Name", _fileColumns.m_fileName);
        _right->append_column("Size", _fileColumns.m_fileSize);
        _right->set_model(_fileStore);
    }

    void pushToSelectionStack(Gtk::TreeModel::iterator& iter) {
        _selectionStack[0] = _selectionStack[1];
        _selectionStack[1] = iter;
    }

    std::unique_ptr< Gtk::Window > _wnd;
    Gtk::TreeView* _left;
    Gtk::TreeView* _right;
    Gtk::Button* _addNewBtn;
    Gtk::Button* _moveDirBtn;
    Gtk::Button* _deleteDirBtn;
    Gtk::Button* _newDirBtn;
    Gtk::Label* _statusBar;
    Gtk::Revealer* _revealerSessions;
    ModelColumns _mdl;

    VmfPtr _messageHandler;

    NotifierCache _notifierCache;
    CallbackCache _callbackCache;
    MessageCache _messageCache;

    // Models, columns...

    // DIRS
    DirectoryTreeColumns _dirColumns;
    Glib::RefPtr<Gtk::TreeStore> _dirStore;
    Glib::RefPtr<Gtk::TreeSelection> _dirSelection;

    // FILES
    FileTreeColumns _fileColumns;
    Glib::RefPtr<Gtk::ListStore> _fileStore;

    // STATE
    int _lastSelectedDirId;
    // should contain two elements always
    std::vector<Gtk::TreeModel::iterator> _selectionStack;
    std::shared_ptr< SafeLists::GtkSessionTab > _sessionTab;
};

struct GtkInputDialog : public Messageable {

    struct Interface {
        // Show the dialog
        // in lua: INDLG_InShowDialog
        DUMMY_REG(InShowDialog,"INDLG_InShowDialog");

        // Set notifier to notify messages
        // in lua: INDLG_InSetNotifier
        // Signature: < InSetNotifier, StrongMsgPtr >
        DUMMY_REG(InSetNotifier,"INDLG_InSetNotifier");

        // Set label text
        // in lua: INDLG_InSetLabel
        // Signature: < InSetLabel, std::string >
        DUMMY_REG(InSetLabel,"INDLG_InSetLabel");

        // Emitted when ok button is clicked
        DUMMY_REG(OutOkClicked,"INDLG_OutOkClicked");

        // Emitted when cancel button is clicked
        DUMMY_REG(OutCancelClicked,"INDLG_OutCancelClicked");

        // Query input text
        // in lua: INDLG_InQueryInput
        // Signature: < QueryInput, std::string (out) >
        DUMMY_REG(QueryInput,"INDLG_QueryInput")
    };

    GtkInputDialog(Glib::RefPtr<Gtk::Builder>& bld) : _handler(genHandler()) {
        Gtk::Dialog* dlg = nullptr;
        bld->get_widget("dialog2",dlg);
        _dlg.reset(dlg);

        bld->get_widget("dialogText",_label);
        bld->get_widget("dialogInput",_entry);
        bld->get_widget("dialogOkButton",_okButton);
        bld->get_widget("dialogCancelButton",_cancelButton);

        _okButton->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkInputDialog::okClicked)
        );

        _cancelButton->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkInputDialog::cancelClicked)
        );
    }

    void message(templatious::VirtualPack& msg) override {
        _handler->tryMatch(msg);
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "Single threaded messaging only." );
    }

private:
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    void okClicked() {
        auto locked = _toNotify.lock();
        if (nullptr == locked) {
            return;
        }

        auto msg = SF::vpack< Interface::OutOkClicked >(nullptr);
        locked->message(msg);
    }

    void cancelClicked() {
        auto locked = _toNotify.lock();
        if (nullptr == locked) {
            return;
        }

        auto msg = SF::vpack< Interface::OutCancelClicked >(nullptr);
        locked->message(msg);
    }

    VmfPtr genHandler() {
        typedef Interface INT;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< INT::InShowDialog, bool >(
                [=](INT::InShowDialog,bool show) {
                    if (show) {
                        _dlg->show();
                    } else {
                        _dlg->hide();
                    }
                }
            ),
            SF::virtualMatch< INT::InSetNotifier, StrongMsgPtr >(
                [=](INT::InSetNotifier,const StrongMsgPtr& msg) {
                    this->_toNotify = msg;
                }
            ),
            SF::virtualMatch< INT::InSetLabel, const std::string >(
                [=](INT::InSetLabel,const std::string& str) {
                    this->_label->set_text(str.c_str());
                }
            ),
            SF::virtualMatch< INT::QueryInput, std::string >(
                [=](INT::InSetLabel,std::string& str) {
                    str = this->_entry->get_text().c_str();
                }
            )
        );
    }

    std::unique_ptr< Gtk::Dialog > _dlg;
    Gtk::Label* _label;
    Gtk::Entry* _entry;
    Gtk::Button* _okButton;
    Gtk::Button* _cancelButton;

    WeakMsgPtr _toNotify;

    VmfPtr _handler;
};

templatious::DynVPackFactory makeVfactory() {
    templatious::DynVPackFactoryBuilder bld;

    LuaContext::registerPrimitives(bld);

    registerSqliteInFactory(bld);

    SafeLists::traverseTypes(
        [&](const char* name,const templatious::TypeNode* node) {
            bld.attachNode(name,node);
        }
    );

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
    auto singleInputDialog = std::make_shared< GtkInputDialog >(builder);
    auto mainModel = std::make_shared< MainModel >();
    ctx->addMesseagableWeak("mainWindow",mainWnd);
    ctx->addMesseagableWeak("singleInputDialog",singleInputDialog);
    ctx->addMesseagableWeak("mainModel",mainModel);
    ctx->addMesseagableWeak("asyncSqliteCurrent",asyncSqlite);
    ctx->doFile("lua/main.lua");
    app->run(mainWnd->getWindow(),argc,argv);
}

typedef templatious::TypeNodeFactory TNF;

#define ATTACH_NAMED_DUMMY(factory,name,type)   \
    factory.attachNode(name,TNF::makeDummyNode< type >(name))

void registerSqliteInFactory(templatious::DynVPackFactoryBuilder& bld) {
    typedef SafeLists::AsyncSqlite ASql;
    ATTACH_NAMED_DUMMY(bld,"ASQL_Execute",ASql::Execute);
}

