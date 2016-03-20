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

char registerTypeInMap(const char* name,const templatious::TypeNode* node) {
    auto& inst = getMap();
    auto& map = inst._m;

    Guard g(inst._mtx);

    auto found = map.find(name);
    if (map.end() != found) {
        printf("Warning, name |%s| is registered two times.\n");
        printf("Previous: |0x%p| Current: |0x%p|\n",(const void*) found->second,(const void*) node);
        //assert(false && "Key already exists");
    }

    map.insert({name,node});

    return '7';
}

void traverseTypes(const std::function<void(const char*,const templatious::TypeNode*)>& func) {
    auto& inst = getMap();
    auto& map = inst._m;

    std::string buf;

    Guard g(inst._mtx);
    for (auto& i : map) {
        buf = i.first;
        func(buf.c_str(),i.second);
    }
}

void MessageableMatchFunctor::message(const std::shared_ptr< templatious::VirtualPack >& msg) {
    assert( false && "Async messages disabled on messageable match functor." );
}

void MessageableMatchFunctor::message(templatious::VirtualPack& msg) {
    _handler->tryMatch(msg);
}

void MessageableMatchFunctorWAsync::message(const std::shared_ptr< templatious::VirtualPack >& msg) {
    _handler->tryMatch(*msg);
}

void MessageableMatchFunctorWAsync::message(templatious::VirtualPack& msg) {
    _handler->tryMatch(msg);
}

}
