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

    // Look for license locally
    // Signature:
    // < LicenseForPublicKeyLocal, std::string (outpath), int (errcode) >
    DUMMY_REG(LicenseForPublicKeyLocal,"LD_LicenseForPublicKeyLocal");

    // singleton
    static StrongMsgPtr getDaemon();
};

int firstTierSignatureVerification(const std::string& theJson);

}

#endif /* end of include guard: LICENSING_X1B9GTMJ */
