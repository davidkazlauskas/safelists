#ifndef DUMMYSTRUCT_PPVAK2XY
#define DUMMYSTRUCT_PPVAK2XY

#define DUMMY_STRUCT(name)  \
    struct name { template <class Any> name(Any&&) {} name() {} };

#endif /* end of include guard: DUMMYSTRUCT_PPVAK2XY */
