
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
            local mainModel = ctx:namedMesseagable("mainModel")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            local inId = val:values()._2
            ctx:message(mainModel,
                VSig("MMI_InLoadFileList"),VInt(inId),
                VMsg(asyncSqlite),VMsg(mainWnd))
        end,"MWI_OutDirChangedSignal","int"),
        VMatch(function()
            ctx:message(mainWnd,
                VSig("MWI_InSetStatusText"),VString("Press on node under which to move"))
        end,"MWI_OutMoveButtonClicked")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))

end
initAll()
initAll = nil
