initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMesseagable("mainWindow")

    ctx:attachContextTo(mainWnd)

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
        VMatch(function()
            print("BALLIN, BALLIN")
            local mainModel = ctx:namedMesseagable("mainModel")
            ctx:message(mainModel,
                VSig("MMI_InLoadFolderTree"),VMsg("asyncSqliteCurrent"),VMsg(mainWnd))
        end,"MWI_OutNewFileSignal")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))

end
initAll()
initAll = nil
