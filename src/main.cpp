
#include <iostream>
#include <gtkmm.h>
#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicPackCreator.hpp>
#include <LuaPlumbing/plumbing.hpp>
#include <gtkmm/GtkMMRangerModel.hpp>
#include <util/AutoReg.hpp>
#include <util/Semaphore.hpp>
#include <gtkmm/GtkMMSessionWidget.hpp>
#include <io/SafeListDownloaderFactory.hpp>
#include <model/AsyncSqliteFactory.hpp>

TEMPLATIOUS_TRIPLET_STD;

struct GenericGtkWindowInterface {
    // query gtk window
    // Signature:
    // < GetGtkWindow, GtkWindow* (outres) >
    DUMMY_STRUCT(GetGtkWindow);
};

struct MainWindowInterface {
    // emitted when new file creation
    // is requested.
    DUMMY_REG(OutNewFileSignal,"MWI_OutNewFileSignal");

    // emitted when move button is
    // pressed.
    DUMMY_REG(OutMoveButtonClicked,"MWI_OutMoveButtonClicked");

    // emitted when new file creation
    // is requested.
    DUMMY_REG(OutDirChangedSignal,"MWI_OutDirChangedSignal");

    // emitted when delete dir button clicked
    // is requested.
    DUMMY_REG(OutDeleteDirButtonClicked,"MWI_OutDeleteDirButtonClicked");

    // emitted when new directory button clicked
    DUMMY_REG(OutNewDirButtonClicked,"MWI_OutNewDirButtonClicked");

    // emitted when download safelist button is clicked
    DUMMY_REG(OutDownloadSafelistButtonClicked,"MWI_OutDownloadSafelistButtonClicked");

    // emitted when open safelist button is clicked
    DUMMY_REG(OutOpenSafelistButtonClicked,"MWI_OutOpenSafelistButtonClicked");

    // emitted when show downloads button is clicked
    // Signature:
    // < OutShowDownloadsToggled, bool (state) >
    DUMMY_REG(OutShowDownloadsToggled,"MWI_OutShowDownloadsToggled");

    // emitted when show downloads button is clicked
    // Signature:
    // < OutShowDownloadsToggled >
    DUMMY_REG(OutRightClickFolderList,"MWI_OutRightClickFolderList");

    // emitted in draw routine after async messages processed,
    // yet still in overloaded draw method
    DUMMY_REG(OutDrawEnd,"MWI_OutDrawEnd");

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

    // Reveal download session
    // Signature: < InRevealDownloads, bool (value) >
    DUMMY_REG(InRevealDownloads,"MWI_InRevealDownloads");

    // Set model for download widgets
    // Signature: < InSetDownloadModel, StrongMsgPtr (model) >
    DUMMY_REG(InSetDownloadModel,"MWI_InSetDownloadModel");

    // query current directory id
    // Signature: < QueryCurrentDirId, int (output) >
    DUMMY_REG(QueryCurrentDirId,"MWI_QueryCurrentDirId");

    // query current directory name
    // Signature: < QueryCurrentDirName, std::string (output) >
    DUMMY_REG(QueryCurrentDirName,"MWI_QueryCurrentDirName");

    // query session widget
    // Signature: < QueryDownloadSessionWidget, StrongMsgPtr >
    DUMMY_REG(QueryDownloadSessionWidget,"MWI_QueryDownloadSessionWidget");
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


#define BIND_GTK_BUTTON(gladeName,memberName,funcName)  \
    bld->get_widget(gladeName,memberName);  \
    memberName->signal_clicked().connect(   \
        sigc::mem_fun(*this,funcName)       \
    );

struct GtkMainWindow : public Messageable {

    typedef MainWindowInterface MWI;

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
        BIND_GTK_BUTTON("downloadButton",
            _dlSafelistBtn,
            &GtkMainWindow::downloadButtonClicked);
        BIND_GTK_BUTTON("openSafelistButton",
            _openSafelistBtn,
            &GtkMainWindow::openSafelistClicked);
        BIND_GTK_BUTTON("showDownloadsButton",
            _showDownloadsBtn,
            &GtkMainWindow::showDownloadsButtonClicked);

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

    static void spinUpdater(const std::shared_ptr< GtkMainWindow >& mwnd) {
        auto res = DrawUpdater::spinNew(mwnd);
        mwnd->_drawUpdater = res;
    }

private:

    struct DrawUpdater {

        DrawUpdater() = delete;
        DrawUpdater(const DrawUpdater&) = delete;
        DrawUpdater(DrawUpdater&&) = delete;

        DrawUpdater(
            const std::shared_ptr< GtkMainWindow >& mwnd) : // 100 ms
            _mwndPtr(mwnd),
            _scheduledTime(-1)
        {}

        static std::shared_ptr< DrawUpdater > spinNew(const std::shared_ptr< GtkMainWindow >& mwnd) {
            auto res = std::make_shared< DrawUpdater >(mwnd);
            std::weak_ptr< GtkMainWindow > weakCpy = mwnd;
            std::thread(
                [=]() {
                    // reference point
                    auto& ref = referencePoint();
                    for (;;) {
                        res->_sem.wait();
                        int64_t scheduledCpy = res->_scheduledTime;
                        if (scheduledCpy > 0) {
                            auto now = std::chrono::high_resolution_clock::now();
                            auto millisApart = std::chrono::duration_cast<
                                std::chrono::milliseconds
                            >(now - ref).count();
                            auto diff = scheduledCpy - millisApart;
                            if (diff > 0) {
                                std::this_thread::sleep_for(
                                    std::chrono::milliseconds(diff)
                                );
                                auto l = weakCpy.lock();
                                if (nullptr == l) {
                                    return;
                                }
                                //printf("Enqued...\n");
                                l->_wnd->queue_draw();
                                res->_scheduledTime = -1;
                            }
                        }
                    }
                }
            ).detach();
            return res;
        }

        void scheduleUpdate(int afterMillis) {
            auto now = std::chrono::high_resolution_clock::now();
            auto& ref = referencePoint();
            auto millisApart = std::chrono::duration_cast<
                std::chrono::milliseconds
            >(now - ref).count();

            int64_t predicted = millisApart + afterMillis;
            if (_scheduledTime < 0) {
                _scheduledTime = predicted;
            }
            _sem.notify();
        }

        static const std::chrono::high_resolution_clock::time_point&
            referencePoint()
        {
            static auto result = std::chrono::high_resolution_clock::now();
            return result;
        }

    private:
        std::weak_ptr< GtkMainWindow > _mwndPtr;
        StackOverflow::Semaphore _sem;
        int64_t _scheduledTime;
    };


    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    VmfPtr genHandler() {
        typedef GenericMesseagableInterface GMI;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch<
                GMI::OutRequestUpdate
            >(
                [=](GMI::OutRequestUpdate) {
                    auto locked = _drawUpdater.lock();
                    if (nullptr != locked) {
                        locked->scheduleUpdate(100);
                    }
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
            ),
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
            SF::virtualMatch< MWI::InSetDownloadModel, StrongMsgPtr >(
                [=](MWI::InSetDownloadModel,const StrongMsgPtr& msg) {
                    this->_sessionTab->setModel(msg);
                }
            ),
            SF::virtualMatch< MWI::QueryDownloadSessionWidget, StrongMsgPtr >(
                [=](MWI::InSetDownloadModel,StrongMsgPtr& msg) {
                    msg = this->_sessionTab;
                }
            ),
            SF::virtualMatch< GenericGtkWindowInterface::GetGtkWindow, Gtk::Window* >(
                [=](GenericGtkWindowInterface::GetGtkWindow,Gtk::Window*& ptr) {
                    ptr = this->_wnd.get();
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
        auto msg = SF::vpack< MWI::OutDrawEnd >(nullptr);
        _notifierCache.notify(msg);
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

    template <class... Types,class... Args>
    void notifySingleThreaded(Args&&... args) {
        auto msg = SF::vpack<Types...>(
            std::forward<Args>(args)...
        );
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

    bool leftListClicked(GdkEventButton* event) {
        if ( (event->type == GDK_BUTTON_PRESS) && (event->button) == 3 ) {
            notifySingleThreaded< MainWindowInterface::OutRightClickFolderList >(nullptr);
            return true;
        }

        return false;
    }

    void createDirModel() {
        _dirStore = Gtk::TreeStore::create(_dirColumns);
        _left->append_column( "Name", _dirColumns.m_colName );
        _left->set_model(_dirStore);
        _left->signal_button_press_event().connect(
            sigc::mem_fun(*this,&GtkMainWindow::leftListClicked),false);

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

    void downloadButtonClicked() {
        notifySingleThreaded<
            MWI::OutDownloadSafelistButtonClicked
        >(nullptr);
    }

    void openSafelistClicked() {
        notifySingleThreaded<
            MWI::OutOpenSafelistButtonClicked
        >(nullptr);
    }

    void showDownloadsButtonClicked() {
        bool res = _showDownloadsBtn->get_active();
        notifySingleThreaded<
            MWI::OutShowDownloadsToggled,
            bool
        >(nullptr,res);
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
    Gtk::Button* _openSafelistBtn;
    Gtk::Button* _dlSafelistBtn;
    Gtk::ToggleButton* _showDownloadsBtn;
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
    std::weak_ptr< GtkMainWindow > _myself;
    std::weak_ptr<DrawUpdater> _drawUpdater;
};

struct GtkDialogService : public Messageable {
    GtkDialogService() : _handler(genHandler()) {}

    // Open file chooser
    // Signature:
    // <
    //  FileChooserDialog,
    //  StrongMsgPtr (parent window),
    //  std::string (title),
    //  std::string (wildcard),
    //  std::string (output path, empty if none)
    // >
    DUMMY_REG(FileChooserDialog,"GDS_FileChooserDialog");

    void message(templatious::VirtualPack& msg) override {
        _handler->tryMatch(msg);
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "Single threaded messaging only." );
    }

    SafeLists::VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch<
                FileChooserDialog,
                StrongMsgPtr,
                std::string,
                std::string,
                std::string
            >(
                [=](FileChooserDialog,
                    const StrongMsgPtr& window,
                    const std::string& title,
                    const std::string& wildcard,
                    std::string& out)
                {
                    Gtk::FileChooserDialog dlg(title.c_str(),Gtk::FILE_CHOOSER_ACTION_OPEN);
                    auto queryTransient = SF::vpack<
                        GenericGtkWindowInterface::GetGtkWindow,
                        Gtk::Window*
                    >(nullptr,nullptr);
                    window->message(queryTransient);
                    assert( queryTransient.useCount() > 0 && "No transient cholo..." );
                    dlg.set_transient_for(*queryTransient.fGet<1>());
                    dlg.add_button("_Cancel",Gtk::RESPONSE_CANCEL);
                    dlg.add_button("Ok",Gtk::RESPONSE_OK);

                    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
                    filter->add_pattern(wildcard.c_str());
                    dlg.set_filter(filter);

                    int result = dlg.run();
                    if (Gtk::RESPONSE_OK == result) {
                        out = dlg.get_filename();
                        return;
                    }

                    out = "";
                }
            )
        );
    }

private:
    SafeLists::VmfPtr _handler;
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

    auto dlFactory = SafeLists::
        SafeListDownloaderFactory::createNew();

    auto builder = Gtk::Builder::create();
    builder->add_from_file("uischemes/main.glade");
    auto mainWnd = std::make_shared< GtkMainWindow >(builder);
    GtkMainWindow::spinUpdater(mainWnd);
    auto singleInputDialog = std::make_shared< GtkInputDialog >(builder);
    auto asyncSqliteFactory = SafeLists::AsyncSqliteFactory::createNew();
    auto mainModel = std::make_shared< MainModel >();
    auto dialogService = std::make_shared< GtkDialogService >();
    ctx->addMesseagableWeak("mainWindow",mainWnd);
    ctx->addMesseagableWeak("singleInputDialog",singleInputDialog);
    ctx->addMesseagableWeak("mainModel",mainModel);
    ctx->addMesseagableStrong("dlSessionFactory",dlFactory);
    ctx->addMesseagableStrong("asyncSqliteFactory",asyncSqliteFactory);
    ctx->addMesseagableStrong("dialogService",dialogService);
    ctx->doFile("lua/main.lua");
    app->run(mainWnd->getWindow(),argc,argv);
}

