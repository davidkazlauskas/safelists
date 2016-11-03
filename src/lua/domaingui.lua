
-- domain globals and domain functions are passed
function fileBrowserRightClickHandler(dg,df)
    local menuModel = nil

    local currentEntityId, isDir = df.getCurrentEntityId()
    if (currentEntityId > 0 and isDir) then
        menuModel = { "Download directory", "New directory", "Move directory", "Delete directory", "Rename directory", "New file" }
        if (currentEntityId == 1) then
            -- root is unmovable, unrenamable and undeletable
            menuModel = { "New directory", "New file" }
        end
        --"Download directory",
    elseif (currentEntityId > 0 and not isDir) then
        menuModel = { "Download file", "Edit file", "Delete file", "Move file" }
        --"Download file",
        -- TODO: localize labels not to depend on them
    else
        return
    end

    return
        makePopupMenuModel(
            dg.ctx,menuModel,
            function(result)
                arraySwitch(result+1,menuModel,
                    arrayBranch("Move directory",function()
                        dg.currentDirToMoveId = df.getCurrentEntityId()
                        if (dg.currentDirToMoveId ~= -1) then
                            df.setStatus("Press on node under which to move")
                            dg.shouldMoveDir = true
                        end
                    end),
                    arrayBranch("Delete directory",instrument(function()
                        local thisCorout = coroutine.running()

                        df.okCancelDialog(
                            "Delete directory?",
                            "This action will delete selected directory recursively.",
                            resumerCallback(thisCorout))

                        local contRes = coroutine.yield()

                        if (contRes ~= "ok") then
                            return
                        end

                        local currentDirId = df.getCurrentEntityId()
                        if (currentDirId ~= -1) then
                            if (currentDirId == 1) then
                                df.setStatus("Root cannot be deleted.")
                                return
                            end
                            local asyncSqlite = dg.currentAsyncSqlite
                            if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                return
                            end
                            local wholeDir = whole(currentDirId)
                            df.execSqliteOnHandler(asyncSqlite,sqlDeleteDirectoryRecursively(wholeDir))
                            df.deleteSelectedDir()
                            df.updateRevision()
                        else
                            df.setStatus("No directory selected.")
                        end
                    end)),
                    arrayBranch("Rename directory",instrument(function()
                        local dialog = df.namedMessageable("singleInputDialog")

                        local showOrHide = function(val)
                            df.message(dialog,VSig("INDLG_InShowDialog"),VBool(val))
                        end

                        local setDlgErr = function(val)
                            df.message(dialog,
                                VSig("INDLG_InSetErrLabel"),VString(val))
                        end

                        local dirName = df.messageRetValues(dg.mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
                        local dirId = whole(df.getCurrentEntityId())
                        local parentDirId = whole(df.getCurrentFileParent())

                        df.message(dialog,VSig("INDLG_InSetLabel"),VString(
                            "Specify new folder name to rename  " .. dirName .. "."
                        ))

                        df.message(dialog,VSig("INDLG_InSetValue"),VString(dirName))

                        local newId = dg.objRetainer:newId()

                        local thisCorout = coroutine.running()

                        local handler = df.makeLuaMatchHandler(
                            VMatch(resumerCallbackSwitch(thisCorout,"ok"),"INDLG_OutOkClicked"),
                            VMatch(resumerCallbackSwitch(thisCorout,"cancel"),"INDLG_OutCancelClicked")
                        )

                        dg.objRetainer:retain(newId,handler)

                        df.message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
                        showOrHide(true)

                        while true do
                            local btnLabel = coroutine.yield()

                            if (btnLabel == "ok") then
                                print("Ok renamed!")
                                local outName = df.messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                                if (outName == "") then
                                    setDlgErr("Some directory name must be specified.")
                                elseif (not isValidFilename(outName)) then
                                    setDlgErr("Directory name entered contains invalid characters.")
                                else
                                    local validationQuery =
                                        sqlCheckForForbiddenFileNamesUpdate(
                                            parentDirId,-1,outName)

                                    local asyncSqlite = dg.currentAsyncSqlite
                                    df.messageAsyncWCallback(
                                        asyncSqlite,
                                        resumerCallbackValues(thisCorout),
                                        VSig("ASQL_OutSingleNum"),
                                        VString(validationQuery),
                                        VInt(-1),
                                        VBool(false)
                                    )

                                    local outNum = coroutine.yield()._3

                                    if (outNum == 1) then
                                        setDlgErr("File with name " .. outName .. " already exists.")
                                    elseif (outNum == 2) then
                                        setDlgErr("Name is forbidden.")
                                    elseif (outNum == 3) then
                                        setDlgErr("Directory with name " .. outName .. " already exists.")
                                    elseif (outNum == 4) then
                                        setDlgErr("Name is forbidden.")
                                    else
                                        df.messageAsyncWCallback(
                                            asyncSqlite,
                                            resumerCallback(thisCorout),
                                            VSig("ASQL_Execute"),
                                            VString(sqlUpdateDirectoryNameStatement(dirId,outName))
                                        )

                                        -- wait for upper statement
                                        coroutine.yield()

                                        df.updateRevision()

                                        df.message(dg.mainWnd,
                                            VSig("MWI_InSetCurrentDirName"),
                                            VString(outName))

                                        showOrHide(false)
                                        dg.objRetainer:release(newId)
                                        return
                                    end
                                end
                            elseif (btnLabel == "cancel") then
                                print("Cancel rename!")
                                showOrHide(false)
                                dg.objRetainer:release(newId)
                                return
                            end
                        end
                    end)),
                    arrayBranch("Download directory",function()
                        local currentDirId = df.getCurrentEntityId()
                        if (currentDirId ~= -1) then
                            local query = sqlSelectOneDirectoryForSession(whole(currentDirId))
                            downloadSessionHandler(df,dh,query)
                        end
                    end),
                    arrayBranch("New directory",instrument(function()
                        local dialog = df.namedMessageable("singleInputDialog")

                        local showOrHide = function(val)
                            df.message(dialog,VSig("INDLG_InShowDialog"),VBool(val))
                        end

                        local setDlgErr = function(val)
                            df.message(dialog,
                                VSig("INDLG_InSetErrLabel"),VString(val))
                        end

                        df.message(dialog,
                            VSig("INDLG_InSetParent"),
                            VMsg(dg.mainWnd))

                        local dirName = df.messageRetValues(dg.mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
                        local dirId = df.getCurrentEntityId()
                        local dirIdWhole = whole(dirId)

                        df.message(dialog,VSig("INDLG_InSetLabel"),VString(
                            "Specify new folder name to create under " .. dirName .. "."
                        ))

                        local newId = dg.objRetainer:newId()

                        local thisCorout = coroutine.running()

                        local handler = df.makeLuaMatchHandler(
                            VMatch(resumerCallbackSwitch(thisCorout,"ok"),"INDLG_OutOkClicked"),
                            VMatch(resumerCallbackSwitch(thisCorout,"cancel"),"INDLG_OutCancelClicked")
                        )

                        dg.objRetainer:retain(newId,handler)

                        df.message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
                        showOrHide(true)

                        while true do
                            local btnLabel = coroutine.yield()

                            if (btnLabel == "ok") then
                                print("Ok!")
                                local outName = df.messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                                -- more thorough user input check should be performed
                                if (outName == "") then
                                    setDlgErr("Some directory name must be specified.")
                                    -- I'd love to just continue loop from here...
                                elseif (not isValidFilename(outName)) then
                                    setDlgErr("Directory name entered contains invalid characters.")
                                else
                                    setDlgErr("")

                                    local asyncSqlite = dg.currentAsyncSqlite
                                    if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                        return
                                    end

                                    local validationQuery =
                                        sqlCheckForForbiddenFileNamesUpdate(
                                            dirIdWhole,-1,outName)

                                    df.messageAsyncWCallback(
                                        asyncSqlite,
                                        resumerCallbackValues(thisCorout),
                                        VSig("ASQL_OutSingleNum"),
                                        VString(validationQuery),
                                        VInt(-1),
                                        VBool(false)
                                    )

                                    local outNum = coroutine.yield()._3

                                    if (outNum == 1) then
                                        setDlgErr("File with name " .. outName .. " already exists.")
                                    elseif (outNum == 2) then
                                        setDlgErr("Name is forbidden.")
                                    elseif (outNum == 3) then
                                        setDlgErr("Directory with name " .. outName .. " already exists.")
                                    elseif (outNum == 4) then
                                        setDlgErr("Name is forbidden.")
                                    else
                                        local theQuery = sqlNewDirectoryStatement(dirIdWhole,outName)

                                        df.messageAsyncWCallback(
                                            asyncSqlite,
                                            resumerCallback(thisCorout),
                                            VSig("ASQL_Execute"),
                                            VString(theQuery)
                                        )

                                        -- wait for upper query to complete
                                        coroutine.yield()

                                        df.messageAsyncWCallback(
                                            asyncSqlite,
                                            function(back)
                                                local newId = back:values()._3
                                                df.message(dg.mainWnd,
                                                    VSig("MWI_InAddChildUnderCurrentDir"),
                                                    VString(outName),VInt(newId))
                                            end,
                                            VSig("ASQL_OutSingleNum"),
                                            VString(sqlSelectLastInsertedDirId()),
                                            VInt(-1),
                                            VBool(false)
                                        )

                                        df.updateRevision()
                                        -- todo: optimize, don't reload all
                                        showOrHide(false)
                                        dg.objRetainer:release(newId)
                                        return
                                    end
                                end
                            elseif (btnLabel == "cancel") then
                                print("Cancel!")
                                showOrHide(false)
                                dg.objRetainer:release(newId)
                                return
                            end
                        end
                    end)),
                    arrayBranch("Download file",function()
                        print("Download file clicked")
                        local currentFileId = whole(df.getCurrentEntityId())
                        downloadSessionHandler(df,dh,sqlSelectOneFileForSession(currentFileId))
                    end),
                    arrayBranch("New file",function()
                        print("New file clicked")
                        local currentFileId = df.getCurrentEntityId()
                        df.newFileDialog(
                            function(result,dialog)
                                local firstValidation =
                                    df.validateNewFileDialogFirst(result,dialog)
                                if (not firstValidation) then
                                    return false
                                end

                                local thisCorout = coroutine.running()

                                -- great success, form validation passed
                                -- TODO: how to prolong file dialog?
                                df.addNewFileUnderCurrentDir(
                                    result,dialog,resumerCallback(thisCorout))
                                return coroutine.yield()
                            end
                        )
                    end),
                    arrayBranch("Edit file",function()
                        local dirId = df.getCurrentFileParent()
                        df.modifyFileDialog(
                            currentEntityId,
                            function(result,orig,dialog)
                                local firstValidation =
                                    df.validateNewFileDialogFirst(result,dialog)
                                if (not firstValidation) then
                                    return false
                                end

                                local thisCorout = coroutine.running()

                                df.updateFileFromDiff(currentEntityId,dirId,
                                    result,orig,dialog,resumerCallback(thisCorout))
                                return coroutine.yield()
                            end
                        )
                    end),
                    arrayBranch("Move file",function()
                        df.setStatus("Select folder to move file to.")
                        dg.fileToMove = currentEntityId
                        dg.shouldMoveFile = true
                    end),
                    arrayBranch("Delete file",instrument(function()
                        local thisCorout = coroutine.running()

                        df.okCancelDialog(
                            "Delete file?",
                            "This action will delete the selected file.",
                            resumerCallback(thisCorout))

                        local contRes = coroutine.yield()

                        if (contRes ~= "ok") then
                            return
                        end

                        local currentFileId = df.getCurrentEntityId()
                        -- we know that this is file because
                        -- we wouldn't see this menu
                        if (currentFileId ~= -1) then
                            local asyncSqlite = dg.currentAsyncSqlite
                            if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                return
                            end
                            df.messageAsync(asyncSqlite,
                                VSig("ASQL_Execute"),
                                VString(sqlDeleteFile(whole(currentFileId))))
                            df.message(dg.mainWnd,VSig("MWI_InDeleteSelectedDir"))
                            df.updateRevision()
                        else
                            df.setStatus("No directory selected.")
                        end
                    end))
                )
            end
        )
end

function downloadSessionHandler(df,dh,selectQuery)
    local dlFactory = df.namedMessageable("dlSessionFactory")
    local dialogService = df.namedMessageable("dialogService")
    local asyncSqlite = dg.currentAsyncSqlite
    if (messageablesEqual(VMsgNil(),asyncSqlite)) then
        assert( false, "Didn't expect download request" ..
            " with null safelist.")
        return
    end

    local isCurrentDead = function()
        return df.messageRetValues(
            asyncSqlite,
            VSig("ASQL_IsDead"),
            VBool(true)
        )._2
    end

    local afterDirectory = function(downloadPath)

        if (downloadPath == "") then
            return
        end

        assert( downloadPath[#downloadPath] ~= "/",
            "Don't expect slash at the end." )

        downloadPath = downloadPath .. "/safelist_session"

        if (dg.currentSessions[downloadPath] == "t") then
            df.messageBox(
                "In progress",
                "Safelist already being downloaded."
            )
            return
        end

        dg.currentSessions[downloadPath] = "t"

        --print("Pre col: " .. collectgarbage('count'))
        --collectgarbage('collect')
        --print("Post col: " .. collectgarbage('count'))

        local currSess = dg.dm:newSession()
        currSess.loggedErrors = false

        local appendLog = function(theStr)
            df.appendSessionLog(currSess,theStr)
        end

        local newId = df.newObjectId()
        local handlerWeak = nil
        local handler = df.makeLuaMatchHandler(
            VMatch(function(natPack,val)
                local values = val:values()
                local dl = currSess:keyDownload(values._2)
                -- dead progress update
                if (nil ~= dl) then
                    local done = values._3
                    local total = values._4
                    dg.dm:incRevision()
                    dl:setProgress(done,total)
                end
                local newBytes = values._5
                dg.downloadSpeedChecker:regBytes(newBytes)
            end,"SLD_OutProgressUpdate","int","double","double","double"),
            VMatch(function(natpack,val)
                local valTree = val:values()
                local newKey = valTree._2
                local newPath = valTree._3
                dg.dm:incRevision()
                currSess:addDownload(newKey,newPath)
            end,"SLD_OutStarted","int","string"),
            VMatch(function(natpack,val)
                local valTree = val:values()
                local delKey = valTree._2
                dg.dm:incRevision()
                currSess:removeDownload(delKey)
            end,"SLD_OutSingleDone","int"),
            VMatch(function(natpack,val)
                -- MAKE-PRETTY
                local valTree = val:values()
                local delKey = valTree._2
                local dkWh = whole(delKey)
                local theDl = currSess:keyDownload(delKey)
                local thePath = theDl:getPath()
                print("File not found brah: |" .. dkWh .. "|" .. thePath .. "|")
                appendLog("File not found: " .. thePath)
                dg.dm:incRevision()
                currSess:removeDownload(delKey)
            end,"SLD_OutFileNotFound","int"),
            VMatch(function()
                print('Downloaded!')
                dg.currentSessions[downloadPath] = nil
                --dg.dm:incRevision()
                --dg.dm:dropSession(currSess)
                --objRetainer:release(newId)
            end,"SLD_OutDone"),
            VMatch(function(natpack,val)
                -- back in the day use counts were incremented
                -- but it's a waste of time because now hashes
                -- don't verify that two safelists are the same
                return
            end,"SLD_OutMirrorUsed","int","string"),
            VMatch(instrument(function(natPack,val)
                local thisCorout = coroutine.running()
                local values = val:values()
                local hash = values._3
                if (nil == asyncSqlite or isCurrentDead()) then
                    return
                end

                local id = values._2
                local idWhole = whole(id)
                local theDl = currSess:keyDownload(id)
                local thePath = theDl:getPath()

                df.messageAsyncWCallback(asyncSqlite,
                    resumerCallbackValues(thisCorout),
                    VSig("ASQL_OutSingleRow"),
                    VString(sqlGetFileHash(idWhole)),
                    VString(""),
                    VBool(false))

                local outVal = coroutine.yield()
                local qhash = outVal._3

                assert( outVal._4, "Query failed..." )
                --assert( qhash ~= hash, "Hash collision, hash is different."
                    --.. " (todo: handle this case)" )

                if (qhash ~= hash and qhash ~= "") then
                    appendLog(
                      "Hash mismatch: " .. thePath
                      .. " is reported to be of hash \"" .. qhash .. "\""
                      .. " but turns out to be \"" .. hash .. "\"."
                      .. " Are mirrors pointing to the same file?"
                    )
                else
                    df.messageAsync(
                        asyncSqlite,
                        VSig("ASQL_Execute"),
                        VString(sqlUpdateFileHashStatement(idWhole,hash))
                    )
                    df.updateRevision()
                end
            end),"SLD_OutHashUpdate","int","string"),
            VMatch(function(natPack,out)
                if (isCurrentDead()) then
                    return
                end

                local val = out:values()
                local id = val._2
                local idWhole = whole(id)
                local newSize = val._3
                -- size collision already checked with assert
                -- TODO: update size in GUI model
                df.messageAsync(
                    asyncSqlite,
                    VSig("ASQL_Execute"),
                    VString(sqlUpdateFileSizeStatement(idWhole,whole(newSize)))
                )
                df.updateRevision()
            end,"SLD_OutSizeUpdate","int","double"),
            VMatch(function(natPack,out)
                local val = out:values()
                local id = val._2
                local exp = val._3
                local real = val._4
                local theDl = currSess:keyDownload(id)
                appendLog(
                  "Size mismatch: " .. theDl:getPath()
                  .. " is reported to be of size " .. whole(exp)
                  .. " but turns out to be " .. whole(real) .. "."
                  .. " Are mirrors pointing to the same file?"
                )
            end,"SLD_OutSizeMismatch","int","double","double"),
            VMatch(function()
                print('Safelist session dun! Downloading...')
                local locked = handlerWeak:lockPtr()
                assert( nil ~= locked , "Locking weak ptr gives nil..." )
                local dlHandle = df.messageRetValues(dlFactory,
                    VSig("SLDF_InNewAsync"),
                    VString(downloadPath),
                    VMsg(locked),
                    VMsg(nil)
                )._4
                assert( dlHandle ~= nil )
            end,"SLDF_OutCreateSessionDone"),
            VMatch(function(natPack,val)
                local vals = val:values()
                print("The total: " .. vals._2)
                currSess:setTotalDownloads(vals._2)
            end,"SLD_OutTotalDownloads","int")
        )

        handlerWeak = handler:getWeak()
        df.retainObjectWId(newId,handler)

        df.message(dlFactory,
            VSig("SLDF_CreateSession"),
            VMsg(asyncSqlite),
            VMsg(handler),
            VString(downloadPath),
            VString(selectQuery)
        )
    end

    local nId = df.newObjectId()

    local handler = df.makeLuaMatchHandler(
        VMatch(function(natPack,val)
            local outPath = val:values()._2
            afterDirectory(outPath)
            df.releaseObject(nId)
        end,"GDS_OutNotifyPath","string")
    )

    df.retainObjectWId(nId,handler)

    local outVal = df.message(dialogService,
        VSig("GDS_DirChooserDialog"),
        VMsg(df.mainWnd()),
        VString("Select download location."),
        VMsg(handler))
end
