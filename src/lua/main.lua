initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMesseagable("mainWindow")

    ctx:attachContextTo(mainWnd)

end
initAll()
initAll = nil
