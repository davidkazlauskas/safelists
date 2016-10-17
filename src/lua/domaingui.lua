
-- domain globals and domain functions are passed
function fileBrowserRightClickHandler(dg,df)
    local menuModel = nil

    local currentEntityId, isDir = df.getCurrentEntityId()
    if (currentEntityId > 0 and isDir) then
        menuModel = { "New directory", "Move directory", "Delete directory", "Rename directory", "New file" }
        if (currentEntityId == 1) then
            -- root is unmovable, unrenamable and undeletable
            menuModel = { "New directory", "New file" }
        end
        --"Download directory",
        -- TODO: implement download directory
    elseif (currentEntityId > 0 and not isDir) then
        menuModel = { "Edit file", "Delete file", "Move file" }
        --"Download file",
        -- TODO: localize labels not to depend on them
        -- TODO: implement download
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
                    arrayBranch("Delete directory",function()
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
                            -- TODO: ask if really want to delete
                            df.execSqliteOnHandler(asyncSqlite,sqlDeleteDirectoryRecursively(wholeDir))
                            df.deleteSelectedDir()
                            df.updateRevision()
                        else
                            df.setStatus("No directory selected.")
                        end
                    end),
                    arrayBranch("Rename directory",function()
                        local dialog = df.namedMessageable("singleInputDialog")

                        local showOrHide = function(val)
                            df.message(dialog,VSig("INDLG_InShowDialog"),VBool(val))
                        end

                        local dirName = df.messageRetValues(dg.mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
                        local dirId = df.getCurrentEntityId()

                        if (dirName == "[unselected]") then
                            df.setStatus("No directory was selected to create new one.")
                            return
                        end

                        if (dirName == "root" and dirId == 1) then
                            df.setStatus("Cannot rename root.")
                            return
                        end

                        df.message(dialog,VSig("INDLG_InSetLabel"),VString(
                            "Specify new folder name to rename  " .. dirName .. "."
                        ))

                        df.message(dialog,VSig("INDLG_InSetValue"),VString(dirName))

                        local handler = df.makeLuaMatchHandler(
                            VMatch(function()
                                print("Ok renamed!")
                                local outName = df.messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                                -- more thorough user input check should be performed
                                if (outName == "") then
                                    df.setStatus("Some directory name must be specified.")
                                    return
                                end

                                local wholeDir = whole(dirId)

                                local asyncSqlite = dg.currentAsyncSqlite
                                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                    return
                                end
                                local mainModel = df.namedMessageable("mainModel")
                                df.messageAsyncWCallback(
                                    asyncSqlite,
                                    function(output)
                                        local val = output:values()
                                        local affected = val._3
                                        if (affected > 0) then
                                            df.message(dg.mainWnd,
                                                VSig("MWI_InSetCurrentDirName"),
                                                VString(outName))
                                        else
                                            messageBox(
                                                "Duplicate name!",
                                                "'" .. outName ..
                                                "' already exists under current parent."
                                            )
                                        end
                                    end,
                                    VSig("ASQL_OutAffected"),
                                    VString(sqlUpdateDirectoryNameStatement(wholeDir,outName)),
                                    VInt(-1)
                                )
                                showOrHide(false)
                            end,"INDLG_OutOkClicked"),
                            VMatch(function()
                                print("Cancel rename!")
                                showOrHide(false)
                            end,"INDLG_OutCancelClicked")
                        )

                        df.message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
                        showOrHide(true)
                    end),
                    arrayBranch("New directory",function()
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

                        if (dirName == "[unselected]") then
                            df.setStatus("No directory was selected to create new one.")
                            return
                        end

                        df.message(dialog,VSig("INDLG_InSetLabel"),VString(
                            "Specify new folder name to create under " .. dirName .. "."
                        ))

                        local newId = dg.objRetainer:newId()

                        local handler = df.makeLuaMatchHandler(
                            VMatch(function()
                                print("Ok!")
                                local outName = df.messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                                -- more thorough user input check should be performed
                                if (outName == "") then
                                    setDlgErr("Some directory name must be specified.")
                                    return
                                end

                                if (not isValidFilename(outName)) then
                                    setDlgErr("Directory name entered contains invalid characters.")
                                    return
                                end

                                setDlgErr("")

                                local asyncSqlite = dg.currentAsyncSqlite
                                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                    return
                                end
                                local mainModel = df.namedMessageable("mainModel")

                                local theQuery = sqlNewDirectoryStatement(dirIdWhole,outName)

                                df.messageAsyncWCallback(
                                    asyncSqlite,
                                    function(res)
                                        local num = res:values()._3
                                        if (num > 0) then
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
                                        else
                                            messageBox(
                                                "Duplicate name!",
                                                "'" .. outName ..
                                                "' already exists under current directory."
                                            )
                                        end
                                    end,
                                    VSig("ASQL_OutAffected"),
                                    VString(theQuery),
                                    VInt(-1)
                                )
                                -- todo: optimize, don't reload all
                                showOrHide(false)
                                dg.objRetainer:release(newId)
                            end,"INDLG_OutOkClicked"),
                            VMatch(function()
                                print("Cancel!")
                                showOrHide(false)
                                dg.objRetainer:release(newId)
                            end,"INDLG_OutCancelClicked")
                        )

                        dg.objRetainer:retain(newId,handler)

                        df.message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
                        showOrHide(true)
                    end),
                    arrayBranch("New file",function()
                        print("New file clicked")
                        df.newFileDialog(
                            function(result,dialog)
                                local firstValidation =
                                    df.validateNewFileDialogFirst(result,dialog)
                                if (not firstValidation) then
                                    return false
                                end

                                -- great success, form validation passed
                                -- TODO: how to prolong file dialog?
                                df.addNewFileUnderCurrentDir(result,dialog)
                                return true
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

                                df.updateFileFromDiff(currentEntityId,dirId,result,orig,dialog)
                                return true
                            end
                        )
                    end),
                    arrayBranch("Move file",function()
                        df.setStatus("Select folder to move file to.")
                        dg.fileToMove = currentEntityId
                        dg.shouldMoveFile = true
                    end),
                    arrayBranch("Delete file",function()
                        local currentFileId = df.getCurrentEntityId()
                        -- we know that this is file because
                        -- we wouldn't see this menu
                        -- TODO: ask if really want to delete
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
                    end)
                )
            end
        )
end
