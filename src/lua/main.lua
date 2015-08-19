
require('lua/mobdebug').start()

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

                local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
                local mainModel = ctx:namedMesseagable("mainModel")
                ctx:messageAsync(asyncSqlite,
                    VSig("ASQL_Execute"),
                    VString("UPDATE directories SET dir_parent=" .. inId
                        .. " WHERE dir_id=" .. currentDirId .. ";"))
                    local selected = currentDirId
                    currentDirId = -1
                    ctx:message(mainModel,
                        VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
                    ctx:message(mainWnd,
                        VSig("MWI_InSelectDirIdInTree"),VInt(selected))
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
        end,"MWI_OutMoveButtonClicked")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))

end
initAll()
initAll = nil
