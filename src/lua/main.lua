
require('lua/mobdebug').start()

setStatus = function(context,widget,text)
    context:message(widget,VSig("MWI_InSetStatusText"),VString(text))
end

revealDownloads = false
sessionWidget = nil

DownloadsModel = {
    sessions = {},
    progressTotal = 0,
    currentSession = 0
}

function DownloadsModel:newSession(o)
    o.currentSession = o.currentSession + 1
    local theSession = o.currentSession
    sessions[theSession] = {
        progress = 0,
        downloadTable = {}
    }
    return sessions[theSession]
end

initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMesseagable("mainWindow")

    ctx:attachContextTo(mainWnd)
    sessionWidget = ctx:messageRetValues(mainWnd,
        VSig("MWI_QueryDownloadSessionWidget"),VMsg(nil))._2

    currentDirId = -1

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
        VMatch(function()
            local mainModel = ctx:namedMesseagable("mainModel")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            ctx:message(mainModel,
                VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
            -- Testing, reveal
            revealDownloads = not revealDownloads
            ctx:message(mainWnd,
                VSig("MWI_InRevealDownloads"),VBool(revealDownloads))
        end,"MWI_OutNewFileSignal"),
        VMatch(function(natpack,val)
            local inId = val:values()._2

            if (currentDirId > 0) then
                ctx:message(mainWnd,VSig("MWI_InSetStatusText"),VString(""))
                if (inId == currentDirId) then
                    return
                end

                local mainWnd = ctx:namedMesseagable("mainWindow")
                local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
                local mainModel = ctx:namedMesseagable("mainModel")
                local outRes = ctx:messageRetValues(mainWnd,VSig("MWI_InMoveChildUnderParent"),VInt(-1))._2
                if (outRes == 1) then
                    setStatus(ctx,mainWnd,"Directory to move is parent of selected directory.")
                    return
                elseif (outRes == 2) then
                    setStatus(ctx,mainWnd,"Directories are the same.")
                    return
                elseif (outRes ~= 0) then
                    setStatus(ctx,mainWnd,"Something bad happened...")
                    return
                end

                ctx:messageAsync(asyncSqlite,
                    VSig("ASQL_Execute"),
                    VString("UPDATE directories SET dir_parent=" .. inId
                        .. " WHERE dir_id=" .. currentDirId .. ";"))
                    currentDirId = -1
                return
            end

            local mainModel = ctx:namedMesseagable("mainModel")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            ctx:message(mainModel,
                VSig("MMI_InLoadFileList"),VInt(inId),
                VMsg(asyncSqlite),VMsg(mainWnd))
        end,"MWI_OutDirChangedSignal","int"),
        VMatch(function()
            currentDirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2
            print("Selected dir: " .. currentDirId)
            if (currentDirId ~= -1) then
                ctx:message(mainWnd,
                    VSig("MWI_InSetStatusText"),
                    VString("Press on node under which to move"))
            end
        end,"MWI_OutMoveButtonClicked"),
        VMatch(function()
            currentDirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2
            if (currentDirId ~= -1) then
                if (currentDirId == 1) then
                    setStatus(ctx,mainWnd,"Root cannot be deleted.")
                end
                local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
                ctx:messageAsync(asyncSqlite,
                    VSig("ASQL_Execute"),
                    VString("DELETE FROM directories WHERE dir_id=" .. currentDirId .. ";"))
                ctx:message(mainWnd,VSig("MWI_InDeleteSelectedDir"))
                currentDirId = -1
            else
                setStatus(ctx,mainWnd,"No directory selected.")
            end
        end,"MWI_OutDeleteDirButtonClicked"),
        VMatch(function()
            local dialog = ctx:namedMesseagable("singleInputDialog")

            local showOrHide = function(val)
                ctx:message(dialog,VSig("INDLG_InShowDialog"),VBool(val))
            end

            local dirName = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
            local dirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2

            if (dirName == "[unselected]") then
                setStatus(ctx,mainWnd,"No directory was selected to create new one.")
                return
            end

            ctx:message(dialog,VSig("INDLG_InSetLabel"),VString(
                "Specify new folder name to create under " .. dirName .. "."
            ))

            local handler = ctx:makeLuaMatchHandler(
                VMatch(function()
                    print("Ok!")
                    local outName = ctx:messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                    -- more thorough user input check should be performed
                    if (outName == "") then
                        setStatus(ctx,mainWnd,"Some directory name must be specified.")
                        return
                    end

                    local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
                    local mainWnd = ctx:namedMesseagable("mainWindow")
                    local mainModel = ctx:namedMesseagable("mainModel")
                    ctx:messageAsync(
                        asyncSqlite,
                        VSig("ASQL_Execute"),
                        VString("INSERT INTO directories (dir_name,dir_parent)"
                            .. " VALUES ('" .. outName .. "'," .. dirId .. ");")
                    )
                    -- todo: optimize, don't reload all
                    ctx:message(mainModel,
                        VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
                    showOrHide(false)
                end,"INDLG_OutOkClicked"),
                VMatch(function()
                    print("Cancel!")
                    showOrHide(false)
                end,"INDLG_OutCancelClicked")
            )

            ctx:message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
            showOrHide(true)
        end,"MWI_OutNewDirButtonClicked"),
        VMatch(function()
            local dlFactory = ctx:namedMesseagable("dlSessionFactory")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natpack,val)
                    local valTree = val:values()
                    local newKey = valTree._2
                    print('Starting... ' .. valTree._2)
                    DownloadsModel.sessions[newKey] = newKey
                    ctx:message(sessionWidget,
                        VSig("DLMDL_InFullUpdate"))
                end,"SLD_OutStarted","int"),
                VMatch(function()
                    print('Downloaded!')
                end,"SLD_OutDone"),
                VMatch(function()
                    print('Safelist session dun! Downloading...')
                    local dlHandle = ctx:messageRetValues(dlFactory,
                        VSig("SLDF_InNewAsync"),
                        VString("downloadtest1/safelist_session"),
                        VMsg(currDlSessionHandler),
                        VMsg(nil)
                    )._4
                    assert( dlHandle ~= nil )
                end,"SLDF_OutCreateSessionDone")
            )

            currDlSessionHandler = handler
            ctx:message(dlFactory,
                VSig("SLDF_CreateSession"),
                VMsg(asyncSqlite),
                VMsg(handler),
                VString("downloadtest1/safelist_session")
            )
        end,"MWI_OutDownloadSafelistButtonClicked")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))


    downloadUpdateModel = ctx:makeLuaMatchHandler(
        VMatch(function(natPack)
            natPack:setSlot(2,VInt( #DownloadsModel.sessions ))
        end,"DLMDL_QueryCount","int")
    )
    ctx:message(mainWnd,VSig("MWI_InSetDownloadModel"),VMsg(downloadUpdateModel))

end
initAll()
initAll = nil
