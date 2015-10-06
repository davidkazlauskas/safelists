
#include <templatious/FullPack.hpp>
#include "GenericStMessageable.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    void GenericStMessageable::message(templatious::VirtualPack& msg) {
        TEMPLATIOUS_FOREACH(auto& i,_handlers) {
            bool didMatch = i->tryMatch(msg);
            if (didMatch) {
                break;
            }
        }
    }

}

