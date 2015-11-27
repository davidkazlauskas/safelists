
#include <fstream>
#include "GtkMMFileString.hpp"

namespace SafeLists {
    
Glib::ustring readFile(const char* path) {
    std::ifstream ifs(path);
    std::string theContent;
    theContent.reserve(256 * 256);
    theContent.assign(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );
    return theContent.c_str();
}

}

