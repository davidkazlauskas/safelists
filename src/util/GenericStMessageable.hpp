#ifndef GENERICSTMESSAGEABLE_C1773IK5
#define GENERICSTMESSAGEABLE_C1773IK5

#include <cassert>
#include <LuaPlumbing/messageable.hpp>

namespace SafeLists {

struct GenericStMessageable : public Messageable {
    // this is for sending message across threads
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
    void regHandler(VmfPtr&& toreg);

private:
    std::vector< VmfPtr > _handlers;
};

}

#endif /* end of include guard: GENERICSTMESSAGEABLE_C1773IK5 */

