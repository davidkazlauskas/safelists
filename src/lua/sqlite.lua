
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

function sqlCheckForForbiddenFileNamesUpdate(dirId,fileId,fileName)
    return    " SELECT CASE"
           .. " WHEN (EXISTS (SELECT file_name FROM files"
           .. "     WHERE dir_id=" .. dirId
           .. "     AND NOT file_id=" .. fileId
           .. "     AND file_name='" .. fileName .. "')) THEN 1"
           .. " WHEN (EXISTS (SELECT file_name FROM files"
           .. "     WHERE dir_id=" .. dirId
           .. "     AND NOT file_id=" .. fileId
           .. "     AND (file_name || '.ilist' ='" .. fileName .. "'"
           .. "     OR file_name || '.ilist.tmp' ='" .. fileName .. "'))) THEN 2"
           .. " ELSE 0"
           .. " END;"
end

-- mirrors are string to be split by new lines
function sqlAddNewFileQuery(dirId,fileName,fileSize,fileHash,fileMirrors)
    local sqliteTransaction = {}
    local push = function(string)
        table.insert(sqliteTransaction,string)
    end

    push("BEGIN;")

    push("INSERT INTO files "
        .. "(dir_id,file_name,file_size,file_hash_256) VALUES(")

    push(dirId .. ",")
    push("'" .. fileName .. "',")
    push(fileSize .. ",")
    push("'" .. fileHash .. "'")

    push(");")

    local currentFileIdSelect =
        "SELECT file_id FROM files WHERE " ..
        "file_name='" .. fileName .. "' AND dir_id="
        .. dirId

    local mirrSplit = string.split(fileMirrors,"\n")

    for k,v in ipairs(mirrSplit) do
        push("INSERT INTO mirrors (file_id,url,use_count) VALUES(")
        push("(" .. currentFileIdSelect .. "),'" .. v .. "',0")
        push(");")
    end

    push("COMMIT;")

    return table.concat(sqliteTransaction," ")
end

-- mirrors are string to be split by new lines
function sqlUpdateFileQuery(fileId,diffName,diffSize,diffHash,diffMirrors)
    local updateString = {}
    local push = function(value)
        table.insert(updateString,value)
    end

    local isFirst = true
    local delim = function()
        if (not isFirst) then
            push(",")
        end
        isFirst = false
    end

    -- the action
    push("BEGIN;")

    if (diffName ~= nil
        or diffSize ~= nil
        or diffHash ~= nil)
    then
        push("UPDATE files SET ")
        if (diffName ~= nil) then
            delim()
            push("file_name='" .. diffName .. "'")
            isFirst = false
        end

        if (diffSize ~= nil) then
            delim()
            push("file_size=" .. diffSize)
            isFirst = false
        end

        if (diffHash ~= nil) then
            delim()
            push("file_hash_256='" .. diffHash .. "'")
            isFirst = false
        end

        push(" WHERE file_id=" .. fileId .. ";")
    end

    if (diffMirrors ~= nil) then
        local mirrSplit = string.split(diffMirrors,"\n")

        for k,v in ipairs(mirrSplit) do
            push("INSERT INTO mirrors (file_id,url,use_count) SELECT ")
            push("" .. fileId .. ",'" .. v .. "',0")
            push(" WHERE '" .. v .. "' NOT IN ")
            push(" (SELECT url FROM mirrors WHERE file_id=" .. fileId .. ");")
        end

        push("DELETE FROM mirrors WHERE file_id=" .. fileId)
        for k,v in ipairs(mirrSplit) do
            push(" AND NOT url='" .. v .. "' ")
        end
        push(";")
    end

    push("COMMIT;")

    return table.concat(updateString," ")
end
