#![feature(plugin)]
#![plugin(regex_macros)]

extern crate libc;
extern crate safe_nfs;
extern crate safe_dns;
extern crate safe_core;
extern crate regex;
#[macro_use] extern crate quick_error;

use std::sync::{Arc,Mutex};

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

impl<'a> DownloadTaskState<'a> {
    //fn new(client: Arc<Mutex<::safe_core::client::Client>>)
     //-> DownloadTaskState<'a>
    //{

    //}
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
    dir_helper: ::safe_nfs::helper::directory_helper::DirectoryHelper)
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

fn get_reader<'a>(
    client: Arc<Mutex<::safe_core::client::Client>>,
    dns_ops: ::safe_dns::dns_operations::DnsOperations,
    path: String)
    -> Result< ::safe_nfs::helper::reader::Reader<'a>, GetReaderError >
{
    let trimmed = path.trim();
    let namergx = regex!(
        r"^([a-zA-Z0-9_-]+).([a-zA-Z0-9_.-]+)/([a-zA-Z0-9_./]+)$");

    for i in namergx.captures_iter(trimmed) {
        let service = i.at(1).unwrap().to_string();
        let name = i.at(2).unwrap().to_string();
        let file = i.at(3).unwrap().to_string();

        let dir_key = dns_ops
            .get_service_home_directory_key(
                &name,&service,None);

        let tokenized_path = path_tokenizer(file.clone());

        if 0 == tokenized_path.len() {
            return Err( GetReaderError::InvalidPath(
                "No tokens in the path.") )
        }

        match dir_key {
            Ok(val) => {
                let dir_helper = ::safe_nfs::helper::directory_helper
                    ::DirectoryHelper::new(client.clone());
                let listing = dir_helper.get(&val);

                match listing {
                    Ok(lst) => {
                        let reslisting = recursive_find_path(
                            &tokenized_path,0,lst,dir_helper);

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

                        let file_helper = ::safe_nfs::helper::file_helper
                            ::FileHelper::new(client.clone());

                        let mut reader = file_helper.read(the_file_res);
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
