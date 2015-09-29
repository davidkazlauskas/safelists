#ifndef GRACEFULSHUTDOWNINTERFACE_XMC7PIDI
#define GRACEFULSHUTDOWNINTERFACE_XMC7PIDI

#include <util/AutoReg.hpp>
#include <util/DummyStruct.hpp>

namespace SafeLists {
    struct GracefulShutdownInterface {

        // add new instance to shutdown guard,
        // single threaded.
        DUMMY_REG(AddNew,"GSI_AddNew");

        // Send this to messageable object
        // with the handle to the messageable.
        // Signature: < InRegisterItself, StrongMsgPtr >
        DUMMY_STRUCT(InRegisterItself);

        // Send this from messageable object
        // with new messageable to be put into queue
        // Signature: < OutRegisterItself, StrongMsgPtr >
        DUMMY_STRUCT(OutRegisterItself);


        // BELOW ARE SINGLE THREADED MESSAGES

        // Emitted to locak handle
        // as single threaded message
        // when shutdown is started
        // for all messageables to stop
        // and try to terminate.
        DUMMY_STRUCT(ShutdownSignal);

        // Emitted to wait out shutdowns.
        // ShutdownSignal must have been
        // sent first.
        DUMMY_STRUCT(WaitOut);

        // Check if object to shutdown is
        // dead.
        // Signature: < IsDead, bool >
        DUMMY_STRUCT(IsDead);
    };
}

#endif /* end of include guard: GRACEFULSHUTDOWNINTERFACE_XMC7PIDI */
