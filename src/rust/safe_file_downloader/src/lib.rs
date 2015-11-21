extern crate libc;
extern crate safe_nfs;

use std::sync::Arc;

const CHUNK_SIZE : i64 = 64 * 1024;

// chunk size for each download
fn get_chunk_size() -> i64 {
    CHUNK_SIZE
}

struct DownloadTaskState<'a> {
    size: i64,
    progress: i64,
    reader: ::safe_nfs::helper::reader::Reader<'a>,
}

struct DownloadTask {
    userdata: *mut libc::c_void,
    userdata_buffer_func: extern fn(
        param: *mut libc::c_void,
        libc::int64_t,
        libc::int64_t,
        *mut libc::c_char),
    userdata_destructor: extern fn(param: *mut libc::c_void),
}

impl Drop for DownloadTask {
    fn drop(&mut self) {
        (self.userdata_destructor)(self.userdata);
        println!("Task destroyed!");
    }
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

impl Drop for DownloaderActor {
    fn drop(&mut self) {
        println!("Downloader actor destroyed!");
    }
}

#[no_mangle]
pub extern fn safe_file_downloader_new() -> *mut libc::c_void {
    let the_arc = Box::new( Arc::new( DownloaderActor::new() ) );
    DownloaderActor::launch_thread((*the_arc).clone());

    let raw = Box::into_raw(the_arc);
    unsafe {
        std::mem::transmute(raw)
    }
}

#[no_mangle]
pub extern fn safe_file_downloader_cleanup(ptr: *mut libc::c_void) {
    unsafe {
        let transmuted : *mut Arc<DownloaderActor> = std::mem::transmute(ptr);
        let boxed : Box< std::sync::Arc<DownloaderActor> > = Box::from_raw(transmuted);
    }
}

#[test]
fn it_works() {
}
