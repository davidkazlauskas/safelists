
extern crate libc;
extern crate safefiledownloader;
extern crate env_logger;

pub extern fn step_function(
    param: *mut libc::c_void,
    start: libc::uint64_t,
    end: libc::uint64_t,
    buffer: *const libc::uint8_t) -> libc::int32_t {
    println!("step");
    return 0i32;
}

pub extern fn next_range_func(
    param: *mut libc::c_void,
    start: *mut libc::int64_t,
    end: *mut libc::int64_t
) -> libc::int32_t {
    println!("nrange");
    return 0i32;
}

pub extern fn arbitrary_message_func(
    param: *mut libc::c_void,
    msgtype: i32,
    msgbod: *mut libc::c_void
) {
    println!("arb");
}

pub extern fn dtor(
    param: *mut libc::c_void
) {
    unsafe {
        let curr: std::sync::Arc<std::sync::mpsc::Sender<i32>> = std::mem::transmute(param);
        curr.send(0);
        drop(curr);
    }
    println!("dtor");
}

fn main() {
    env_logger::init().unwrap();

    use safefiledownloader::{DownloadTask, DownloaderActor, DownloaderActorLocal, DownloaderMsgs};
    let (remote,local) = DownloaderActor::new();
    DownloaderActorLocal::launch_thread(local);

    let (tx,rx) = std::sync::mpsc::channel::<i32>();
    let wtx = std::sync::Arc::new(tx);

    unsafe {
        let task = DownloadTask {
            userdata: std::mem::transmute(wtx),
            userdata_buffer_func: step_function,
            userdata_next_range_func: next_range_func,
            userdata_arbitrary_message_func: arbitrary_message_func,
            userdata_destructor: dtor
        };

        let msg = DownloaderMsgs::Schedule {
            path: "www.slstocks/bg.jpg".to_string(), task: task
        };

        remote.send.send(msg);
    }

    rx.recv();

    println!("Hello world");
}
