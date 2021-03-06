
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

function sqlCheckForForbiddenFileNamesUpdateNoQuotes(dirId,fileId,fileName)
    return    " SELECT CASE"
           .. " WHEN (EXISTS (SELECT file_name FROM files"
           .. "     WHERE dir_id=" .. dirId
           .. "     AND NOT file_id=" .. fileId
           .. "     AND lower(file_name)=lower(" .. fileName .. "))) THEN 1"
           .. " WHEN (EXISTS (SELECT file_name FROM files"
           .. "     WHERE dir_id=" .. dirId
           .. "     AND NOT file_id=" .. fileId
           .. "     AND (lower(file_name || '.ilist') = lower(" .. fileName .. ")"
           .. "     OR lower(file_name || '.ilist.tmp') = lower(" .. fileName .. ")))) THEN 2"
           .. " WHEN (EXISTS (SELECT dir_name FROM directories"
           .. "     WHERE dir_parent=" .. dirId
           .. "     AND lower(dir_name)=lower(" .. fileName .. "))) THEN 3"
           .. " WHEN (EXISTS (SELECT dir_name FROM directories"
           .. "     WHERE dir_parent=" .. dirId
           .. "     AND (lower(dir_name || '.ilist') = lower(" .. fileName .. ")"
           .. "     OR lower(dir_name || '.ilist.tmp') = lower(" .. fileName .. ")))) THEN 4"
           .. " ELSE 0"
           .. " END;"
end

function sqlCheckForForbiddenFileNamesUpdate(dirId,fileId,fileName)
    return sqlCheckForForbiddenFileNamesUpdateNoQuotes(dirId,fileId,"'" .. fileName .. "'")
end

function sqlFileIdSelect(dirId,fileName)
    return
        "SELECT file_id FROM files WHERE " ..
        "file_name='" .. fileName .. "' AND dir_id="
        .. dirId
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

    local currentFileIdSelect = sqlFileIdSelect(dirId,fileName)

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
    local allDirsSelect =
           " WITH RECURSIVE"
        .. " children(d_id) AS ("
        .. "       VALUES(" .. dirId .. ")"
        .. "       UNION ALL"
        .. "       SELECT dir_id"
        .. "       FROM directories JOIN children ON "
        .. "          directories.dir_parent=children.d_id "
        .. " ) SELECT d_id FROM children"
    return
        "DELETE FROM mirrors WHERE file_id IN " ..
        "  (SELECT file_id FROM files WHERE dir_id IN (" .. allDirsSelect .. "));" ..
        "DELETE FROM files WHERE dir_id IN (" .. allDirsSelect .. ");" ..
        "DELETE FROM directories WHERE dir_id IN (" .. allDirsSelect .. ");"
end

function sqlUpdateDirectoryNameStatement(dirId,name)
    return
        "UPDATE directories SET dir_name='" .. name .. "'"
        .. " WHERE dir_id=" .. dirId .. " AND NOT EXISTS("
        .. " SELECT 1 FROM directories WHERE dir_name='".. name
        .. "' AND dir_parent=(SELECT dir_parent FROM directories "
        .. " WHERE dir_id=" .. dirId .. " )" .. ");"
end

function sqlNewDirectoryStatement(dirParent,dirName)
    return
           "INSERT INTO directories (dir_name,dir_parent)"
        .. " SELECT '" .. dirName .. "', " .. dirParent
        .. " WHERE NOT EXISTS("
        .. " SELECT 1 FROM directories WHERE dir_name='".. dirName
        .. "' AND dir_parent=" .. dirParent .. ");"
end

function sqlSelectLastInsertedDirId()
    return
        "SELECT dir_id FROM"
        .. " directories WHERE"
        .. " rowid=last_insert_rowid();"
end

function sqlDeleteFile(fileId)
    return
        "DELETE FROM files WHERE file_id=" .. fileId .. ";"
        .. "DELETE FROM mirrors WHERE file_id=" .. fileId .. ";"
end

function sqlSelectAllFilesForSession()
    -- query must select four fields, file_id, path_to_download, file_size, file_hash
    return
        "SELECT file_id, path_name || file_name, file_size, file_hash_256 " ..
        "FROM files " ..
        "LEFT OUTER JOIN " ..
        "(  " ..
        "   WITH RECURSIVE " ..
        "   children(d_id,path_name) AS ( " ..
        "      SELECT dir_id,'' FROM directories WHERE dir_name='root' AND dir_id=1 " ..
        "      UNION ALL " ..
        "      SELECT dir_id,children.path_name || dir_name || '/' " ..
        "      FROM directories JOIN children ON directories.dir_parent=children.d_id " ..
        "   ) SELECT d_id,path_name FROM children   " ..
        ")  " ..
        "ON dir_id=d_id; "
end

function sqlSelectOneFileForSession(fileId)
    -- query must select four fields, file_id, path_to_download, file_size, file_hash
    return
        "SELECT file_id, file_name, file_size, file_hash_256 " ..
        "FROM files " ..
        "WHERE file_id=" .. fileId .. "; "
end

function sqlSelectOneDirectoryForSession(dirId)
    -- query must select four fields, file_id, path_to_download, file_size, file_hash
    return
        "SELECT file_id, path_name || file_name, file_size, file_hash_256 " ..
        "FROM files " ..
        "LEFT OUTER JOIN " ..
        "(  " ..
        "   WITH RECURSIVE " ..
        "   children(d_id,path_name) AS ( " ..
        "      SELECT dir_id,'' FROM directories WHERE dir_id=" .. dirId .. " " ..
        "      UNION ALL " ..
        "      SELECT dir_id,children.path_name || dir_name || '/' " ..
        "      FROM directories JOIN children ON directories.dir_parent=children.d_id " ..
        "   ) SELECT d_id,path_name FROM children   " ..
        ")  " ..
        "ON dir_id=d_id " ..
        "WHERE d_id IS NOT NULL; "
end

function sqlSelectDirNameById(dirId)
    return "SELECT dir_name FROM directories WHERE dir_id=" .. dirId
end
