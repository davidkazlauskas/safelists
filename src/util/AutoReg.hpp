#ifndef AUTOREG_PYXSMU4M
#define AUTOREG_PYXSMU4M

#include <templatious/detail/DynamicPackCreator.hpp>

namespace SafeLists {

static int registerTypeInMap(const char* name,const templatious::TypeNode* node);
static void traverseTypes(std::function<void(const char*,const templatious::TypeNode*)>& func);

}

#endif /* end of include guard: AUTOREG_PYXSMU4M */
