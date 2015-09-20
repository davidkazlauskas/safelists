#ifndef ASYNCSQLITEFACTORY_MG84D90V
#define ASYNCSQLITEFACTORY_MG84D90V

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct AsyncSqliteFactory {

    // Creates new async sqlite according
    // to the path.
    // Signature:
    // < CreateNew, std::string (path) >
    DUMMY_REG(CreateNew,"ASQLF_CreateNew");

    static StrongMsgPtr createNew();
};

}

#endif /* end of include guard: ASYNCSQLITEFACTORY_MG84D90V */
