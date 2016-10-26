
extern crate libc;
extern crate safefiledownloader;

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
    println!("dtor");
}

fn main() {
    use safefiledownloader;
    let (remote,local) = safefiledownloader::DownloaderActor::new();
    safefiledownloader::DownloaderActorLocal::launch_thread(local);

    let task = safefiledownloader::DownloadTask {
        userdata: std::ptr::null_mut(),
        userdata_buffer_func: step_function,
        userdata_next_range_func: next_range_func,
        userdata_arbitrary_message_func: arbitrary_message_func,
        userdata_destructor: dtor
    };

    let msg = DownloaderMsgs::Schedule {
        path: "www.slstocks/bg.jpg", task: task
    };

    remote.send(msg);

    println!("Hello world");
}
