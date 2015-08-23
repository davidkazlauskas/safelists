#include <map>

#include "AutoReg.hpp"

namespace {

typedef std::map<std::string,const templatious::TypeNode*> TNodeMap;

static TNodeMap& getMap() {
    static TNodeMap map;
    return map;
}

}

namespace SafeLists {

static int registerTypeInMap(const char* name,const templatious::TypeNode* node) {
    auto& m = getMap();

    return 7;
}

}
