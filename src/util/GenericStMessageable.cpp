
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
    int GenericStMessageable::regHandler(VmfPtr&& toreg) {
        _handlers.insert(_handlers.begin(),std::move(toreg));
        return SA::size(_handlers);
    }

    // may reg handler some time
    GenericStMessageable::GenericStMessageable() {}

    void GenericStMessageable::passMessageUp(int myInheritanceLevel,templatious::VirtualPack& msg) {
        int currSize = SA::size(_handlers);
        int theIndex = currSize - myInheritanceLevel + 1;
        if (theIndex < currSize) {
            _handlers[theIndex]->tryMatch(msg);
        }
    }

    GenericStMessageableWCallbacks::GenericStMessageableWCallbacks() :
        GenericStMessageable()
    {
        typedef GenericMessageableInterface GMI;
        regHandler(
            SF::virtualMatchFunctorPtr(
                SF::virtualMatch< GMI::InAttachToEventLoop, std::function<bool()> >(
                    [=](GMI::InAttachToEventLoop,const std::function<bool()>& func) {
                        this->_cache.attach(func);
                    }
                )
            )
        );
    }

    void GenericStMessageableWCallbacks::fireCallbacks() {
        _cache.process();
    }
}

