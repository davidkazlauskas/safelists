
#include <fstream>

#include "GtkMMSessionWidget.hpp"

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
}

namespace SafeLists {

std::shared_ptr< GtkSessionWidget > GtkSessionWidget::makeNew() {
    auto builder = Gtk::Builder::create();
    static auto schema = loadDownloaderSchema();
    builder->add_from_string(schema);
    return nullptr;
}

}

