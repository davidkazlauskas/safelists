#ifndef LICENSING_X1B9GTMJ
#define LICENSING_X1B9GTMJ

#include <LuaPlumbing/messageable.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct LicenseDaemon {

    // asynchronous check if license is expied.
    // Signature:
    // < IsExpired, bool (out result) >
    DUMMY_REG(IsExpired,"LD_IsExpired");

    // get current user id
    // Signature:
    // < GetCurrentUserId, std::string (out) >
    DUMMY_REG(GetCurrentUserId,"LD_GetCurrentUserId");

    // Get local record.
    // Signature:
    // <
    //   GetLocalRecord,
    //   std::string (pubkey base64),
    //   std::string (out license),
    //   int (errocode, 0 success)
    // >
    DUMMY_REG(GetLocalRecord,"LD_GetLocalRecord");

    // Get user record from server.
    // Signature:
    // <
    //   GetServerRecord,
    //   std::string (pubkey base64),
    //   std::string (out license),
    //   int (error code, 0 success)
    // >
    DUMMY_REG(GetServerRecord,"LD_GetServerRecord");

    // Store local license into file for
    // faster retrieval in the future.
    // Signature:
    // <
    //   StoreLocalLicense,
    //   std::string (pubkey base64),
    //   std::string (license content),
    //   int (error code, 0 success)
    // >
    DUMMY_REG(StoreLocalLicense,"LD_StoreLocalIcense");

    // singleton
    static StrongMsgPtr getDaemon();
};

int firstTierSignatureVerification(const std::string& theJson);

}

#endif /* end of include guard: LICENSING_X1B9GTMJ */
