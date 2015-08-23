#ifndef AUTOREG_PYXSMU4M
#define AUTOREG_PYXSMU4M

#include <templatious/detail/DynamicPackCreator.hpp>

#define DUMMY_REG(name,strname)  \
    struct name { template <class Any> name(Any&&,int reg = Regger::s_sideEffect) {} \
        typedef ::SafeLists::TypeRegger<name> Regger;                                \
        name() {}                                                                    \
        static int registerType() {                                                  \
            static int res = ::SafeLists::registerTypeInMap(strname,                 \
                templatious::TypeNodeFactory::makeDummyNode< name >(strname));       \
            return res;                                                              \
        }                                                                            \
    };

namespace SafeLists {

template <class T>
struct TypeRegger {
    static const int s_sideEffect;
};

template <class T>
const int TypeRegger<T>::s_sideEffect = T::registerType();

int registerTypeInMap(const char* name,const templatious::TypeNode* node);
void traverseTypes(const std::function<void(const char*,const templatious::TypeNode*)>& func);

}

#endif /* end of include guard: AUTOREG_PYXSMU4M */
