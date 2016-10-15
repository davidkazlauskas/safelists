
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

function sqlCheckForForbiddenFileNames(dirId,fileName)
    return    " SELECT CASE"
           .. " WHEN (EXISTS (SELECT file_name FROM files"
           .. "     WHERE dir_id=" .. dirId
           .. "     AND file_name='" .. fileName .. "')) THEN 1"
           .. " WHEN (EXISTS (SELECT file_name FROM files"
           .. "     WHERE dir_id=" .. dirId
           .. "     AND (file_name || '.ilist' ='" .. fileName .. "'"
           .. "     OR file_name || '.ilist.tmp' ='" .. fileName .. "'))) THEN 2"
           .. " ELSE 0"
           .. " END;"
end
