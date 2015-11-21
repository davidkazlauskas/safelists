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

impl DownloaderActor {
    fn new() -> DownloaderActor {
        DownloaderActor {
            tasks: vec![],
        }
    }
}

extern "C" fn safe_file_downloader_new() -> *mut libc::c_void {
    let the_arc = std::sync::Arc::new( DownloaderActor::new() );
}

#[test]
fn it_works() {
}
