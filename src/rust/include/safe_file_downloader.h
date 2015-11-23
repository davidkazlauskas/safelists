#ifndef SAFE_FILE_DOWNLOADER_JD3CE0PO
#define SAFE_FILE_DOWNLOADER_JD3CE0PO

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    typedef int32_t (*sfd_userdata_buffer_func)(
        void* userdata,int64_t start,int64_t end,const uint8_t* buf);

    typedef void (*sfd_userdata_arbitrary_message_func)(
        void* userdata,int32_t msgtype,const void* buf);

    typedef void (*sfd_userdata_function)(
        void* userdata);

    struct safe_file_downloader_args {
        void* userdata;
        sfd_userdata_buffer_func userdata_buffer_func;
        sfd_userdata_arbitrary_message_func userdata_arbitrary_message_func;
        sfd_userdata_function userdata_destructor;
        const char* path;
    };

    void* safe_file_downloader_new();
    void safe_file_downloader_cleanup(void* handle);
    void safe_file_downloader_schedule(void* handle,void* args /* safe_file_downloader_args */ );

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: SAFE_FILE_DOWNLOADER_JD3CE0PO */
