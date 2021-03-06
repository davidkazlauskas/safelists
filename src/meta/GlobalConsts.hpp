#ifndef GLOBALCONSTS_1I5CHXX9
#define GLOBALCONSTS_1I5CHXX9

#include <LuaPlumbing/messageable.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

    struct GlobalConsts {

        // Lookup string in global settings.
        // Signature:
        // <
        //   LookupString,
        //   const std::string (key),
        //   std::string (out value),
        //   int (errcode)
        // >
        //
        // Settings:
        // "userdatapath" -> path to userdata folder
        // "settingspath" -> path to settings.json file
        DUMMY_REG(LookupString,"GLC_LookupString");

        // singleton
        static StrongMsgPtr getConsts();
    };

}

#endif /* end of include guard: GLOBALCONSTS_1I5CHXX9 */
