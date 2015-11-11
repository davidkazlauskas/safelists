#ifndef GENERICSTMESSAGEABLE_C1773IK5
#define GENERICSTMESSAGEABLE_C1773IK5

#include <cassert>
#include <LuaPlumbing/messageable.hpp>
#include <LuaPlumbing/plumbing.hpp>

namespace SafeLists {

struct GenericStMessageable : public Messageable {
    // this is for sending message across threads (disabled)
    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false && "GenericStMessageable only deals with single threaded messages." );
    }

    // this is for sending stack allocated (faster)
    // if we know we're on the same thread as GUI
    void message(templatious::VirtualPack& msg) override;

protected:
    GenericStMessageable();
    GenericStMessageable(const GenericStMessageable&) = delete;
    GenericStMessageable(GenericStMessageable&&) = delete;

    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    // SHOULD ONLY BE CALLED IN CONSTRUCTOR
    int regHandler(VmfPtr&& toreg);

    // to possibly remove virtual call overhead in some cases...
    // (if it even exists)
    void messageNonVirtual(templatious::VirtualPack& msg);

    // Basically, allows calling base class methods.
    // Process message starting at so and so level.
    void passMessageUp(int myInheritanceLevel,templatious::VirtualPack& msg);
private:
    std::vector< VmfPtr > _handlers;
};

struct GenericStMessageableWCallbacks : public GenericStMessageable {
protected:
    GenericStMessageableWCallbacks();
    GenericStMessageableWCallbacks(const GenericStMessageableWCallbacks&) = delete;
    GenericStMessageableWCallbacks(GenericStMessageableWCallbacks&&) = delete;

    // drive attached events with this function
    void fireCallbacks();
private:
    CallbackCache _cache;
};

}

#endif /* end of include guard: GENERICSTMESSAGEABLE_C1773IK5 */

