#ifndef SAFE_FFI_BDET7AES
#define SAFE_FFI_BDET7AES

#include <cstdint>

extern "C" {

    int32_t create_unregistered_client(void** out);
    void drop_client(void* client);

}

#endif /* end of include guard: SAFE_FFI_BDET7AES */
