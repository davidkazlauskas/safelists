#ifndef SCOPEGUARD_AZYV9OGO
#define SCOPEGUARD_AZYV9OGO

#include <utility>

namespace SafeLists {

template <class T>
struct ScopeGuard {
    template <class V>
    ScopeGuard(V&& function) :
        _use(true), _f(std::forward<V>(function))
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

#endif /* end of include guard: SCOPEGUARD_AZYV9OGO */
