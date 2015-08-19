
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

    // emitted when move button is
    // pressed.
    // In lua: MWI_OutMoveButtonClicked
    DUMMY_STRUCT(OutMoveButtonClicked);

    // emitted when new file creation
    // is requested.
    // In lua: MWI_OutDirChangedSignal
    DUMMY_STRUCT(OutDirChangedSignal);

    // emit to attach listener
    // In lua: MWI_InAttachListener
    // Signature: < InAttachListener, StrongMsgPtr >
    DUMMY_STRUCT(InAttachListener);

    // emit to set tree model from snapshot
    // Signature: < InSetTreeData, TableSnapshot >
    DUMMY_STRUCT(InSetTreeData);

    // emit to set tree model from snapshot
    // Signature: < InSetFileData, TableSnapshot >
    DUMMY_STRUCT(InSetFileData);

    // set current status text
    // Signature: < InSetStatusText, std::string >
    DUMMY_STRUCT(InSetStatusText);

    // display specified id in the tree
    // Signature: < InSelectDirIdInTree, int >
    DUMMY_STRUCT(InSelectDirIdInTree);

    // query current directory id
    // Signature: < QueryCurrentDirId, int (output) >
    DUMMY_STRUCT(QueryCurrentDirId);

    static void registerInFactory(templatious::DynVPackFactoryBuilder& bld);
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
        DUMMY_STRUCT(InLoadFolderTree);

        // use to load folder tree
        // Signature: <
        //     InLoadFolderTree,
        //     StrongMsgPtr (async sqlite),
        //     StrongMsgPtr (notify)
        // >
        DUMMY_STRUCT(InLoadFileList);

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
            "SELECT file_id,dir_id,file_name,file_size,file_hash_sha256 FROM files"
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
        bld->get_widget("moveButton",_moveBtn);
        bld->get_widget("statusBarLabel",_statusBar);

        auto mdl = SafeLists::RangerTreeModel::create();

        _addNewBtn->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkMainWindow::addNewButtonClicked)
        );

        _moveBtn->signal_clicked().connect(
            sigc::mem_fun(*this,&GtkMainWindow::moveButtonClicked)
        );

        _wnd->signal_draw().connect(
            sigc::mem_fun(*this,&GtkMainWindow::onDraw)
        );

        createDirModel();
        createFileModel();
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
            )
        );
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
            if (currentChildren.size() > 0) {
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

    // receiving id, dir name, dir parent
    void setTreeModel(TableSnapshot& snapshot) {
        struct Row {
            int _id;
            int _parent;
            std::string _name;
        };

        auto setRow =
            [=](const Row& r,Gtk::TreeModel::Row& mdlRow) {
                mdlRow[_dirColumns.m_colName] = r._name;
                mdlRow[_dirColumns.m_colId] = r._id;
                mdlRow[_dirColumns.m_colParent] = r._parent;
            };

        std::vector< Row > vecRow;
        vecRow.reserve( 256 );
        Row r;
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
        setRow(first,row);
        rows.insert( std::pair<int,Gtk::TreeModel::Row>(
            first._id,row) );

        auto range = SF::seqL<int>(1,SA::size(vecRow));
        TEMPLATIOUS_FOREACH(auto ir,range) {
            auto& i = vecRow[ir];
            auto find = rows.find(i._parent);
            assert( find != rows.end() );

            auto newRow = *(_dirStore->append(find->second->children()));
            setRow(i,newRow);
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

    std::unique_ptr< Gtk::Window > _wnd;
    Gtk::TreeView* _left;
    Gtk::TreeView* _right;
    Gtk::Button* _addNewBtn;
    Gtk::Button* _moveBtn;
    Gtk::Label* _statusBar;
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
    registerSqliteInFactory(bld);

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
    ATTACH_NAMED_DUMMY(bld,"MWI_OutMoveButtonClicked",MWI::OutMoveButtonClicked);
    ATTACH_NAMED_DUMMY(bld,"MWI_OutDirChangedSignal",MWI::OutDirChangedSignal);
    ATTACH_NAMED_DUMMY(bld,"MWI_InAttachListener",MWI::InAttachListener);
    ATTACH_NAMED_DUMMY(bld,"MWI_InSetStatusText",MWI::InSetStatusText);
    ATTACH_NAMED_DUMMY(bld,"MWI_InSelectDirIdInTree",MWI::InSelectDirIdInTree);
    ATTACH_NAMED_DUMMY(bld,"MWI_QueryCurrentDirId",MWI::QueryCurrentDirId);
}

void MainModel::MainModelInterface::registerInFactory(templatious::DynVPackFactoryBuilder& bld) {
    typedef MainModel::MainModelInterface MMI;
    ATTACH_NAMED_DUMMY(bld,"MMI_InLoadFolderTree",MMI::InLoadFolderTree);
    ATTACH_NAMED_DUMMY(bld,"MMI_InLoadFileList",MMI::InLoadFileList);
}

void registerSqliteInFactory(templatious::DynVPackFactoryBuilder& bld) {
    typedef SafeLists::AsyncSqlite ASql;
    ATTACH_NAMED_DUMMY(bld,"ASQL_Execute",ASql::Execute);
}
