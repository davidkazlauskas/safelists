#ifndef LICENSING_X1B9GTMJ
#define LICENSING_X1B9GTMJ

#include <LuaPlumbing/messageable.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct LicenseDaemon {

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

    // Get local tiemspan.
    // Signature:
    // <
    //   GetLocalTimespan,
    //   std::string (pubkey base64),
    //   std::string (out timespan),
    //   int (errocode, 0 success)
    // >
    DUMMY_REG(GetLocalTimespan,"LD_GetLocalTimespan");

    // Get server tiemspan.
    // Signature:
    // <
    //   GetServerTimespan,
    //   std::string (pubkey base64),
    //   std::string (out timespan),
    //   int (errocode, 0 success)
    // >
    DUMMY_REG(GetServerTimespan,"LD_GetServerTimespan");

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

    // Query safecoin rate from server
    // Signature:
    // <
    //   GetServerSafecoinRate,
    //   std::string (content),
    //   int (error code, 0 success)
    // >
    DUMMY_REG(GetServerSafecoinRate,"LD_GetServerSafecoinRate");

    // Get safecoin rate if stored locally
    // Signature:
    // <
    //   GetLocalSafecoinRate,
    //   std::string (content),
    //   int (error code, 0 success)
    // >
    DUMMY_REG(GetLocalSafecoinRate,"LD_GetLocalSafecoinRate");

    // Store safecoin rate locally
    // Signature:
    // <
    //   StoreLocalSafecoinRate,
    //   std::string (content),
    //   int (error code, 0 success)
    // >
    DUMMY_REG(StoreLocalSafecoinRate,"LD_StoreLocalSafecoinRate");

    // Delete local license
    // Signature:
    // <
    //   DeleteLocalLicense,
    //   std::string (pubkey base64),
    //   int (error code, 0 success)
    // >
    DUMMY_REG(DeleteLocalLicense,"LD_DeleteLocalLicense");

    // Delete local license
    // Signature:
    // <
    //   DeleteLocalSpan,
    //   std::string (pubkey base64),
    //   int (error code, 0 success)
    // >
    DUMMY_REG(DeleteLocalSpan,"LD_DeleteLocalSpan");

    // Check if user record is valid. Json, retrieved from
    // server expected.
    // Signature:
    // <
    //   UserRecordValidity,
    //   std::string (json),
    //   int (out result, 0 success)
    // >
    DUMMY_REG(UserRecordValidity,"LD_UserRecordValidity");

    // Check user timespan is valid.
    // Signature:
    // <
    //   TimeSpanValidity,
    //   std::string (json),
    //   int (out result, 0 success)
    // >
    DUMMY_REG(TimeSpanValidity,"LD_TimeSpanValidity");

    // Check if safecoin rate is valid.
    // Signature:
    // <
    //   SafecoinRateValidity,
    //   std::string (json),
    //   int (out result, 0 success)
    // >
    DUMMY_REG(SafecoinRateValidity,"LD_SafecoinRateValidity");

    // Register user.
    // Signature:
    // <
    //   RegisterUser,
    //   std::string (username),
    //   StrongMsgPtr (tonotify)
    // >
    DUMMY_REG(RegisterUser,"LD_RegisterUser");

    // Register user, solving challenge.
    // Signature:
    // <
    //   RU_OutSolving
    // >
    DUMMY_REG(RU_OutSolving,"LD_RU_OutSolving");

    // Register user finished.
    // Signature:
    // <
    //   RU_OutFinished,
    //   int (out result)
    // >
    DUMMY_REG(RU_OutFinished,"LD_RU_OutFinished");

    // Register user finished.
    // Signature:
    // <
    //   RU_OutServerResponse,
    //   std::string (response)
    // >
    DUMMY_REG(RU_OutServerResponse,"LD_RU_OutServerResponse");

    // singleton
    static StrongMsgPtr getDaemon();
};

int firstTierSignatureVerification(const std::string& theJson);

}

#endif /* end of include guard: LICENSING_X1B9GTMJ */
