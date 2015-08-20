
require('lua/mobdebug').start()

setStatus = function(context,widget,text)
    context:message(widget,VSig("MWI_InSetStatusText"),VString(text))
end

initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMesseagable("mainWindow")

    ctx:attachContextTo(mainWnd)

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
        VMatch(function()
            print("BALLIN, BALLIN")
            local mainModel = ctx:namedMesseagable("mainModel")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            ctx:message(mainModel,
                VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
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
            ctx:message(dialog,VSig("INDLG_InShowDialog"))
        end,"MWI_OutNewDirButtonClicked")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))

end
initAll()
initAll = nil
