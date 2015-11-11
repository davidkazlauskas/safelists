
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

