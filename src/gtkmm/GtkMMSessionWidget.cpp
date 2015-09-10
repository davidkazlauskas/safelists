
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

    static std::mutex& getMutexSessions() {
        static std::mutex mtx;
        return mtx;
    }

    static std::vector< Glib::RefPtr< Gtk::Builder > >& getCacheSessions() {
        static std::vector< Glib::RefPtr< Gtk::Builder > > sessions;
        return sessions;
    }

    typedef std::lock_guard< std::mutex > LGuard;

    static void cacheSession(const Glib::RefPtr<Gtk::Builder>& bld) {
        auto& vec = getCacheSessions();
        auto& mtx = getMutexSessions();

        LGuard g(mtx);
        SA::add(vec,bld);
    }

    static Glib::RefPtr<Gtk::Builder> popSession() {
        auto& vec = getCacheSessions();
        auto& mtx = getMutexSessions();

        Glib::RefPtr< Gtk::Builder > result(0);
        LGuard g(mtx);
        if (SA::size(vec) == 0)
            return result;

        result = vec.back();
        vec.pop_back();
        return result;
    }
}

namespace SafeLists {

std::shared_ptr< GtkSessionWidget > GtkSessionWidget::makeNew() {
    auto builder = Gtk::Builder::create();
    auto& schema = loadDownloaderSchemaStatic();
    builder->add_from_string(schema,"mainBox");
    std::shared_ptr< GtkSessionWidget > res(new GtkSessionWidget(builder));
    return res;
}

GtkSessionWidget::GtkSessionWidget(Glib::RefPtr<Gtk::Builder>& bld) :
    _container(bld)
{
    _container->get_widget("mainBox",_mainBox);
    _container->get_widget("sessionLabel",_sessionLabel);
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

