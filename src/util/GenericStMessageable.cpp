
#include <templatious/FullPack.hpp>
#include "GenericStMessageable.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    void GenericStMessageable::message(templatious::VirtualPack& msg) {
        messageNonVirtual(msg);
    }

    void GenericStMessageable::messageNonVirtual(templatious::VirtualPack& msg) {
        TEMPLATIOUS_FOREACH(auto& i,_handlers) {
            bool didMatch = i->tryMatch(msg);
            if (didMatch) {
                break;
            }
        }
    }

    // SHOULD ONLY BE CALLED IN CONSTRUCTOR
    void GenericStMessageable::regHandler(VmfPtr&& toreg) {
        _handlers.insert(_handlers.begin(),std::move(toreg));
    }

    // may reg handler some time
    GenericStMessageable::GenericStMessageable() {}
}

