#include <map>

#include "AutoReg.hpp"

namespace {

typedef std::map<std::string,const templatious::TypeNode*> TNodeMap;
typedef std::lock_guard<std::mutex> Guard;

struct TheMap {
    TNodeMap _m;
    std::mutex _mtx;
};

static TheMap& getMap() {
    static TheMap map;
    return map;
}

}

namespace SafeLists {

static int registerTypeInMap(const char* name,const templatious::TypeNode* node) {
    auto& inst = getMap();
    auto& map = inst._m;

    Guard g(inst._mtx);

    auto found = map.find(name);
    if (map.end() != found) {
        assert(false && "Key already exists");
    }

    map.insert({name,node});

    return 7;
}

static void traverseTypes(const std::function<void(const char*,const templatious::TypeNode*)>& func) {
    auto& inst = getMap();
    auto& map = inst._m;

    std::string buf;

    Guard g(inst._mtx);
    for (auto& i : map) {
        buf = i.first;
        func(buf.c_str(),i.second);
    }
}

}
