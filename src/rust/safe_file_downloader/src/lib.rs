#![feature(plugin)]
#![plugin(regex_macros)]

extern crate libc;
extern crate safe_nfs;
extern crate safe_dns;
extern crate safe_core;
extern crate regex;
#[macro_use] extern crate quick_error;

use std::sync::{Arc,Mutex};

const CHUNK_SIZE : u64 = 64 * 1024;

// chunk size for each download
fn get_chunk_size() -> u64 {
    CHUNK_SIZE
}

struct ReaderKit {
    client: Arc< Mutex< ::safe_core::client::Client > >,
    dns_ops: ::safe_dns::dns_operations::DnsOperations,
    dir_helper: ::safe_nfs::helper::directory_helper::DirectoryHelper,
    file_helper: ::safe_nfs::helper::file_helper::FileHelper,
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
        let client = ::safe_core::client
            ::Client::create_unregistered_client();

        if client.is_err() {
            return Err( GetReaderKitError::CouldNotCreateClient(
                "Could not create unregistered client.") );
        }

        let clientu = Arc::new( Mutex::new( client.unwrap() ) );

        let dns_ops = ::safe_dns::dns_operations
            ::DnsOperations::new_unregistered(clientu.clone());
        let dir_helper = ::safe_nfs::helper::directory_helper
            ::DirectoryHelper::new(clientu.clone());
        let file_helper = ::safe_nfs::helper::file_helper
            ::FileHelper::new(clientu.clone());

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

struct DownloadTaskState {
    size: u64,
    progress: u64
}

impl DownloadTaskState {
    fn new(rk: &ReaderKit,file: &::safe_nfs::file::File)
     -> DownloadTaskState
    {
        let reader = rk.file_helper.read(&file);
        let size = reader.size();

        let res =
            DownloadTaskState {
                size: size,
                progress: 0,
            };

        res
    }
}

struct DownloadTask {
    userdata: *mut libc::c_void,
    userdata_buffer_func: extern fn(
        param: *mut libc::c_void,
        libc::uint64_t,
        libc::uint64_t,
        *mut libc::uint8_t),
    userdata_destructor: extern fn(param: *mut libc::c_void),
}

impl Drop for DownloadTask {
    fn drop(&mut self) {
        (self.userdata_destructor)(self.userdata);
        println!("Task destroyed!");
    }
}

unsafe impl Send for DownloadTask {}

enum DownloaderMsgs {
    Schedule { path: String,task: DownloadTask },
    Stop,
}

struct DownloadTaskWRreader {
    task: DownloadTask,
    file: ::safe_nfs::file::File,
    state: DownloadTaskState,
}

impl DownloadTaskWRreader {
    fn next_chunk(&self,chunk_size: u64) -> (u64,u64) // start, length
    {
        let remaining = self.remaining_size(chunk_size);
        (self.state.progress,remaining)
    }

    fn remaining_size(&self,to_take: u64) -> u64 {
        let remaining = self.state.size - self.state.progress;
        if remaining > to_take {
            to_take
        } else {
            remaining
        }
    }

    fn advance(&mut self,chunk_size: u64) {
        let remaining = self.remaining_size(chunk_size);
        self.state.progress += remaining;
    }

    fn is_done(&self) -> bool {
        self.state.size == self.state.progress
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
        }
    }

    fn handle(&mut self,msg: DownloaderMsgs) -> bool {
        match msg {
            DownloaderMsgs::Schedule { path: path, task: task } => {
                self.add_task(path,task);
            },
            DownloaderMsgs::Stop => {
                return false;
            },
        }

        return true;
    }

    fn add_task(&mut self,path: String,task: DownloadTask)
    -> Result< bool, AddTaskError > {
        let file = get_file(&self.rkit,&path);
        if file.is_err() {
            return Err( AddTaskError
                ::FileNotFound("Could not find file.") );
        }

        let fileu = file.unwrap();

        let state =
            {
                let reader = self.rkit.file_helper.read(&fileu);
                let size = reader.size();
                DownloadTaskState {
                    size: size,
                    progress: 0,
                }
            };

        let mut res = DownloadTaskWRreader {
            task: task,
            file: fileu,
            state: state,
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

    fn next_task(&mut self) {
        self.iter += 1;

        if self.iter >= self.tasks.len() {
            self.iter = 0;
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
                let (chunkstart,chunkend) =
                    nextu.next_chunk(get_chunk_size());
                let mut reader = self.rkit.file_helper.read(&nextu.file);
                (reader.read(chunkstart,chunkend),chunkstart,chunkstart+chunkend)
            }
        };

        if readres.is_ok() {
            {
                let mut unwrapped = readres.unwrap();
                let next = self.current_task().unwrap();
                (next.task.userdata_buffer_func)(
                    next.task.userdata,
                    chunkstart,chunkend,
                    unwrapped.as_mut_slice().as_mut_ptr()
                );
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
                        return;
                    }
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
pub extern fn safe_file_downloader_cleanup(ptr: *mut libc::c_void) {
    unsafe {
        let transmuted : *mut Arc<DownloaderActor> = std::mem::transmute(ptr);
        let boxed : Box< std::sync::Arc<DownloaderActor> > = Box::from_raw(transmuted);
    }
}

quick_error! {
    #[derive(Debug)]
    pub enum GetReaderError {
        SafeDns(err: ::safe_dns::errors::DnsError) {
            from()
            description("Safe DNS error")
            display("Safe DNS error: {:?}",err)
        }
        SafeNfs(err: ::safe_nfs::errors::NfsError) {
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
    root: ::safe_nfs::directory_listing::DirectoryListing,
    dir_helper: &::safe_nfs::helper::directory_helper::DirectoryHelper)
    -> Result< ::safe_nfs::directory_listing::DirectoryListing,
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
    -> Result< ::safe_nfs::file::File, GetReaderError >
{
    let trimmed = path.trim();
    let namergx = regex!(
        r"^([a-zA-Z0-9_-]+).([a-zA-Z0-9_.-]+)/([a-zA-Z0-9_./]+)$");

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
