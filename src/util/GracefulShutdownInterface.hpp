#ifndef GRACEFULSHUTDOWNINTERFACE_XMC7PIDI
#define GRACEFULSHUTDOWNINTERFACE_XMC7PIDI

#include <util/DummyStruct.hpp>

namespace SafeLists {
    struct GracefulShutdownInterface {
        // Send this to messageable object
        // with the handle to the messageable.
        // Signature: < InRegisterItself, StrongMsgPtr >
        DUMMY_STRUCT(InRegisterItself);

        // Send this from messageable object
        // with new messageable to be put into queue
        // Signature: < OutRegisterItself, StrongMsgPtr >
        DUMMY_STRUCT(OutRegisterItself);
    };
}

#endif /* end of include guard: GRACEFULSHUTDOWNINTERFACE_XMC7PIDI */
