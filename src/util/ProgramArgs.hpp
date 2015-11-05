// access program args from anywhere
#ifndef PROGRAMARGS_59ON0NIB
#define PROGRAMARGS_59ON0NIB

#include <string>
#include <vector>

namespace SafeLists {

    void setGlobalProgramArgs(int argc,char** argv);
    std::vector< std::string > getGlobalProgramArgs();

}

#endif /* end of include guard: PROGRAMARGS_59ON0NIB */
