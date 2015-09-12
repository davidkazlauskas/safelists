#ifndef SCOPEGUARD_AZYV9OGO
#define SCOPEGUARD_AZYV9OGO

#include <utility>

namespace SafeLists {

template <class T>
struct ScopeGuard {
    ScopeGuard(T&& function) :
        _use(true), _f(std::move(function))
    {}

    void dismiss() {
        _use = false;
    }

    ~ScopeGuard() {
        if (_use) {
            _f();
        }
    }

private:
    bool _use;
    T _f;
};

template <class T>
auto makeScopeGuard(T&& t)
    -> ScopeGuard< typename std::decay<T>::type >
{
    typedef typename std::decay<T>::type DecT;
    return ScopeGuard<DecT>(std::forward<T>(t));
}

}

#define SCOPE_GUARD(stuff) ::SafeLists::makeScopeGuard(stuff)

#endif /* end of include guard: SCOPEGUARD_AZYV9OGO */
