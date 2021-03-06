#ifndef SAFE_FILE_DOWNLOADER_JD3CE0PO
#define SAFE_FILE_DOWNLOADER_JD3CE0PO

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    typedef int32_t (*sfd_userdata_buffer_func)(
        void* userdata,int64_t start,int64_t end,const uint8_t* buf);

    typedef int32_t (*sfd_request_next_range_func)(
        void* userdata,int64_t* start,int64_t* end);

    typedef void (*sfd_userdata_arbitrary_message_func)(
        void* userdata,int32_t msgtype,void* buf);

    typedef void (*sfd_userdata_function)(
        void* userdata);

    struct safe_file_downloader_args {
        void* userdata;
        sfd_userdata_buffer_func userdata_buffer_func;
        sfd_request_next_range_func userdata_next_range_func;
        sfd_userdata_arbitrary_message_func userdata_arbitrary_message_func;
        sfd_userdata_function userdata_destructor;
        const char* path;
    };

// messaged to function requested file is not found.
// additional arg is null.
#define SAFE_DOWNLOADER_MSG_FILE_NOT_FOUND 7

// messaged after successful download.
// additional arg is null.
#define SAFE_DOWNLOADER_MSG_DOWNLOAD_SUCCESS 8

// messaged to find out if download is
// done and should be stopped (arg is i32, 0 is false anything else true)
#define SAFE_DOWNLOADER_MSG_IS_DONE 9

// messaged when file size is known (arg is filesize as i64)
#define SAFE_DOWNLOADER_MSG_FILE_SIZE_FOUND 10

    // init safe file downloader and return handle.
    void* safe_file_downloader_new();

    // cleanup handle, non blocking. if on_done function is not null it is
    // called with on_done_data just before actor terminates.
    void safe_file_downloader_cleanup(void* handle,sfd_userdata_function on_done,void* on_done_data);

    // args should be pointer safe_file_downloader_args structure.
    // Ownership now belongs for safe_file_downloader actor.
    // safe_file_downloader ensures that destructor on safe_file_downloader_args.userdata
    // is called.
    void safe_file_downloader_schedule(void* handle,void* args /* safe_file_downloader_args */ );

    // init logging for rust
    void safe_file_downloader_init_logging();
#ifdef __cplusplus
}
#endif

#endif /* end of include guard: SAFE_FILE_DOWNLOADER_JD3CE0PO */
