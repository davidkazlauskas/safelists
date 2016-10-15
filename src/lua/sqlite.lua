
function sqlRevNumber()
    return "SELECT revision_number,datetime(modification_date,'unixepoch','localtime')" ..
           " FROM metadata;"
end

function sqlGetMirrorUsesForFile(fileIdWhole)
    return "SELECT file_name,file_size,file_hash_256,"
           .. " sum(use_count),group_concat(url,',') FROM mirrors"
           .. " LEFT OUTER JOIN files"
           .. " ON files.file_id=mirrors.file_id"
           .. " WHERE mirrors.file_id=" .. fileIdWhole
           .. " GROUP BY files.file_id;"
end
