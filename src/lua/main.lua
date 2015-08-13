initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMesseagable("mainWindow")

    ctx:attachContextTo(mainWnd)

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
        VMatch(function()
            print("BALLIN, BALLIN")
        end,"MWI_OutNewFileSignal")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))

end
initAll()
initAll = nil
