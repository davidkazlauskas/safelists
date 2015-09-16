
#include <fstream>
#include <mutex>

#include <templatious/FullPack.hpp>
#include <LuaPlumbing/messageable.hpp>

#include "GtkMMSessionWidget.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    static Glib::ustring loadDownloaderSchema() {
        const char* PATH = "uischemes/download_progress.glade";
        std::ifstream ifs(PATH);
        std::string theContent;
        theContent.reserve(256 * 256);
        theContent.assign(
            std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()
        );
        return theContent.c_str();
    }

    static const Glib::ustring& loadDownloaderSchemaStatic() {
        static Glib::ustring result = loadDownloaderSchema();
        return result;
    }

    struct RefBuilderCache {
        RefBuilderCache() {}
        RefBuilderCache(const RefBuilderCache&) = delete;
        RefBuilderCache(RefBuilderCache&&) = delete;

        void cacheBuilder(const Glib::RefPtr<Gtk::Builder>& bld) {
            LGuard g(_mtx);
            SA::add(_vec,bld);
        }

        Glib::RefPtr<Gtk::Builder> popBuilder() {
            Glib::RefPtr< Gtk::Builder > result(0);
            LGuard g(_mtx);
            if (SA::size(_vec) == 0)
                return result;

            result = _vec.back();
            _vec.pop_back();
            return result;
        }
    private:
        typedef std::lock_guard< std::mutex > LGuard;
        typedef std::vector< Glib::RefPtr< Gtk::Builder > > Vec;
        std::mutex _mtx;
        Vec _vec;
    };

    static RefBuilderCache& getSessionCache() {
        static RefBuilderCache cache;
        return cache;
    }

    static RefBuilderCache& getSessionTabCache() {
        static RefBuilderCache cache;
        return cache;
    }

    static RefBuilderCache& getDownloadBarCache() {
        static RefBuilderCache cache;
        return cache;
    }
}

namespace SafeLists {

std::shared_ptr< GtkDownloadItem > GtkDownloadItem::makeNew() {
    auto& dlBarCache = getDownloadBarCache();
    auto builder = dlBarCache.popBuilder();
    if (0 == builder) {
        builder = Gtk::Builder::create();
        auto& schema = loadDownloaderSchemaStatic();
        builder->add_from_string(schema,"singleDownloadWidget");
    }
    std::shared_ptr< GtkDownloadItem > res(new GtkDownloadItem(builder));
    return res;
}

GtkDownloadItem::GtkDownloadItem(Glib::RefPtr<Gtk::Builder>& bld) :
    _container(bld)
{
    _container->get_widget("downloadItemLabel",_theLabel);
    _container->get_widget("downloadItemProgressBar",_theProgressBar);
    _container->get_widget("singleDownloadWidget",_mainBox);
}

GtkDownloadItem::~GtkDownloadItem() {
    auto& dlBarCache = getDownloadBarCache();
    dlBarCache.cacheBuilder(_container);
}

Gtk::Box* GtkDownloadItem::getMainBox() {
    return _mainBox;
}

std::shared_ptr< GtkSessionWidget > GtkSessionWidget::makeNew() {
    auto& sessCache = getSessionCache();

    auto builder = sessCache.popBuilder();
    if (0 == builder) {
        builder = Gtk::Builder::create();
        auto& schema = loadDownloaderSchemaStatic();
        builder->add_from_string(schema,"mainBox");
    }
    std::shared_ptr< GtkSessionWidget > res(new GtkSessionWidget(builder));
    return res;
}

GtkSessionWidget::GtkSessionWidget(Glib::RefPtr<Gtk::Builder>& bld) :
    _container(bld)
{
    _container->get_widget("mainBox",_mainBox);
    _container->get_widget("downloadArea",_sessionList);
    _container->get_widget("sessionLabel",_sessionLabel);
}

GtkSessionWidget::~GtkSessionWidget() {
    auto& sessCache = getSessionCache();
    sessCache.cacheBuilder(_container);
}

Gtk::Box* GtkSessionWidget::getMainBox() {
    return _mainBox;
}

void GtkSessionWidget::setDownloadBoxCount(int number) {
    int diff = SA::size(_dlItems) - number;
    while (diff != 0) {
        if (diff < 0) {
            auto newOne = GtkDownloadItem::makeNew();
            SA::add(_dlItems,newOne);
            _sessionList->append(*newOne->getMainBox());
            ++diff;
        } else {
            auto& back = _dlItems.back();
            _sessionList->remove(*back->getMainBox());
            _dlItems.pop_back();
            --diff;
        }
    }
}

std::shared_ptr< GtkSessionTab > GtkSessionTab::makeNew() {
    auto& dlCache = getSessionTabCache();

    auto builder = dlCache.popBuilder();
    if (0 == builder) {
        builder = Gtk::Builder::create();
        auto& schema = loadDownloaderSchemaStatic();
        builder->add_from_string(schema,"sessionTab");
    }
    std::shared_ptr< GtkSessionTab > res(new GtkSessionTab(builder));
    return res;
}

GtkSessionTab::GtkSessionTab(Glib::RefPtr<Gtk::Builder>& bld) :
    _container(bld), _handler(genHandler())
{
    _container->get_widget("sessionTab",_mainTab);
}

void GtkSessionTab::addSession(const std::shared_ptr< GtkSessionWidget >& widget) {
    SA::add(_sessions,widget);
    auto raw = widget->getMainBox();
    _mainTab->append_page(*raw,"Some stuff");
}

void GtkSessionTab::setModel(const StrongMsgPtr& model) {
    _weakModel = model;
    fullModelUpdate();
}

void GtkSessionTab::fullModelUpdate() {
    auto locked = _weakModel.lock();
    if (nullptr == locked) {
        return;
    }

    typedef ModelInterface MI;
    auto queryCount = SF::vpack< MI::QueryCount, int >(
        nullptr, -1
    );
    locked->message(queryCount);
    assert( queryCount.useCount() > 0 && "Pack was unused..." );

    int total = queryCount.fGet<1>();
    int diff = SA::size(_sessions) - total;
    while (diff != 0) {
        if (diff < 0) {
            addSession( GtkSessionWidget::makeNew() );
            ++diff;
        } else {
            // need new method for this...
            _mainTab->remove_page(-1);
            _sessions.pop_back();
            --diff;
        }
    }


    TEMPLATIOUS_0_TO_N(i,total) {
        auto queryCurrentSessionCount = SF::vpack<
            MI::QuerySessionDownloadCount, int, int
        >(nullptr,i,-1);
        locked->message(queryCurrentSessionCount);
        assert( queryCurrentSessionCount.useCount() > 0 && "Pack was unused..." );
        int theCount = queryCurrentSessionCount.fGet<2>();
        _sessions[i]->setDownloadBoxCount(theCount);
    }
}

GtkSessionTab::~GtkSessionTab() {
    auto& dlCache = getSessionTabCache();
    dlCache.cacheBuilder(_container);
}

Gtk::Notebook* GtkSessionTab::getTabs() {
    return _mainTab;
}

void GtkSessionTab::message(const std::shared_ptr< templatious::VirtualPack >& msg) {
    assert( false && "Can't touch this." );
}

void GtkSessionTab::message(templatious::VirtualPack& msg) {
    _handler->tryMatch(msg);
}

VmfPtr GtkSessionTab::genHandler() {
    typedef GtkSessionTab::ModelInterface MI;
    return SF::virtualMatchFunctorPtr(
        SF::virtualMatch< MI::InFullUpdate >(
            [=](MI::InFullUpdate) {
                this->fullModelUpdate();
            }
        )
    );
}

}

