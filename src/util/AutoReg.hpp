#ifndef AUTOREG_PYXSMU4M
#define AUTOREG_PYXSMU4M

#include <templatious/detail/DynamicPackCreator.hpp>

#define DUMMY_REG(name,strname)                 \
    DUMMY_STRUCT(name);                         \
    static const int intvar_##name##__LINE__ =  \
        ::SafeLists::registerTypeInMap(strname, \
            templatious::TypeNodeFactory::makeDummyNode< name >(strname))

namespace SafeLists {

static int registerTypeInMap(const char* name,const templatious::TypeNode* node);
static void traverseTypes(const std::function<void(const char*,const templatious::TypeNode*)>& func);

}

#endif /* end of include guard: AUTOREG_PYXSMU4M */
