
#include <iostream>
#include <cstdlib>
#include <gtkmm.h>
#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicPackCreator.hpp>
#include <LuaPlumbing/plumbing.hpp>
#include <gtkmm/GtkMMRangerModel.hpp>
#include <util/AutoReg.hpp>
#include <util/Semaphore.hpp>
#include <util/GracefulShutdownGuard.hpp>
#include <util/GenericStMessageable.hpp>
#include <util/ProgramArgs.hpp>
#include <gtkmm/GtkMMSessionWidget.hpp>
#include <gtkmm/GtkMMFileString.hpp>
#include <gtkmm/GenericGtkWidget.hpp>
#include <io/SafeListDownloaderFactory.hpp>
#include <io/RandomFileWriter.hpp>
#include <io/AsyncDownloader.hpp>
#include <model/AsyncSqliteFactory.hpp>
#include <meta/Licensing.hpp>

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

    // emitted when open safelist button is clicked
    DUMMY_REG(OutCreateSafelistButtonClicked,"MWI_OutCreateSafelistButtonClicked");

    // emitted when open safelist button is clicked
    DUMMY_REG(OutResumeDownloadButtonClicked,"MWI_OutResumeDownloadButtonClicked");

    // emitted when show downloads button is clicked
    // Signature:
    // < OutShowDownloadsToggled, bool (state) >
    DUMMY_REG(OutShowDownloadsToggled,"MWI_OutShowDownloadsToggled");

    // emitted when show downloads button is clicked
    // Signature:
    // < OutShowDownloadsToggled >
    DUMMY_REG(OutRightClickFolderList,"MWI_OutRightClickFolderList");

    // emitted when show downloads button is clicked
    // Signature:
    // < OutShowDownloadsToggled >
    DUMMY_REG(OutRightClickFileList,"MWI_OutRightClickFileList");

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

    // set current status text
    // Signature: < InSetDownloadText, std::string >
    DUMMY_REG(InSetDownloadText,"MWI_InSetDownloadText");

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

    // Enable/disable widget.
    // Signature: < InSetWidgetEnabled, std::string (name), bool (value) >
    DUMMY_REG(InSetWidgetEnabled,"MWI_InSetWidgetEnabled");

    // Set widget text
    // Signature: < InSetWidgetText, std::string(name), std::string (text) >
    DUMMY_REG(InSetWidgetText,"MWI_InSetWidgetText");

    // Set selected directory text
    // Signature: < InSetCurrentDirName, std::string(name) >
    DUMMY_REG(InSetCurrentDirName,"MWI_InSetCurrentDirName");

    // Add child directory with specified name under current
    // Signature: < InAddChildUnderCurrentDir, std::string (name), int (id) >
    DUMMY_REG(InAddChildUnderCurrentDir,"MWI_InAddChildUnderCurrentDir");

    // Clear file list.
    DUMMY_REG(InClearCurrentFiles,"MWI_InClearCurrentFiles");

    // add new file under current directory
    // Signature: <
    //     InAddNewFileInCurrent,
    //     int (fileid),
    //     int (dirid),
    //     std::string (filename),
    //     double (filesize),
    //     std::string (filehash)
    // >
    DUMMY_REG(InAddNewFileInCurrent,"MWI_InAddNewFileInCurrent");

    // add new file under current directory
    // Signature: <
    //     InSetCurrentFileValues,
    //     int (fileid),
    //     int (dirid),
    //     std::string (filename),
    //     double (filesize),
    //     std::string (filehash)
    // >
    DUMMY_REG(InSetCurrentFileValues,"MWI_InSetCurrentFileValues");

    // load css from file.
    // Signature: <
    //     InLoadCss,
    //     std::string (path)
    // >
    DUMMY_REG(InLoadCss,"MWI_InLoadCss");

    // query current directory id
    // Signature: < QueryCurrentDirId, int (output) >
    DUMMY_REG(QueryCurrentDirId,"MWI_QueryCurrentDirId");

    // query current directory name
    // Signature: < QueryCurrentDirName, std::string (output) >
    DUMMY_REG(QueryCurrentDirName,"MWI_QueryCurrentDirName");

    // query session widget
    // Signature: < QueryDownloadSessionWidget, StrongMsgPtr >
    DUMMY_REG(QueryDownloadSessionWidget,"MWI_QueryDownloadSessionWidget");

    // query current selected file id
    // Signature: < QueryCurrentFileId, int >
    DUMMY_REG(QueryCurrentFileId,"MWI_QueryCurrentFileId");

    // popup model
    // Show popup menu.
    // Signature:
    // < PopupMenuModel_ShowMenu, StrongMsgPtr (model) >
    DUMMY_REG(PopupMenuModel_ShowMenu,"MWI_PMM_ShowMenu");
    //
    // Query total items in menu
    // Signature:
    // < PopupMenuModel_QueryCount, int (out) >
    DUMMY_REG(PopupMenuModel_QueryCount,"MWI_PMM_QueryCount");
    //
    // Query item.
    // Siganture:
    // < PopupMenuModel_QueryItem, int (which), std::string (out) >
    DUMMY_REG(PopupMenuModel_QueryItem,"MWI_PMM_QueryItem");
    //
    // Emitted when item is selected.
    // Signature:
    // < PopupMenuModel_OutSelected, int (out) >
    DUMMY_REG(PopupMenuModel_OutSelected,"MWI_PMM_OutSelected");
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
                MMI::InLoadFolderTree,
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
    registerAndGetWidget(gladeName,memberName);  \
    memberName->signal_clicked().connect(   \
        sigc::mem_fun(*this,funcName)       \
    );

struct GtkMainWindow : public SafeLists::GenericGtkWidget {

    typedef MainWindowInterface MWI;

    GtkMainWindow(Glib::RefPtr<Gtk::Builder>& bld) :
        SafeLists::GenericGtkWidget(bld,"mainAppWindow"),
        _builder(bld),
        _left(nullptr),
        _right(nullptr),
        _lastSelectedDirId(-1)
    {
        regHandler(genHandler());

        registerAndGetWidget("mainAppWindow",_wnd);
        registerAndGetWidget("fileList",_right);
        registerAndGetWidget("dirList",_left);
        registerAndGetWidget("statusBarLabel",_statusBar);
        registerAndGetWidget("downloadStatusLabel",_downloadLabel);
        registerAndGetWidget("safelistRevisionLabel",_safelistRevisionLbl);
        registerAndGetWidget("reavealerSessions",_revealerSessions);
        registerAndGetWidget("resumeDownloadButton",_resumeDownloadButton);
        registerAndGetWidget("resumeDownloadButton",_resumeDownloadButton);
        BIND_GTK_BUTTON("downloadButton",
            _dlSafelistBtn,
            &GtkMainWindow::downloadButtonClicked);
        BIND_GTK_BUTTON("openSafelistButton",
            _openSafelistBtn,
            &GtkMainWindow::openSafelistClicked);
        BIND_GTK_BUTTON("resumeDownloadButton",
            _resumeDownloadButton,
            &GtkMainWindow::resumeDownloadClicked);
        BIND_GTK_BUTTON("showDownloadsButton",
            _showDownloadsBtn,
            &GtkMainWindow::showDownloadsButtonClicked);
        BIND_GTK_BUTTON("createSafelistButton",
            _createSafelistBtn,
            &GtkMainWindow::createSafelistClicked);

        _sessionTab = SafeLists::GtkSessionTab::makeNew();
        _revealerSessions->add(*_sessionTab->getTabs());

        _wnd->signal_draw().connect(
            sigc::mem_fun(*this,&GtkMainWindow::onDraw)
        );

        _wnd->signal_delete_event().connect(
            sigc::mem_fun(*this,&GtkMainWindow::onCloseEvent)
        );

        createDirModel();
        createFileModel();

        _selectionStack.resize(2);
    }

    bool onCloseEvent(GdkEventAny* ev) {
        if (nullptr != _shutdownGuard) {
            _shutdownGuard->waitAll();
        }
        return false;
    }

    void setShutdownGuard(const std::shared_ptr< SafeLists::GracefulShutdownGuard >& guard) {
        _shutdownGuard = guard;
    }

    template <class T>
    void registerAndGetWidget(const char* name,T*& out) {
        _builder->get_widget(name,out);
        _wgtMap.insert(std::pair<std::string,Gtk::Widget*>(name,out));
    }

    Gtk::Widget* retrieveWidget(const char* name) {
        auto found = _wgtMap.find(name);
        if (found != _wgtMap.end()) {
            return found->second;
        }
        return nullptr;
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
        messageNonVirtual(msg);
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

    void cloneDirSubTree(
        const Glib::RefPtr<Gtk::TreeStore>& store,
        const Gtk::TreeModel::iterator& from,
        const Gtk::TreeModel::iterator& to)
    {
        DirRow dr;
        getDirRow(dr,*from);
        setDirRow(dr,*to);
        auto toChildren = to->children();
        auto children = from->children();
        auto end = children.end();
        for (auto b = children.begin(); b != end; ++b) {
            auto iter = store->append(toChildren);
            cloneDirSubTree(store,b,iter);
        }
    }

    VmfPtr genHandler() {
        typedef GenericMessageableInterface GMI;
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
            SF::virtualMatch< MWI::InSetDownloadText, const std::string >(
                [=](MWI::InSetDownloadText,const std::string& text) {
                    this->_downloadLabel->set_text(text.c_str());
                }
            ),
            SF::virtualMatch< MWI::InSetWidgetEnabled, const std::string, const bool >(
                [=](MWI::InSetWidgetEnabled,const std::string& name,bool val) {
                    auto wgt = this->retrieveWidget(name.c_str());
                    if (nullptr != wgt) {
                        wgt->set_sensitive(val);
                    } else {
                        assert( false && "You did not just drop a no name widget on me bro..." );
                    }
                }
            ),
            SF::virtualMatch< MWI::InSetWidgetText, const std::string, const std::string >(
                [=](MWI::InSetWidgetText,const std::string& name,const std::string& text) {
                    auto wgt = this->retrieveWidget(name.c_str());
                    if (nullptr != wgt) {
                        auto downcast = dynamic_cast< Gtk::Label* >(wgt);
                        if (nullptr != downcast) {
                            downcast->set_text(text.c_str());
                        } else {
                            assert( false && "No downcast pookie..." );
                        }
                    } else {
                        assert( false && "You did not just drop a no name widget on me bro..." );
                    }
                }
            ),
            SF::virtualMatch< MWI::InSetCurrentDirName, const std::string >(
                [=](MWI::InSetWidgetText,const std::string& name) {
                    auto selection = _dirSelection->get_selected();
                    if (nullptr != selection) {
                        auto row = *selection;
                        row[_dirColumns.m_colName] = name.c_str();
                    } else {
                        assert( false && "Setting current name when not selected." );
                    }
                }
            ),
            SF::virtualMatch< MWI::InAddChildUnderCurrentDir, const std::string, const int >(
                [=](MWI::InAddChildUnderCurrentDir,const std::string& name,int id) {
                    auto selection = _dirSelection->get_selected();
                    if (nullptr != selection) {
                        DirRow d;
                        d._id = id;
                        d._parent = (*selection)[_dirColumns.m_colId];
                        d._name = name;
                        auto newOne = _dirStore->append(selection->children());
                        setDirRow(d,*newOne);
                    } else {
                        assert( false && "Setting current name when not selected." );
                    }
                }
            ),
            SF::virtualMatch< MWI::InClearCurrentFiles >(
                [=](ANY_CONV) {
                    _fileStore->clear();
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
            SF::virtualMatch< MWI::InAddNewFileInCurrent,
                const int, const int, const std::string,
                const double, const std::string
            >(
                [=](MWI::InAddNewFileInCurrent,
                    int fileId, int dirId, const std::string& filename,
                    double fileSize, const std::string& hash
                ) {
                    auto newIter = _fileStore->append();
                    auto newRow = *newIter;
                    newRow[_fileColumns.m_fileId] = fileId;
                    newRow[_fileColumns.m_dirId] = dirId;
                    newRow[_fileColumns.m_fileName] = filename;
                    newRow[_fileColumns.m_fileSize] = static_cast<int64_t>(fileSize);
                    newRow[_fileColumns.m_fileHash] = hash;
                }
            ),
            SF::virtualMatch< MWI::InSetCurrentFileValues,
                const int, const int, const std::string,
                const double, const std::string
            >(
                [=](MWI::InAddNewFileInCurrent,
                    int fileId, int dirId, const std::string& filename,
                    double fileSize, const std::string& hash
                ) {
                    auto selection = _fileSelection->get_selected();
                    if (nullptr != selection) {
                        auto theRow = *selection;
                        theRow[_fileColumns.m_fileId] = fileId;
                        theRow[_fileColumns.m_dirId] = dirId;
                        theRow[_fileColumns.m_fileName] = filename;
                        theRow[_fileColumns.m_fileSize] = static_cast<int64_t>(fileSize);
                        theRow[_fileColumns.m_fileHash] = hash;
                    } else {
                        assert( false && "Iter fail..." );
                    }
                }
            ),
            SF::virtualMatch< MWI::InLoadCss, std::string >(
                [=](ANY_CONV,const std::string& path) {
                    auto css = Gtk::CssProvider::create();
                    css->load_from_path(path);
                    auto screen = Gdk::Screen::get_default();
                    auto ctx = _wnd->get_style_context();
                    ctx->add_provider_for_screen(
                        screen,
                        css,
                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                }
            ),
            SF::virtualMatch< MWI::QueryCurrentFileId, int >(
                [=](MWI::QueryCurrentFileId,int& outId) {
                    auto selection = _fileSelection->get_selected();
                    if (nullptr != selection) {
                        auto row = *selection;
                        outId = row[_fileColumns.m_fileId];
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

                    auto newPlace = _dirStore->append(parent->children());
                    cloneDirSubTree(_dirStore,toMove,newPlace);
                    _dirStore->erase(toMove);
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
                    this->addToNotify(ptr);
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
            SF::virtualMatch< MWI::PopupMenuModel_ShowMenu, StrongMsgPtr >(
                [=](MWI::PopupMenuModel_ShowMenu,const StrongMsgPtr& model) {
                    this->showPopupMenu(model);
                }
            ),
            SF::virtualMatch< GenericGtkWindowInterface::GetGtkWindow, Gtk::Window* >(
                [=](GenericGtkWindowInterface::GetGtkWindow,Gtk::Window*& ptr) {
                    ptr = this->_wnd;
                }
            )
        );
    }

    void showPopupMenu(const StrongMsgPtr& model) {
        auto count = SF::vpack< MWI::PopupMenuModel_QueryCount, int >(nullptr,-1);
        model->message(count);
        assert( count.useCount() > 0 );

        this->_popupModel = model;

        auto item = SF::vpack< MWI::PopupMenuModel_QueryItem, int, std::string>(
            nullptr, -1, "");

        int total = count.fGet<1>();
        assert( total > 0 && "Menu should be shown with something..." );

        auto children = _popupMenu.get_children();
        TEMPLATIOUS_FOREACH(auto& i,children) {
            _popupMenu.remove(*i);
        }

        TEMPLATIOUS_0_TO_N(i,total) {
            int prevCount = item.useCount();
            item.fGet<1>() = i;
            model->message(item);
            assert( item.useCount() > prevCount );

            auto managed = Gtk::manage(
                new Gtk::MenuItem(item.fGet<2>().c_str(),true));
            managed->set_data("index",reinterpret_cast<void*>(i));
            managed->signal_activate().connect(
                    sigc::mem_fun(*this,&GtkMainWindow::selectedItem));
            _popupMenu.append(*managed);
        }

        _popupMenu.accelerate(*_wnd);
        _popupMenu.show_all();
        _popupMenu.popup(3,gtk_get_current_event_time());
    }

    void selectedItem() {
        assert( nullptr != _popupModel && "Caught me off guard bro..." );

        auto selected = _popupMenu.get_active();
        auto data = selected->get_data("index");
        int idx = *reinterpret_cast<int*>(&data);
        auto msg = SF::vpack< MWI::PopupMenuModel_OutSelected, int >(nullptr,idx);
        _popupModel->message(msg);
        _popupModel = nullptr;
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

    void setDirRow(const DirRow& r,const Gtk::TreeModel::Row& mdlRow) {
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
                this->messageNonVirtual(pack);
            }
        );
        auto msg = SF::vpack< MWI::OutDrawEnd >(nullptr);
        notifyObservers(msg);
        _shutdownGuard->processMessages();
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
            notifyObservers(msg);
        }
    }

    void fileToViewChanged() {
        auto iter = _fileSelection->get_selected();
        if (nullptr != iter) {
            auto row = *iter;
            int id = row[_fileColumns.m_fileId];
            _lastSelectedFileId = id;
        }
    }

    template <class... Types,class... Args>
    void notifySingleThreaded(Args&&... args) {
        auto msg = SF::vpack<Types...>(
            std::forward<Args>(args)...
        );
        notifyObservers(msg);
    }

    bool leftListClicked(GdkEventButton* event) {
        if ( (event->type == GDK_BUTTON_PRESS) && (event->button) == 3 ) {
            notifySingleThreaded< MainWindowInterface::OutRightClickFolderList >(nullptr);
            return true;
        }

        return false;
    }

    bool leftFileListClicked(GdkEventButton* event) {
        if ( (event->type == GDK_BUTTON_PRESS) && (event->button) == 3 ) {
            notifySingleThreaded< MainWindowInterface::OutRightClickFileList >(nullptr);
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
        _right->signal_button_press_event().connect(
            sigc::mem_fun(*this,&GtkMainWindow::leftFileListClicked),false);
        _right->set_model(_fileStore);

        _fileSelection = _right->get_selection();
        _fileSelection->set_mode(Gtk::SELECTION_SINGLE);
        _fileSelection->signal_changed().connect(sigc::mem_fun(
            *this,&GtkMainWindow::fileToViewChanged));
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

    void createSafelistClicked() {
        notifySingleThreaded<
            MWI::OutCreateSafelistButtonClicked
        >(nullptr);
    }

    void resumeDownloadClicked() {
        notifySingleThreaded<
            MWI::OutResumeDownloadButtonClicked
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

    // what a mess... what can I do,
    // I WAS YOUNG!
    Glib::RefPtr< Gtk::Builder > _builder;

    Gtk::Window* _wnd;
    Gtk::TreeView* _left;
    Gtk::TreeView* _right;
    Gtk::Button* _openSafelistBtn;
    Gtk::Button* _createSafelistBtn;
    Gtk::Button* _dlSafelistBtn;
    Gtk::Button* _resumeDownloadButton;
    Gtk::ToggleButton* _showDownloadsBtn;
    Gtk::Label* _statusBar;
    Gtk::Label* _downloadLabel;
    Gtk::Label* _safelistRevisionLbl;
    Gtk::Revealer* _revealerSessions;
    ModelColumns _mdl;

    std::map< std::string, Gtk::Widget* > _wgtMap;

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
    Glib::RefPtr<Gtk::TreeSelection> _fileSelection;

    // Menu
    Gtk::Menu _popupMenu;
    StrongMsgPtr _popupModel;

    // STATE
    int _lastSelectedDirId;
    int _lastSelectedFileId;
    // should contain two elements always
    std::vector<Gtk::TreeModel::iterator> _selectionStack;
    std::shared_ptr< SafeLists::GtkSessionTab > _sessionTab;
    std::weak_ptr< GtkMainWindow > _myself;
    std::weak_ptr<DrawUpdater> _drawUpdater;
    std::shared_ptr< SafeLists::GracefulShutdownGuard > _shutdownGuard;
};

struct GtkGenericDialogMessages {
    // Show the dialog
    // in lua: INDLG_InShowDialog
    DUMMY_REG(InShowDialog,"INDLG_InShowDialog");

    // Show the dialog
    // in lua: INDLG_InHideDialog
    DUMMY_REG(InHideDialog,"INDLG_InHideDialog");

    // Set notifier to notify messages
    // in lua: INDLG_InSetNotifier
    // Signature: < InSetNotifier, StrongMsgPtr >
    DUMMY_REG(InSetNotifier,"INDLG_InSetNotifier");

    // Set label text
    // in lua: INDLG_InSetLabel
    // Signature: < InSetLabel, std::string (if only one label) >
    // Signature: < InSetLabel,
    //     std::string (label name from glade),
    //     std::string (the value)
    // >
    DUMMY_REG(InSetLabel,"INDLG_InSetLabel");

    // Set value text
    // in lua: INDLG_InSetValue
    // Signature: < InSetValue, std::string (if only one value) >
    // Signature: < InSetValue,
    //     std::string (value name from glade),
    //     std::string (the value)
    // >
    DUMMY_REG(InSetValue,"INDLG_InSetValue");

    // Hook onto button click and return signal integer
    // Signature: <
    //     InHookButtonClick, std::string (name),
    //     int (out number)
    // >
    DUMMY_REG(InHookButtonClick,"INDLG_HookButtonClick");

    // Receive focus
    // Signature: < InAlwaysAbove >
    DUMMY_REG(InAlwaysAbove,"INDLG_InAlwaysAbove");

    // Set control enabled.
    // Signature:
    // < InSetControlEnabled, std::string (name), bool (value) >
    DUMMY_REG(InSetControlEnabled,"INDLG_InSetControlEnabled");

    // Emitted when ok button is clicked
    DUMMY_REG(OutOkClicked,"INDLG_OutOkClicked");

    // Emitted when cancel button is clicked
    DUMMY_REG(OutCancelClicked,"INDLG_OutCancelClicked");

    // Emitted when numbered event is emitted.
    // Signature: < OutSignalEmitted, int (signal) >
    DUMMY_REG(OutSignalEmitted,"INDLG_OutGenSignalEmitted");

    // Emitted when dialog is exited
    DUMMY_REG(OutDialogExited,"INDLG_OutDialogExited");

    // Query input text
    // in lua: INDLG_InQueryInput
    // Signature: < QueryInput, std::string (out) >
    DUMMY_REG(QueryInput,"INDLG_QueryInput");
};

struct GenericDialog : public Messageable {

    GenericDialog() = delete;
    GenericDialog(const GenericDialog&) = delete;
    GenericDialog(GenericDialog&&) = delete;

    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    void message(templatious::VirtualPack& msg) override {
        _handler->tryMatch(msg);
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "Single threaded messaging only." );
    }

    GenericDialog(
        const Glib::RefPtr< Gtk::Builder>& ref,
        const char* mainName) :
        _bldPtr(ref),
        _handler(genHandler()),
        _signalId(0)
    {
        _bldPtr->get_widget(mainName,_main);
        _main->signal_delete_event().connect(
            sigc::mem_fun(*this,&GenericDialog::exitEvent)
        );
    }

private:
    template <class... Args,class... Constr>
    void notifySingleThreaded(Constr&&... args) {
        auto locked = _toNotify.lock();
        if (nullptr != locked) {
            auto msg = SF::vpack< Args... >(
                std::forward<Constr>(args)...);
            locked->message(msg);
        }
    }

    void intEvent(int i) {
        notifySingleThreaded< Int::OutSignalEmitted, int >(
            nullptr,i);
    }

    bool exitEvent(GdkEventAny* ev) {
        notifySingleThreaded< Int::OutDialogExited >(nullptr);
        return false;
    }

    typedef GtkGenericDialogMessages Int;

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< Int::InShowDialog >(
                [=](Int::InShowDialog) {
                    _main->show();
                }
            ),
            SF::virtualMatch< Int::InHideDialog >(
                [=](Int::InHideDialog) {
                    _main->hide();
                }
            ),
            SF::virtualMatch< Int::InSetNotifier, StrongMsgPtr >(
                [=](Int::InSetNotifier,const StrongMsgPtr& ptr) {
                    _toNotify = ptr;
                }
            ),
            SF::virtualMatch< Int::InSetLabel, std::string, std::string >(
                [=](Int::InSetLabel,
                    const std::string& name,
                    const std::string& value)
                {
                    Gtk::Label* lbl = nullptr;
                    _bldPtr->get_widget(name.c_str(),lbl);
                    assert( nullptr != lbl && "Huh?" );
                    lbl->set_text(value.c_str());
                }
            ),
            SF::virtualMatch< Int::InSetValue, const std::string, const std::string >(
                [=](Int::InSetValue,
                    const std::string& name,
                    const std::string& value)
                {
                    Gtk::Widget* wgt = nullptr;
                    _bldPtr->get_widget(name.c_str(),wgt);
                    assert( nullptr != wgt && "Huh?" );
                    Gtk::Entry* isEntry =
                        dynamic_cast< Gtk::Entry* >(wgt);
                    if (nullptr != isEntry) {
                        isEntry->set_text(value.c_str());
                        return;
                    }

                    Gtk::TextView* isTextView =
                        dynamic_cast< Gtk::TextView* >(wgt);
                    if (nullptr != isTextView) {
                        isTextView->get_buffer()->set_text(
                            value.c_str());
                        return;
                    }

                    assert( false && "Dunno how to set text for this cholo." );
                }
            ),
            SF::virtualMatch< Int::QueryInput, const std::string, std::string >(
                [=](Int::QueryInput,
                    const std::string& name,
                    std::string& value)
                {
                    Gtk::Widget* wgt = nullptr;
                    _bldPtr->get_widget(name.c_str(),wgt);
                    assert( nullptr != wgt && "Huh?" );
                    Gtk::Entry* isEntry =
                        dynamic_cast< Gtk::Entry* >(wgt);
                    if (nullptr != isEntry) {
                        value = isEntry->get_text().c_str();
                        isEntry->set_text(value.c_str());
                        return;
                    }

                    Gtk::TextView* isTextView =
                        dynamic_cast< Gtk::TextView* >(wgt);
                    if (nullptr != isTextView) {
                        auto buf = isTextView->get_buffer();
                        value = buf->get_text(buf->begin(),buf->end()).c_str();
                        return;
                    }

                    assert( false && "Dunno how to set text for this cholo." );
                }
            ),
            SF::virtualMatch< Int::InHookButtonClick, const std::string, int >(
                [=](Int::InHookButtonClick,const std::string& str,int& out) {
                    Gtk::Button* button = nullptr;
                    _bldPtr->get_widget(str.c_str(),button);
                    assert( nullptr != button && "Cannot find button to hook into." );

                    int theSignal = _signalId;
                    ++_signalId;

                    button->signal_clicked().connect(
                        sigc::bind< int >(
                            sigc::mem_fun(
                                *this,&GenericDialog::intEvent
                            ),theSignal)
                    );

                    out = theSignal;
                }
            ),
            SF::virtualMatch< Int::InAlwaysAbove >(
                [=](Int::InAlwaysAbove) {
                    auto dlg = dynamic_cast< Gtk::Dialog* >(_main);
                    assert( nullptr != dlg && "Not a dialog." );
                    dlg->set_keep_above(true);
                }
            ),
            SF::virtualMatch< Int::InSetControlEnabled, const std::string, bool >(
                [=](Int::InSetControlEnabled,const std::string& name,bool val) {
                    Gtk::Widget* wgt = nullptr;
                    _bldPtr->get_widget(name.c_str(),wgt);
                    assert( nullptr != wgt && "Cant pull widget." );
                    wgt->set_sensitive(val);
                }
            ),
            SF::virtualMatch< GenericGtkWindowInterface::GetGtkWindow, Gtk::Window* >(
                [=](GenericGtkWindowInterface::GetGtkWindow,Gtk::Window*& ptr) {
                    ptr = static_cast<Gtk::Window*>(_main);
                }
            )
        );
    }

    VmfPtr _handler;
    Glib::RefPtr< Gtk::Builder > _bldPtr;
    WeakMsgPtr _toNotify;
    Gtk::Widget* _main;
    int _signalId;
};

const Glib::ustring& mainUiSchema() {
    static Glib::ustring res = SafeLists::readFile("appdata/uischemes/main.glade");
    return res;
}

const Glib::ustring& licensingUiSchema() {
    static Glib::ustring res = SafeLists::readFile("appdata/uischemes/licensing.glade");
    return res;
}

const Glib::ustring& dialogsSchema() {
    static Glib::ustring res = SafeLists::readFile("appdata/uischemes/dialogs.glade");
    return res;
}

struct GtkDialogService : public Messageable {
    GtkDialogService() : _handler(genHandler()) {
        _schemaMap["main"] = &mainUiSchema();
        _schemaMap["licensing"] = &licensingUiSchema();
        _schemaMap["dialogs"] = &dialogsSchema();
    }

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

    // Open directory chooser
    // Signature:
    // <
    //  DirChooserDialog,
    //  StrongMsgPtr (parent window),
    //  std::string (title),
    //  std::string (output path, empty if none)
    // >
    DUMMY_REG(DirChooserDialog,"GDS_DirChooserDialog");

    // Open file saver dialog
    // Signature:
    // <
    //  FileSaverDialog,
    //  StrongMsgPtr (parent window),
    //  std::string (title),
    //  std::string (output path)
    // >
    DUMMY_REG(FileSaverDialog,"GDS_FileSaverDialog");

    // Show alert
    // Signature:
    // < AlertDialog, std::String (title), std::string (message) >
    DUMMY_REG(AlertDialog,"GDS_AlertDialog");

    // Show dialog with Ok Cancel options
    // Signature:
    // < OkCancelDialog,
    //  StrongMsgPtr (parent),
    //  std::string (title),
    //  std::string (message)
    //  int (outres) >
    // outres -> 0: ok, 1: cancel, -1: none
    DUMMY_REG(OkCancelDialog,"GDS_OkCancelDialog");

    // DEPRECATED
    // make generic dialog which is hookable
    // and setable with buttons, etc.
    // Signature:
    // < MakeGenericDialog,
    //     std::string (resource),
    //     std::string (widget name),
    //     StrongMsgPtr (output)
    // >
    DUMMY_REG(MakeGenericDialog,"GDS_MakeGenericDialog");

    // make generic widget.
    // Signature:
    // < MakeGenericWidget,
    //     std::string (resource),
    //     std::string (widget name),
    //     StrongMsgPtr (output)
    // >
    DUMMY_REG(MakeGenericWidget,"GDS_MakeGenericWidget");

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
                [](FileChooserDialog,
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
            ),
            SF::virtualMatch<
                DirChooserDialog,
                StrongMsgPtr,
                const std::string,
                std::string
            >(
                [](DirChooserDialog,
                   const StrongMsgPtr& window,
                   const std::string& title,
                   std::string& out)
                {
                    Gtk::FileChooserDialog dlg(title.c_str(),
                        Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
                    auto queryTransient = SF::vpack<
                        GenericGtkWindowInterface::GetGtkWindow,
                        Gtk::Window*
                    >(nullptr,nullptr);
                    window->message(queryTransient);
                    assert( queryTransient.useCount() > 0 && "No transient cholo..." );
                    dlg.set_transient_for(*queryTransient.fGet<1>());
                    dlg.add_button("_Cancel",Gtk::RESPONSE_CANCEL);
                    dlg.add_button("Ok",Gtk::RESPONSE_OK);

                    int result = dlg.run();
                    if (Gtk::RESPONSE_OK == result) {
                        out = dlg.get_filename();
                        return;
                    }

                    out = "";
                }
            ),
            SF::virtualMatch<
                FileSaverDialog,
                StrongMsgPtr,
                std::string,
                std::string
            >([](FileSaverDialog,
                 const StrongMsgPtr& window,
                 const std::string& title,
                 std::string& out)
                {
                    Gtk::FileChooserDialog dlg(title.c_str(),
                        Gtk::FILE_CHOOSER_ACTION_SAVE);
                    auto queryTransient = SF::vpack<
                        GenericGtkWindowInterface::GetGtkWindow,
                        Gtk::Window*
                    >(nullptr,nullptr);
                    window->message(queryTransient);
                    assert( queryTransient.useCount() > 0
                            && "No transient cholo..." );
                    dlg.set_transient_for(*queryTransient.fGet<1>());
                    dlg.add_button("_Cancel",Gtk::RESPONSE_CANCEL);
                    dlg.add_button("Ok",Gtk::RESPONSE_OK);

                    int result = dlg.run();
                    if (Gtk::RESPONSE_OK == result) {
                        out = dlg.get_filename();
                        return;
                    }

                    out = "";
                }
            ),
            SF::virtualMatch< AlertDialog, StrongMsgPtr,
                 const std::string,const std::string
            >(
                [](AlertDialog,
                    const StrongMsgPtr& parent,
                    const std::string& title,
                    const std::string& message)
                {
                    auto queryTransient = SF::vpack<
                        GenericGtkWindowInterface::GetGtkWindow,
                        Gtk::Window*
                    >(nullptr,nullptr);
                    parent->message(queryTransient);
                    auto gtkParent = queryTransient.fGet<1>();
                    Gtk::MessageDialog dlg(
                        *gtkParent,title.c_str());
                    dlg.set_message(message.c_str());
                    dlg.run();
                }
            ),
            SF::virtualMatch<
                MakeGenericDialog,
                const std::string,
                const std::string,
                StrongMsgPtr
            >(
                [=](MakeGenericDialog,
                    const std::string& resource,
                    const std::string& widgetName,
                    StrongMsgPtr& outPtr)
                {
                    auto iter = _schemaMap.find(resource);
                    assert( iter != _schemaMap.end() && "No resource." );
                    auto& string = *iter->second;
                    auto bld = Gtk::Builder::create_from_string(
                        string,widgetName.c_str());

                    auto dialog = std::make_shared< GenericDialog >(
                        bld,widgetName.c_str());
                    outPtr = dialog;
                }
            ),
            SF::virtualMatch<
                MakeGenericWidget,
                const std::string,
                const std::string,
                StrongMsgPtr
            >(
                [=](MakeGenericDialog,
                    const std::string& resource,
                    const std::string& widgetName,
                    StrongMsgPtr& outPtr)
                {
                    auto iter = _schemaMap.find(resource);
                    assert( iter != _schemaMap.end() && "No resource." );
                    auto& string = *iter->second;
                    auto bld = Gtk::Builder::create_from_string(
                        string,widgetName.c_str());

                    auto dialog = std::make_shared<
                        SafeLists::GenericGtkWidget >(
                            bld,widgetName.c_str());
                    outPtr = dialog;
                }
            ),
            SF::virtualMatch<
                OkCancelDialog,
                StrongMsgPtr,
                const std::string,
                const std::string,
                int
            >(
                [](OkCancelDialog,
                   const StrongMsgPtr& parent,
                   const std::string& title,
                   const std::string& message,
                   int& out)
                {
                    auto queryTransient = SF::vpack<
                        GenericGtkWindowInterface::GetGtkWindow,
                        Gtk::Window*
                    >(nullptr,nullptr);
                    parent->message(queryTransient);
                    auto gtkParent = queryTransient.fGet<1>();
                    Gtk::MessageDialog dlg(
                        title.c_str(),gtkParent);
                    dlg.set_secondary_text(message.c_str());
                    dlg.add_button("_Cancel",1);
                    dlg.set_default_response(-1);
                    int outRes = dlg.run();
                    if (outRes == Gtk::RESPONSE_OK) {
                        outRes = 0;
                    }
                    out = outRes;
                }
            )
        );
    }

private:
    SafeLists::VmfPtr _handler;
    std::map< std::string, const Glib::ustring* > _schemaMap;
};

struct GtkInputDialog : public Messageable {

    GtkInputDialog(Glib::RefPtr<Gtk::Builder>& bld) : _handler(genHandler()) {
        Gtk::Dialog* dlg = nullptr;
        bld->get_widget("dialog2",_dlg);

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

        auto msg = SF::vpack< GtkGenericDialogMessages::OutOkClicked >(nullptr);
        locked->message(msg);
    }

    void cancelClicked() {
        auto locked = _toNotify.lock();
        if (nullptr == locked) {
            return;
        }

        auto msg = SF::vpack< GtkGenericDialogMessages::OutCancelClicked >(nullptr);
        locked->message(msg);
    }

    VmfPtr genHandler() {
        typedef GtkGenericDialogMessages INT;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< INT::InShowDialog, bool >(
                [=](INT::InShowDialog,bool show) {
                    if (show) {
                        _dlg->set_modal(true);
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
            SF::virtualMatch< INT::InSetValue, const std::string >(
                [=](INT::InSetLabel,const std::string& str) {
                    this->_entry->set_text(str.c_str());
                }
            ),
            SF::virtualMatch< INT::QueryInput, std::string >(
                [=](INT::InSetLabel,std::string& str) {
                    str = this->_entry->get_text().c_str();
                }
            )
        );
    }

    Gtk::Dialog* _dlg;
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

std::string xdgCustomDir() {
    auto execPath = SafeLists::executablePath();
    execPath += "/appdata/usr/share";
    return execPath;
}

void uniformSlashes(char* path) {
    int len = strlen(path);
    TEMPLATIOUS_0_TO_N(i,len) {
        if (path[i] == '\\') {
            path[i] = '/';
        }
    }
}

void prepEnv(
    std::vector< std::unique_ptr<char[]> >& envVec,
    int argc,char** argv)
{
    SafeLists::setGlobalProgramArgs(argc,argv);

    auto catEnv =
        [&](const char* str) {
            int len = ::strlen(str);
            auto buff = std::unique_ptr< char[] >( new char[len] );
            ::strcpy( buff.get(), str );
            SA::add( envVec, std::move(buff) );
        };

    char arr[1024];

    arr[0] = '\0';
    // don't trust default rendering,
    // fails on linux, nothing can
    // be as accurate as CPU drawn pixels.
    ::strcat(arr,"GDK_RENDERING=image");
    catEnv(arr);

    std::string customDir = xdgCustomDir();

    arr[0] = '\0';
    ::strcat(arr,"XDG_DATA_DIRS=");
    ::strcat(arr,customDir.c_str());
    auto currDirs = ::getenv("XDG_DATA_DIRS");
    if (nullptr != currDirs) {
#ifdef __linux__
        ::strcat(arr,":");
#elif defined(__MINGW32__)
        ::strcat(arr,";");
#else
        static_assert( false, "Platform not supported yet." );
#endif
        ::strcat(arr,currDirs);
    }

    uniformSlashes(arr);
    catEnv(arr);

    arr[0] = '\0';
    ::strcat(arr,"GSETTINGS_SCHEMA_DIR=");
    ::strcat(arr,customDir.c_str());
    ::strcat(arr,"/glib-2.0/schemas/");
    uniformSlashes(arr);
    catEnv(arr);

    TEMPLATIOUS_FOREACH(auto& i,envVec) {
        ::putenv(i.get());
    }
}

void gtkSpec() {
    char arr[1024];
    arr[0] = '\0';

    std::string customDir = xdgCustomDir();
    ::strcat(arr,customDir.c_str());
    ::strcat(arr,"/icons/");
    uniformSlashes(arr);

    auto def = ::gtk_icon_theme_get_default();
    ::gtk_icon_theme_prepend_search_path(def,arr);
}

int main(int argc,char** argv) {
    std::vector< std::unique_ptr<char[]> > envVars;
    prepEnv(envVars,argc,argv);

    auto app = Gtk::Application::create(argc,argv);

    gtkSpec();

    auto ctx = LuaContext::makeContext("lua/plumbing.lua");
    ctx->setFactory(vFactory());

    auto downloader = SafeLists::AsyncDownloader::createNew("imitation");
    auto randomFileWriter = SafeLists::RandomFileWriter::make();
    auto licenseService = SafeLists::LicenseDaemon::getDaemon();

    auto dlFactory = SafeLists::
        SafeListDownloaderFactory::createNew(downloader,randomFileWriter);

    auto builder = Gtk::Builder::create();
    builder->add_from_string(mainUiSchema());
    auto mainWnd = std::make_shared< GtkMainWindow >(builder);
    GtkMainWindow::spinUpdater(mainWnd);
    auto singleInputDialog = std::make_shared< GtkInputDialog >(builder);
    auto asyncSqliteFactory = SafeLists::AsyncSqliteFactory::createNew();
    auto mainModel = std::make_shared< MainModel >();
    auto dialogService = std::make_shared< GtkDialogService >();
    auto shutdownGuard = SafeLists::GracefulShutdownGuard::makeNew();
    shutdownGuard->add(randomFileWriter);
    shutdownGuard->add(downloader);
    shutdownGuard->add(licenseService);

    mainWnd->setShutdownGuard(shutdownGuard);
    ctx->addMessageableWeak("mainWindow",mainWnd);
    ctx->addMessageableWeak("singleInputDialog",singleInputDialog);
    ctx->addMessageableWeak("mainModel",mainModel);
    ctx->addMessageableWeak("shutdownGuard",shutdownGuard);
    ctx->addMessageableWeak("randomFileWriter",randomFileWriter);
    ctx->addMessageableWeak("licenseService",licenseService);
    ctx->addMessageableStrong("dlSessionFactory",dlFactory);
    ctx->addMessageableStrong("asyncSqliteFactory",asyncSqliteFactory);
    ctx->addMessageableStrong("dialogService",dialogService);
    ctx->doFile("lua/main.lua");
    app->run(mainWnd->getWindow(),argc,argv);
}

