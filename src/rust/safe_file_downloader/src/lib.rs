#![feature(plugin)]

extern crate libc;
extern crate safe_core;
extern crate regex;
extern crate rustc_serialize;
extern crate core;
#[macro_use] extern crate quick_error;

use std::sync::{Arc,Mutex};
use regex::Regex;

use safe_core::core::client::Client;
use safe_core::nfs::helper::directory_helper::DirectoryHelper;
use safe_core::nfs::helper::file_helper::FileHelper;
use safe_core::nfs::directory_listing::DirectoryListing;
use safe_core::nfs::errors::NfsError;
use safe_core::dns::dns_operations::DnsOperations;
use safe_core::dns::errors::DnsError;

const SAFE_DOWNLOADER_MSG_FILE_NOT_FOUND : i32 = 7;
const SAFE_DOWNLOADER_MSG_DOWNLOAD_SUCCESS : i32 = 8;
const SAFE_DOWNLOADER_MSG_IS_DONE : i32 = 9;
const SAFE_DOWNLOADER_MSG_FILE_SIZE_FOUND : i32 = 10;

struct ReaderKit {
    client: Arc< Mutex< Client > >,
    dns_ops: DnsOperations,
    dir_helper: DirectoryHelper,
    file_helper: FileHelper,
}

quick_error! {
    #[derive(Debug)]
    pub enum GetReaderKitError {
        CouldNotCreateClient(err: &'static str) {
            description(err)
            display("{}",err)
        }
    }
}

impl ReaderKit {
    // should be created only once
    // per program
    fn unregistered_kit() -> Result< ReaderKit, GetReaderKitError >  {
        let client = Client::create_unregistered_client();

        if client.is_err() {
            return Err( GetReaderKitError::CouldNotCreateClient(
                "Could not create unregistered client.") );
        }

        let clientu = Arc::new( Mutex::new( client.unwrap() ) );

        let dns_ops = DnsOperations::new_unregistered(clientu.clone());
        let dir_helper = DirectoryHelper::new(clientu.clone());
        let file_helper = FileHelper::new(clientu.clone());

        Ok(
            ReaderKit {
                client: clientu,
                dns_ops: dns_ops,
                dir_helper: dir_helper,
                file_helper: file_helper,
            }
        )
    }
}

struct DownloadTask {
    userdata: *mut libc::c_void,
    userdata_buffer_func: extern fn(
        param: *mut libc::c_void,
        libc::uint64_t,
        libc::uint64_t,
        *const libc::uint8_t) -> libc::int32_t,
    userdata_next_range_func: extern fn(
        param: *mut libc::c_void,
        *mut libc::int64_t,
        *mut libc::int64_t) -> libc::int32_t,
    userdata_arbitrary_message_func: extern fn(
        param: *mut libc::c_void,
        msgtype: i32,
        param: *mut libc::c_void),
    userdata_destructor: extern fn(param: *mut libc::c_void),
}

impl Drop for DownloadTask {
    fn drop(&mut self) {
        (self.userdata_destructor)(self.userdata);
        println!("Task destroyed!");
    }
}

unsafe impl Send for DownloadTask {}
unsafe impl Send for DownloaderMsgs {}

enum DownloaderMsgs {
    Schedule { path: String,task: DownloadTask },
    Stop {
        donefunc: extern fn(param: *mut libc::c_void),
        donedata: *mut libc::c_void
    },
}

struct DownloadTaskWRreader {
    task: DownloadTask,
    file: safe_core::nfs::file::File,
}

impl DownloadTaskWRreader {
    fn next_chunk(&self) -> (u64,u64) // start, length
    {
        let (mut chunkstart,mut chunkend) = (0i64,0i64);

        unsafe {
            (self.task.userdata_next_range_func)(
                self.task.userdata,
                &mut chunkstart as *mut i64,
                &mut chunkend as *mut i64);
        }


        (chunkstart as u64,chunkend as u64)
    }

    fn is_done(&self) -> bool {
        let mut out: i32 = 0i32;
        unsafe {
            (self.task.userdata_arbitrary_message_func)(
                self.task.userdata,
                SAFE_DOWNLOADER_MSG_IS_DONE,
                std::mem::transmute(&mut out as *mut i32));
        }

        out > 0
    }
}

struct DownloaderActor {
    send: ::std::sync::mpsc::Sender< DownloaderMsgs >,
}

struct DownloaderActorLocal {
    recv: ::std::sync::mpsc::Receiver< DownloaderMsgs >,
    rkit: ReaderKit,
    tasks: Vec< DownloadTaskWRreader >,
    iter: usize,
    endfunc: Option< extern fn(udata: *mut libc::c_void) >,
    enddata: *mut libc::c_void,
}

impl DownloaderActor {
    fn new() -> (
        DownloaderActor,
        ::std::sync::mpsc::Receiver< DownloaderMsgs >
    )
    {
        let (tx,rx) = ::std::sync::mpsc::channel();

        (
            DownloaderActor {
                send: tx,
            },
            rx
        )
    }
}

quick_error! {
    #[derive(Debug)]
    pub enum AddTaskError {
        FileNotFound(err: &'static str) {
            description(err)
            display("{}",err)
        }
    }
}

impl DownloaderActorLocal {
    fn new(kit: ReaderKit,
           receiver: std::sync::mpsc::Receiver< DownloaderMsgs >)
        -> DownloaderActorLocal
    {
        DownloaderActorLocal {
            recv: receiver,
            rkit: kit,
            tasks: vec![],
            iter: 0,
            endfunc: None,
            enddata: std::ptr::null_mut(),
        }
    }

    fn handle(&mut self,msg: DownloaderMsgs) -> bool {
        match msg {
            DownloaderMsgs::Schedule { path, task } => {
                self.add_task(path,task);
            },
            DownloaderMsgs::Stop { donedata, donefunc } => {
                self.endfunc = Some(donefunc);
                self.enddata = donedata;
                return false;
            },
        }

        return true;
    }

    fn add_task(&mut self,path: String,task: DownloadTask)
    -> Result< bool, AddTaskError > {
        let file = get_file(&self.rkit,&path);
        if file.is_err() {
            (task.userdata_arbitrary_message_func)(
                task.userdata,
                SAFE_DOWNLOADER_MSG_FILE_NOT_FOUND,
                std::ptr::null_mut()
            );
            return Err( AddTaskError
                ::FileNotFound("Could not find file.") );
        }

        let fileu = file.unwrap();

        {
            let reader = self.rkit.file_helper.read(&fileu);
            let size = reader.size() as i64;
            let mut sizemut : i64 = size;
            unsafe {
                (task.userdata_arbitrary_message_func)(
                    task.userdata,
                    SAFE_DOWNLOADER_MSG_FILE_SIZE_FOUND,
                    std::mem::transmute(&mut sizemut as *mut i64));
            }
        }

        let mut res = DownloadTaskWRreader {
            task: task,
            file: fileu,
        };

        self.tasks.push(res);
        Ok(true)
    }

    fn current_task(&self) -> Option< &DownloadTaskWRreader > {
        let len = self.tasks.len();
        if 0 == len {
            return None;
        }

        Some(&self.tasks[self.iter])
    }

    fn current_task_mut(&mut self) -> Option< &mut DownloadTaskWRreader > {
        let len = self.tasks.len();
        if 0 == len {
            return None;
        }

        Some(&mut self.tasks[self.iter])
    }

    fn next_task(&mut self) {
        self.iter += 1;

        if self.iter >= self.tasks.len() {
            self.iter = 0;
        }
    }

    fn ensure_valid(&mut self) {
        if self.iter >= self.tasks.len() {
            self.next_task();
        }
    }

    fn main_tasks(&mut self) -> bool {
        let res = self.proc_messages_non_blocking();
        if !res {
            return false;
        }
        self.download_next();
        return res;
    }

    fn filter_done(&mut self) {
        let mut new_vec = Vec::new();
        ::std::mem::swap(&mut self.tasks,&mut new_vec);
        for i in new_vec {
            if !i.is_done() {
                self.tasks.push(i);
            }
        }
    }

    fn proc_messages_non_blocking(&mut self) -> bool {
        loop {
            let next = self.quick_check();
            if next.is_some() {
                let keepgoing = self.handle(next.unwrap());
                if !keepgoing {
                    return false;
                }
            } else {
                return true;
            }
        }
    }

    fn download_next(&mut self) {
        let (readres,chunkstart,chunkend) = {
            self.ensure_valid();
            let nextu = {
                let next = self.current_task();

                if next.is_none() {
                    return;
                }

                let res = next.unwrap();
                if res.is_done() {
                    return;
                }
                res
            };

            {
                let (chunkstart,chunkend) = nextu.next_chunk();
                let mut reader = self.rkit.file_helper.read(&nextu.file);
                let size = chunkend - chunkstart;
                let readres = reader.read(chunkstart,size);
                (readres,chunkstart,chunkend)
            }
        };

        if readres.is_ok() {
            {
                let mut unwrapped = readres.unwrap();
                let next = self.current_task().unwrap();
                (next.task.userdata_buffer_func)(
                    next.task.userdata,
                    chunkstart,chunkend,
                    unwrapped.as_ptr()
                );
            }
            {
                let next = self.current_task_mut().unwrap();
                if next.is_done() {
                    (next.task.userdata_arbitrary_message_func)(
                        next.task.userdata,
                        SAFE_DOWNLOADER_MSG_DOWNLOAD_SUCCESS,
                        std::ptr::null_mut()
                    );
                }
            }
            self.next_task();
        } else {
            // possibly report read fail
        }
    }

    fn perform_iteration(&mut self) -> bool {
        if self.tasks.len() > 0 {
            return self.main_tasks();
        } else {
            let recv = self.recv.recv();
            if recv.is_ok() {
                let res = self.handle(recv.unwrap());
                return res;
            } else {
                ::std::thread::sleep(
                    ::std::time::Duration::from_millis(100));
            }

            return true;
        }
    }

    fn launch_thread(recv: ::std::sync::mpsc::Receiver< DownloaderMsgs >) {
        println!("Thread lunched!");

        std::thread::spawn(
            move || {
                let kit = ReaderKit::unregistered_kit();
                if kit.is_err() {
                    return;
                }

                let mut local = DownloaderActorLocal::new(kit.unwrap(),recv);

                loop {
                    let curr = local.perform_iteration();
                    if !curr {
                        if local.endfunc.is_some() {
                            (local.endfunc.unwrap())(local.enddata);
                        }
                        return;
                    }
                    local.filter_done();
                }
            }
        );
    }

    fn quick_check(&mut self) -> Option< DownloaderMsgs > {
        let res = self.recv.try_recv();
        if res.is_err() {
            None
        } else {
            Some(res.unwrap())
        }
    }
}

impl Drop for DownloaderActor {
    fn drop(&mut self) {
        println!("Downloader actor destroyed!");
    }
}

#[no_mangle]
pub extern fn safe_file_downloader_new() -> *mut libc::c_void {
    let (remote,local) = DownloaderActor::new();
    let the_arc = Box::new( Arc::new( remote ) );
    DownloaderActorLocal::launch_thread(local);

    let raw = Box::into_raw(the_arc);
    unsafe {
        std::mem::transmute(raw)
    }
}

#[no_mangle]
pub extern fn safe_file_downloader_cleanup(
    ptr: *mut libc::c_void,
    ondone: extern fn(param: *mut libc::c_void),
    userdata: *mut libc::c_void)
{
    unsafe {
        let transmuted : *mut Arc<DownloaderActor> = std::mem::transmute(ptr);
        let boxed : Box< std::sync::Arc<DownloaderActor> > = Box::from_raw(transmuted);
        let actor = &**boxed;
        actor.send.send(DownloaderMsgs::Stop {
            donefunc: ondone,
            donedata: userdata,
        });
    }
}

#[repr(C)]
struct ScheduleArgs {
    userdata: *mut libc::c_void,
    userdata_buffer_func: extern fn(
        param: *mut libc::c_void,
        libc::uint64_t,
        libc::uint64_t,
        *const libc::uint8_t) -> libc::int32_t,
    userdata_next_range_func: extern fn(
        param: *mut libc::c_void,
        *mut libc::int64_t,
        *mut libc::int64_t) -> libc::int32_t,
    userdata_arbitrary_message_func: extern fn(
        param: *mut libc::c_void,
        msgtype: i32,
        param: *mut libc::c_void),
    userdata_destructor: extern fn(param: *mut libc::c_void),
    path: *const libc::c_schar,
}

#[no_mangle]
pub extern fn safe_file_downloader_schedule(
    ptr: *mut libc::c_void,args: *mut libc::c_void) -> libc::int32_t
{
    unsafe {
        let argptr: *mut ScheduleArgs = ::std::mem::transmute(args);
        let transmuted : *mut Arc<DownloaderActor> = std::mem::transmute(ptr);

        let task = DownloadTask {
            userdata: (*argptr).userdata,
            userdata_buffer_func:
                (*argptr).userdata_buffer_func,
            userdata_next_range_func:
                (*argptr).userdata_next_range_func,
            userdata_arbitrary_message_func:
                (*argptr).userdata_arbitrary_message_func,
            userdata_destructor: (*argptr).userdata_destructor,
        };

        let pathC = std::ffi::CStr::from_ptr((*argptr).path);
        let pathStr = pathC.to_str();
        if pathStr.is_err() {
            return 1;
        }

        let pathString = pathStr.unwrap().to_string();

        let sched_struct =
            DownloaderMsgs::Schedule {
                path: pathString, task: task
            };

        (*transmuted).send.send(sched_struct);

        return 0;
    }
}

quick_error! {
    #[derive(Debug)]
    pub enum GetReaderError {
        SafeDns(err: DnsError) {
            from()
            description("Safe DNS error")
            display("Safe DNS error: {:?}",err)
        }
        SafeNfs(err: NfsError) {
            from()
            description("Safe NFS error")
            display("Safe NFS error: {:?}",err)
        }
        InvalidPath(err: &'static str) {
            description(err)
            display("Path is invalid: {}",err)
        }
        DirectoryNotFound(err: &'static str) {
            description(err)
            display("No directory found: {}",err)
        }
        FileNotFound(err: &'static str) {
            description(err)
            display("No file found: {}",err)
        }
        Regex(err: &'static str) {
            description(err)
            display("Regex error: {}",err)
        }
    }
}

pub fn path_tokenizer(the_path: String) -> Vec<String> {
    the_path.split("/").filter(|a| !a.is_empty()).map(|a| a.to_string()).collect()
}

fn recursive_find_path(
    tokens: &Vec< String >,num: usize,
    root: DirectoryListing,
    dir_helper: &DirectoryHelper)
    -> Result< DirectoryListing,
               GetReaderError >
{
    if num < tokens.len() - 1 {
        let current = tokens[num].clone();

        let found = root.find_sub_directory(&current);
        match found {
            Some(val) => {
                let thekey = val.get_key();
                let next = dir_helper.get(thekey);
                match next {
                    Ok(val) => {
                        recursive_find_path(tokens,num + 1,val,dir_helper)
                    },
                    Err(err) => {
                        Err( GetReaderError::SafeNfs(err) )
                    },
                }
            },
            None => {
                Err( GetReaderError::DirectoryNotFound("Path doesn't exist.") )
            },
        }
    } else {
        Ok(root)
    }
}

fn get_file(
    rk: &ReaderKit,
    path: &String)
    -> Result< safe_core::nfs::file::File, GetReaderError >
{
    let trimmed = path.trim();
    let namergx = Regex::new(
        r"^([a-zA-Z0-9_-]+).([a-zA-Z0-9_.-]+)/([a-zA-Z0-9_./]+)$").unwrap();

    for i in namergx.captures_iter(trimmed) {
        let service = i.at(1).unwrap().to_string();
        let name = i.at(2).unwrap().to_string();
        let file = i.at(3).unwrap().to_string();

        let dir_key = rk.dns_ops
            .get_service_home_directory_key(
                &name,&service,None);

        let tokenized_path = path_tokenizer(file.clone());

        if 0 == tokenized_path.len() {
            return Err( GetReaderError::InvalidPath(
                "No tokens in the path.") )
        }

        match dir_key {
            Ok(val) => {
                let listing = rk.dir_helper.get(&val);

                match listing {
                    Ok(lst) => {
                        let reslisting = recursive_find_path(
                            &tokenized_path,0,lst,&rk.dir_helper);

                        if reslisting.is_err() {
                            return Err( reslisting.err().unwrap() );
                        }

                        let unwrapped = reslisting.unwrap();
                        let the_file = unwrapped.find_file(
                            tokenized_path.last().unwrap());

                        if the_file.is_none() {
                            return Err( GetReaderError::FileNotFound(
                                "Could not find file.") )
                        }

                        let the_file_res = the_file.unwrap();
                        return Ok(the_file_res.clone());
                    },
                    Err(err) => return Err( GetReaderError::SafeNfs(err) )
                }
            },
            Err(err) => return Err( GetReaderError::SafeDns(err) ),
        }
    }

    Err( GetReaderError::Regex("Could not parse url.") )
}

#[test]
fn it_works() {
}
