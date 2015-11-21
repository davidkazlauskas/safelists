extern crate libc;

use std::sync::Arc;

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

    fn launch_thread(pointer: Arc<DownloaderActor>) {
        println!("Thread lunched!");
    }
}

extern "C" fn safe_file_downloader_new() -> *mut libc::c_void {
    let the_arc = Arc::new( DownloaderActor::new() );
    DownloaderActor::launch_thread(the_arc.clone());
}

#[test]
fn it_works() {
}
