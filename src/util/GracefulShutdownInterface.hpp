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

        // Emitted to locak handle
        // as single threaded message
        // when shutdown is started
        // for all messageables to stop
        // and try to terminate.
        DUMMY_STRUCT(ShutdownSignal);

        // Check if object to shutdown is
        // dead.
        // Signature: < IsDead, bool >
        DUMMY_STRUCT(IsDead);
    };
}

#endif /* end of include guard: GRACEFULSHUTDOWNINTERFACE_XMC7PIDI */
