
#include <fstream>
#include <mutex>

#include <templatious/FullPack.hpp>

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

    static RefBuilderCache& getDownloadBarCache() {
        static RefBuilderCache cache;
        return cache;
    }
}

namespace SafeLists {

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
    _container->get_widget("sessionLabel",_sessionLabel);
}

GtkSessionWidget::~GtkSessionWidget() {
    auto& sessCache = getSessionCache();
    sessCache.cacheBuilder(_container);
}

std::shared_ptr< GtkSessionTab > GtkSessionTab::makeNew() {
    auto builder = Gtk::Builder::create();
    auto& schema = loadDownloaderSchemaStatic();
    builder->add_from_string(schema,"sessionTab");
    std::shared_ptr< GtkSessionTab > res(new GtkSessionTab(builder));
    return res;
}

GtkSessionTab::GtkSessionTab(Glib::RefPtr<Gtk::Builder>& bld) :
    _container(bld)
{
    _container->get_widget("sessionTab",_mainTab);
}

Gtk::Notebook* GtkSessionTab::getTabs() {
    return _mainTab;
}

}

