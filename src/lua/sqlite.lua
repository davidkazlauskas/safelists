
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

function sqlMoveFileValidation(toMoveId,dirId)
    local fileToMoveSelect =
        "(SELECT file_name FROM files WHERE file_id="
        .. toMoveId .. ")"

    return
           " SELECT CASE"
        .. " WHEN (EXISTS (SELECT file_name FROM files"
        .. "     WHERE dir_id=" .. dirId
        .. "     AND file_name=" .. fileToMoveSelect .. ")) THEN 1"
        .. " WHEN (EXISTS (SELECT file_name FROM files"
        .. "     WHERE dir_id=" .. dirId
        .. "     AND (file_name || '.ilist' =" .. fileToMoveSelect .. ""
        .. "     OR file_name || '.ilist.tmp' =" .. fileToMoveSelect .. "))) THEN 2"
        .. " ELSE 0"
        .. " END,"
        .. " file_name,file_size,file_hash_256 FROM files WHERE file_id="
        .. toMoveId
        .. ";"
end

function sqlMoveFileStatement(toMoveId,dirId)
    return
        "UPDATE files SET dir_id="
        .. dirId
        .. " WHERE file_id=" .. toMoveId
        .. ";"
end

function sqlMoveDirCondition(dirInId,dirToMoveId)
    return
           " SELECT CASE"
        -- dir is a parent of dir to move under
        .. " WHEN (" .. dirInId .. " IN"
        .. " ("
        .. "     WITH RECURSIVE"
        .. "     children(d_id) AS ("
        .. "           SELECT dir_id FROM directories "
        .. "               WHERE dir_parent=" .. dirToMoveId
        .. "           UNION ALL"
        .. "           SELECT dir_id"
        .. "           FROM directories JOIN children ON "
        .. "              directories.dir_parent=children.d_id "
        .. "     ) SELECT d_id FROM children"
        .. " )) THEN 1"
        -- dir under parent already
        .. " WHEN ((SELECT dir_parent FROM directories"
        .. "     WHERE dir_id=" .. dirToMoveId
        .. "     ) = " .. dirInId .. ") THEN 3"
        -- same name already under directory
        .. " WHEN (" .. "(SELECT dir_name FROM"
        .. "     directories WHERE dir_id=" .. dirToMoveId .. ") IN"
        .. "     ( SELECT dir_name FROM directories WHERE"
        .. "     dir_parent=" .. dirInId .. ")) THEN 2"
        .. " ELSE 0"
        .. " END;"
end

function sqlMoveDirStatement(dirToMoveId,dirId)
    return "UPDATE directories SET dir_parent="
           .. dirId .. " WHERE dir_id=" .. dirToMoveId .. ";"
end

function sqlUpdateFileHashStatement(fileId,hash)
    return
        "UPDATE files SET file_hash_256='"
        .. hash .. "' WHERE file_id='" .. fileId .. "';"
end

function sqlGetFileHash(fileId)
    return
        "SELECT file_hash_256 FROM files WHERE file_id='"
        .. fileId .. "';"
end

function sqlUpdateFileSizeStatement(fileId,size)
    return
        "UPDATE files SET file_size='"
        .. size .. "' WHERE file_id='" .. fileId .. "'"
        .. " AND file_size=-1;"
end

function sqlDeleteDirectoryRecursively(dirId)
    -- TODO: IMPORTANT recursive delete query, cold
    -- this does not delete directories recursively
    return
        "DELETE FROM directories WHERE dir_id=" .. dirId .. ";" ..
        "DELETE FROM mirrors WHERE file_id IN " ..
        "  (SELECT file_id FROM files WHERE dir_id=" .. dirId .. ");" ..
        "DELETE FROM files WHERE dir_id=" .. dirId .. ";"
end

function sqlUpdateDirectoryNameStatement(dirId,name)
    return
        "UPDATE directories SET dir_name='" .. name .. "'"
        .. " WHERE dir_id=" .. dirId .. " AND NOT EXISTS("
        .. " SELECT 1 FROM directories WHERE dir_name='".. name
        .. "' AND dir_parent=(SELECT dir_parent FROM directories "
        .. " WHERE dir_id=" .. dirId .. " )" .. ");"
end
