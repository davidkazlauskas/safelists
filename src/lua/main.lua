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
        end,"MWI_OutNewFileSignal")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))

end
initAll()
initAll = nil
