#ifndef AUTOREG_PYXSMU4M
#define AUTOREG_PYXSMU4M

#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicPackCreator.hpp>

#include <messageable.hpp>

// object in C++ has to be at least byte in size,
// we might as well use that byte to store the proof
// of calculation for static initialization.
#define DUMMY_REG(name,strname)  \
    struct name {                                                                    \
        template <class Any> name(Any&&,int reg = Regger::s_sideEffect) {            \
            assignStatic();                                                          \
        }                                                                            \
        typedef ::SafeLists::TypeRegger<name> Regger;                                \
        name() { assignStatic(); }                                                   \
        static int registerType() {                                                  \
            static int res = ::SafeLists::registerTypeInMap(strname,                 \
                templatious::TypeNodeFactory::makeDummyNode< name >(strname));       \
            return res;                                                              \
        }                                                                            \
        char dummyData() { return _var; }                                            \
    private:                                                                         \
        void assignStatic() {                                                        \
            static const char* effect = &Regger::s_sideEffect;                       \
            _var = *effect;                                                          \
        }                                                                            \
        char _var;                                                                   \
    };

// register for native only use
#define DUMMY_STRUCT_NATIVE(name)  \
    struct name {                                                                    \
        template <class Any> name(Any&&) {}                                          \
    };

namespace SafeLists {

template <class T>
struct TypeRegger {
    static const char s_sideEffect;
};

template <class T>
const char TypeRegger<T>::s_sideEffect = T::registerType();

char registerTypeInMap(const char* name,const templatious::TypeNode* node);
void traverseTypes(const std::function<void(const char*,const templatious::TypeNode*)>& func);

struct MessageableMatchFunctor : public Messageable {
    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;

    MessageableMatchFunctor() = delete;
    MessageableMatchFunctor(MessageableMatchFunctor&&) = delete;
    MessageableMatchFunctor(const MessageableMatchFunctor&) = delete;

    MessageableMatchFunctor(VmfPtr&& ptr) :
        _handler(std::move(ptr)) {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override;
    void message(templatious::VirtualPack& msg) override;

private:
    VmfPtr _handler;
};

}

#endif /* end of include guard: AUTOREG_PYXSMU4M */
