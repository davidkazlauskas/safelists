extern crate libc;

struct DownloadTask {
    userdata: *mut libc::c_void,
    userdata_buffer_func: extern fn(
        param: *mut libc::c_void,
        libc::int64_t,
        libc::int64_t,
        *mut libc::c_char),
    userdata_destructor: extern fn(param: *mut libc::c_void),
}

struct DownloaderActor {
    tasks: Vec< DownloadTask >,
}

#[test]
fn it_works() {
}
