
--require('lua/mobdebug').start()

package.path = package.path .. ";" .. LUA_SCRIPTS_PATH .. "/?.lua"

require('util')
require('sqlite')
require('safelist-constants')
require('genericwidget')
require('settings')
require('messages')
require('guiutil')
require('domaingui')
require('downloadsmodel')
require('utilobjects')
require('menumodel')


DomainGlobals = {
    currentAsyncSqlite = nil,
    shouldMoveDir = false,
    shouldMoveFile = false,
    ctx = nil,
    mainWnd = nil,
    oneOffFunctions = {},
    frameEndFunctions = {},
    sessionWidget = nil,
    currentSessions = {},
    currentSafelist = CurrentSafelist.__index.new(),
    downloadSpeedChecker = DownloadSpeedChecker.__index.new(7)
}

DomainFunctions = {
    updateRevision = nil
}

dg = DomainGlobals
df = DomainFunctions

dg.hrm = {
    hashRevisionUpdate = 0,
    hashRevisionDrawingUpdate = 0
}

dg.dm = DownloadsModel.newDownloadsModel()
dg.objRetainer = ObjectRetainer.new()

df.retainObject = function(obj)
    return dg.objRetainer:retainNewId(obj)
end
df.releaseObject = function(id)
    dg.objRetainer:release(id)
end
df.newObjectId = function()
    return dg.objRetainer:newId()
end
df.retainObjectWId = function(id,obj)
    dg.objRetainer:retain(id,obj)
end

df.updateSessionWidget = function()
    -- global state of session move, should we move this?
    local wgt = dg.sessionWidget
    if (nil == wgt) then
        return
    end

    if (dg.dm:isDirty()) then
        local ctx = luaContext()
        fullDownloadModelUpdate(ctx,wgt)
    end
end

initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMessageable("mainWindow")
    local genMainWnd = GenericWidget.putOn(mainWnd)
    local globConsts = ctx:namedMessageable("globalConsts")
    local writer = ctx:namedMessageable("randomFileWriter")

    dg.ctx = ctx
    dg.mainWnd = mainWnd

    df.ctx = luaContext
    df.mainWnd = function() return dg.mainWnd end
    df.namedMessageable = function(name)
        return ctx:namedMessageable(name)
    end

    df.message = function(...)
        ctx:message(table.unpack({...}))
    end

    df.messageRetValues = function(...)
        return ctx:messageRetValues(table.unpack({...}))
    end

    df.messageAsync = function(...)
        ctx:messageAsync(table.unpack({...}))
    end

    df.messageAsyncWCallback = function(...)
        ctx:messageAsyncWCallback(table.unpack({...}))
    end

    df.makeLuaMatchHandler = function(...)
        return ctx:makeLuaMatchHandler(table.unpack({...}))
    end

    df.quitApplication = function()
        df.message(
            mainWnd,
            VSig("MWI_InQuit")
        )
    end

    df.openUrlInBrowser = function(url)
        df.message(
            df.namedMessageable("dialogService"),
            VSig("GDS_OpenUrlInBrowser"),
            VString(url)
        )
    end

    df.globSetting =
        function(thename)
            return df.messageRetValues(
                globConsts,
                VSig("GLC_LookupString"),
                VString(thename),
                VString(""),
                VInt(-1)
            )._3
        end

    local settingsFileLocation = df.globSetting("settingspath")
    local examplesPath = df.globSetting("appdatapath") .. "/examples/"

    dg.persistentSettings = PersistentSettings.new(
        function(saveData)
            df.messageAsync(
                writer,
                VSig("RFW_WriteStringToFileWDir"),
                VString(settingsFileLocation),
                VString(saveData),
                VInt(-1)
            )
        end,
        function(functionForLoading)
            df.messageAsyncWCallback(
                writer,
                function(out)
                    local tbl = out:values()
                    if (tbl._4 == 0) then
                        functionForLoading(tbl._3)
                    end
                end,
                VSig("RFW_ReadStringFromFile"),
                VString(settingsFileLocation),
                VString(""),
                VInt(-1)
            )
        end
    )

    df.scheduleOneOffFunction = function(newFunc)
        table.insert(dg.oneOffFunctions,newFunc)
    end

    dg.mainWndButtonHandlers = {}

    df.loadCss = function(path)
        local cssMng = df.namedMessageable("themeManager")
        df.message(
            cssMng,
            VSig("THM_LoadTheme"),
            VString(path)
        )
    end

    dg.themes = {
        ["Adwaita (light)"] = "@/org/gtk/libgtk/theme/Adwaita.css",
        ["Adwaita (dark)"] = "@/org/gtk/libgtk/theme/Adwaita-dark.css",
        ["Raleigh"] = "@/org/gtk/libgtk/theme/Raleigh.css",
        ["Vertex (light)"] = "appdata/themes/vertex/gtk.css",
        ["Vertex (dark)"] = "appdata/themes/vertex/gtk-dark.css",
        ["Borderline GTK"] = "appdata/themes/borderline-gtk/gtk.css",
        ["Windows 10 (light)"] = "appdata/themes/windows-10/gtk.css",
        ["Windows 10 (dark)"] = "appdata/themes/windows-10/gtk-dark.css"
    }

    df.loadTheme = function(name)
        local path = dg.themes[name]
        assert( nil ~= path, "Theme doesn't exist." )
        df.loadCss(path)
        dg.persistentSettings:setValue("safelists.theme",name)
    end

    -- menu bar model attempt
    setupMenuModel(df,dg)

    df.safelistDependantWigets = function()
        return {
            "dirList",
            "downloadButton"
        }
    end

    df.resetVarsForSafelist = function()
        dg.currentDirToMoveId = -1
    end

    df.resetVarsForSafelist()

    df.setWidgetsEnabled = function(...)
        setWidgetsEnabled(ctx,table.unpack({...}))
    end

    df.noSafelistState = function()
        df.setWidgetsEnabled(
            mainWnd,
            false,
            table.unpack(df.safelistDependantWigets())
        )
    end

    df.onSafelistState = function()
        df.setWidgetsEnabled(
            mainWnd,
            true,
            table.unpack(df.safelistDependantWigets())
        )
    end

    df.updateRevision = function()
        dg.hrm.hashRevisionUpdate =
            dg.hrm.hashRevisionUpdate + 1
        df.updateSessionWidget()
    end

    df.addFrameEndFunction = function(another)
        table.insert(dg.frameEndFunctions,another)
    end

    df.updateRevisionGui = instrument(function()
        if (dg.hrm.hashRevisionUpdate ==
            dg.hrm.hashRevisionDrawingUpdate)
        then
            return
        end

        dg.hrm.hashRevisionDrawingUpdate =
            dg.hrm.hashRevisionUpdate

        local thisCorout = coroutine.running()

        local sess = dg.currentAsyncSqlite
        assert( nil ~= sess, "Sess is null for revision read..." )

        df.messageAsyncWCallback(sess,
            resumerCallbackValues(thisCorout),
            VSig("ASQL_OutSingleRow"),VString(sqlRevNumber()),
            VString("empty"),VBool(false))

        -- nap time
        local values = coroutine.yield()

        local succeeded = values._4
        local outString = values._3
        assert( succeeded, "Great success!" )
        local split = outString:split("|")
        local outRes = "Safelist revision: " .. split[1] ..
            ", last modification date: " .. split[2]

        df.message(mainWnd,VSig("MWI_InSetWidgetText"),
            VString("safelistRevisionLabel"),VString(outRes))

        end)

    df.setDownloadSpeedGui = function(string)
        df.message(
            mainWnd,
            VSig("MWI_InSetDownloadText"),
            VString(string)
        )
    end

    df.updateDownloadSpeed = function()
        dg.downloadSpeedChecker:regBytes(0)
        local theSpeed = dg.downloadSpeedChecker:bytesPerSec()

        local speedString = ""
        if (theSpeed > 0) then
            -- TODO: move to separate function
            if (theSpeed > 1024 * 1024) then
                local mbSec = theSpeed / (1024 * 1024)
                speedString = roundFloatStr(mbSec,2) .. " MB/s"
            elseif (theSpeed > 1024) then
                local kbSec = theSpeed / 1024
                speedString = roundFloatStr(kbSec,2) .. " KB/s"
            else
                speedString = theSpeed .. " B/s"
            end
        end

        if (speedString ~= "") then
            speedString = "Download speed: " .. speedString
        end

        df.setDownloadSpeedGui(speedString)
    end

    df.newAsqlite = function(path)
        local factory = df.namedMessageable("asyncSqliteFactory")
        local shutdownGuard = df.namedMessageable("shutdownGuard")

        local result = df.messageRetValues(factory,
            VSig("ASQLF_CreateNew"),
            VString(path),VMsg(nil))._3

        df.message(shutdownGuard,
            VSig("GSI_AddNew"),
            VMsg(result))

        return result
    end

    df.newSafelist = function(path)
        local res = df.newAsqlite(path)
        df.messageAsync(
            res,
            VSig("ASQL_Execute"),
            VString(newSafelistSchema())
        )
        return res
    end

    df.messageBoxWParent = function(title,message,parent)
        local dialogService =
            df.namedMessageable("dialogService")

        local dialog =
            df.messageRetValues(
                dialogService,
                VSig("GDS_MakeGenericWidget"),
                VString("dialogs"),
                VString("okDialogWindow"),
                VMsg(nil)
            )._4

        local wrapped = GenericWidget.putOn(dialog)
        local window = wrapped:getWidget("okDialogWindow")
        local titleWgt = wrapped:getWidget("okDialogMessage")
        local buttonWgt = wrapped:getWidget("okDialogButton")

        titleWgt:labelSetText(message)
        buttonWgt:buttonSetText("OK")
        buttonWgt:hookButtonClick(function()
            window:setVisible(false)
        end)
        window:windowSetPosition("WIN_POS_CENTER")
        window:windowSetTitle(title)
        window:windowSetParent(parent)
        window:setVisible(true)
    end

    df.dialogService = function()
        return df.namedMessageable("dialogService")
    end

    df.messageBox = function(title,message)
        local dialogService = df.dialogService()

        local mainWrapped = GenericWidget.putOn(mainWnd)
        local mainAppWnd = mainWrapped:getWidget("mainAppWindow")

        df.messageBoxWParent(title,message,mainAppWnd:getMessageable())
    end

    df.validateNewFileDialogFirst = function(result,dialog)
        assert( result.finished, "Should be good..." )

        if (result.name ~= nil and
            not isValidFilename(result.name)
        ) then
            df.messageBoxWParent(
                "Invalid input",
                "Filename contains invalid characters.",
                dialog
            )
            return false
        end

        local uniqueMirrMap = {}
        if (result.mirrors ~= nil) then
            -- todo validate mirrors
            local mirrors = string.split(result.mirrors,"\n")
            local mirrTrimmed = {}
            for k,v in ipairs(mirrors) do
                local trimmed = trimString(v)

                if (uniqueMirrMap[trimmed] == 1) then
                    df.messageBoxWParent(
                        "Invalid input",
                        "Mirror field contains duplicate mirror '"
                        .. trimmed .. "'.",
                        dialog
                    )
                    return false
                end
                uniqueMirrMap[trimmed] = 1
                -- todo: add checking for valid
                -- url (don't know what valid url is yet)
                if (trimmed ~= "") then
                    table.insert(mirrTrimmed,trimmed)
                end
            end

            if (#mirrTrimmed == 0) then
                df.messageBoxWParent(
                    "Invalid input",
                    "No valid mirrors found.",
                    dialog
                )
                return false
            end

            result.mirrorsTable = mirrTrimmed
        end

        if (result.hash ~= nil and result.hash ~= "" and
            not isValidDumbHash256(result.hash))
        then
            df.messageBoxWParent(
                "Invalid input",
                "DumbHash256 entered is invalid." ..
                " Expected 64 hexadecimal digits.",
                dialog
            )
            return false
        end

        if (result.size ~= nil and result.size ~= "" and
            not string.match(result.size,"^%d+$"))
        then
            df.messageBoxWParent(
                "Invalid input",
                "Size is invalid. Expected number in bytes.",
                dialog
            )
            return false
        end

        if (result.size ~= nil and result.size == "") then
            result.size = "-1"
        end

        return true
    end

    df.hookButton = function(dialog,widget)
        return df.messageRetValues(
            dialog,
            VSig("INDLG_HookButtonClick"),
            VString(widget),
            VInt(-1)
        )._3
    end

    df.makeGenericDialog = function(schema,name)
        return df.messageRetValues(
            df.namedMessageable("dialogService"),
            VSig("GDS_MakeGenericDialog"),
            VString(schema),
            VString(name),
            VMsg(nil)
        )._4
    end

    df.hideDialog = function(dialog)
        df.message(
            dialog,
            VSig("INDLG_InHideDialog")
        )
    end

    df.newFileDialog = instrument(function(funcSuccess)
        local thisCorout = coroutine.running()
        local dialogService = df.namedMessageable("dialogService")

        local dialog = df.makeGenericDialog("main","newFileDialog")
        local wrapped = GenericWidgetNode.putOn(dialog)
        wrapped:windowSetTitle("New file")

        local hookButton = function(widget)
            return df.hookButton(dialog,widget)
        end

        local hookedOk = hookButton("okButton")
        local hookedCancel = hookButton("cancelButton")

        local hideDlg = function()
            df.hideDialog(dialog)
        end

        local handler = df.makeLuaMatchHandler(
            VMatch(resumerCallbackWBranch("answer", thisCorout),
                "INDLG_OutGenSignalEmitted","int"),
            VMatch(resumerCallbackWBranch("exited", thisCorout),
                "INDLG_OutDialogExited")
        )

        local newId = df.retainObject(handler)

        df.message(
            dialog,
            VSig("INDLG_InSetNotifier"),
            VMsg(handler)
        )
        df.message(
            dialog,
            VSig("INDLG_InAlwaysAbove")
        )
        df.message(
            dialog,
            VSig("INDLG_InShowDialog")
        )

        while true do
            -- nap: wait for response of the dialog
            local outBranch, _, val = coroutine.yield()

            if (outBranch == "answer") then
                local signal = val:values()._2
                if (signal == hookedOk) then
                    print("ok clicked")

                    local queryInput = function(value)
                        return df.messageRetValues(
                            dialog,
                            VSig("INDLG_QueryInput"),
                            VString(value),
                            VString(""))._3
                    end

                    -- Return results:
                    -- finished - did finish?
                    -- name - the name
                    -- mirrors - the mirror string
                    -- size - file size (no by default)
                    -- hash - hash (no by default)
                    local outResult = {}

                    outResult.finished = true
                    outResult.name = queryInput("fileNameInp")
                    outResult.mirrors = queryInput("mirrorsTextView")
                    outResult.size = queryInput("fileSizeInp")
                    outResult.hash = queryInput("fileHashInp")

                    if funcSuccess(outResult,dialog) then
                        df.releaseObject(newId)
                        return
                    end
                elseif (signal == hookedCancel) then
                    print("Cancel clicked")
                    hideDlg()
                    df.releaseObject(newId)
                    return
                else
                    assert( false, "No such signal? " .. signal )
                    return
                end
            elseif (outBranch == "exited") then
                print("Exit vanilla")
                df.releaseObject(newId)
                return
            else
                assert(false, "lolwut?")
                return
            end
        end
    end)

    df.modifyFileDialog = instrument(function(fileId,funcSuccess)
        local fileIdWhole = whole(fileId)
        local dialogService = df.namedMessageable("dialogService")
        local thisCorout = coroutine.running()

        -- Return results:
        -- finished - did finish?
        -- name - the name
        -- mirrors - the mirror string
        -- size - file size (no by default)
        -- hash - hash (no by default)
        local original = {
            finished = false
        }

        local dialog = df.makeGenericDialog("main","newFileDialog")
        local wrapped = GenericWidgetNode.putOn(dialog)
        wrapped:windowSetTitle("Edit file")

        local hookButton = function(widget)
            return df.messageRetValues(
                dialog,
                VSig("INDLG_HookButtonClick"),
                VString(widget),
                VInt(-1)
            )._3
        end

        local hookedOk = hookButton("okButton")
        local hookedCancel = hookButton("cancelButton")

        local hideDlg = function()
            df.hideDialog(dialog)
        end

        -- TODO: sort to make sure?
        local trimMirrors = function(text)
            local newTable = {}
            local split = string.split(text,"\n")
            for k,v in ipairs(split) do
                local trimmed = trimString(v)
                if (trimmed ~= "") then
                    table.insert(newTable,trimmed)
                end
            end
            return table.concat(newTable,"\n")
        end

        local handler = df.makeLuaMatchHandler(
            VMatch(resumerCallbackWBranch("answer", thisCorout),
                "INDLG_OutGenSignalEmitted","int"),
            VMatch(resumerCallbackWBranch("exited", thisCorout),
                "INDLG_OutDialogExited")
        )
        local newId = df.retainObject(handler)

        df.message(
            dialog,
            VSig("INDLG_InSetNotifier"),
            VMsg(handler)
        )
        df.message(
            dialog,
            VSig("INDLG_InAlwaysAbove")
        )

        -- lookup actual data
        local query = sqlGetMirrorUsesForFile(fileIdWhole)
        local asyncSqlite = dg.currentAsyncSqlite

        df.messageAsyncWCallback(
            asyncSqlite,
            resumerCallbackValues(thisCorout),
            VSig("ASQL_OutSingleRow"),
            VString(query),
            VString(""),
            VBool(false)
        )

        -- nap: file modified dialog is finished
        local tbl = coroutine.yield()

        local isOk = tbl._4
        assert( isOk, "Your query failed, friendo" )
        local outputRow = tbl._3

        local splitRow = string.split(outputRow,"|")

        local fileName = splitRow[1]
        local fileSize = splitRow[2]
        local fileHash = splitRow[3]
        local totalUses = tonumber(splitRow[4])
        local splitMirrors = string.split(splitRow[5],",")

        if (fileSize == "-1") then
            fileSize = ""
        end

        local setInput = function(name,value)
            df.message(
                dialog,
                VSig("INDLG_InSetValue"),
                VString(name),
                VString(value)
            )
        end

        local concatMirrors = table.concat(splitMirrors,"\n")

        original.name = fileName
        original.size = fileSize
        original.hash = fileHash
        original.mirrors = concatMirrors

        setInput("fileNameInp",fileName)
        setInput("mirrorsTextView",concatMirrors)
        setInput("fileSizeInp",fileSize)
        setInput("fileHashInp",fileHash)

        local hashAndSizeOff = totalUses > 0
        if (hashAndSizeOff) then
            local offInput = function(name)
                df.message(dialog,
                    VSig("INDLG_InSetControlEnabled"),
                    VString(name),
                    VBool(false))
            end

            offInput("fileSizeInp")
            offInput("fileHashInp")
        end

        df.message(
            dialog,
            VSig("INDLG_InShowDialog")
        )

        while true do
            -- nap: wait for response of the dialog
            local outBranch, _, val = coroutine.yield()

            if (outBranch == "answer") then
                local signal = val:values()._2
                if (signal == hookedOk) then

                    local queryInput = function(value)
                        return df.messageRetValues(
                            dialog,
                            VSig("INDLG_QueryInput"),
                            VString(value),
                            VString(""))._3
                    end

                    local outResult = {
                        finished = false
                    }

                    local diffAssign = function(field,prev,inpField)
                        local current = queryInput(inpField)
                        if (prev ~= current) then
                            outResult[field] = current
                        end
                    end

                    local mirrTrimmed = trimMirrors(queryInput("mirrorsTextView"))

                    outResult.finished = true
                    diffAssign("name",original.name,"fileNameInp")
                    diffAssign("size",original.size,"fileSizeInp")
                    diffAssign("hash",original.hash,"fileHashInp")

                    if (mirrTrimmed ~= original.mirrors) then
                        outResult.mirrors = mirrTrimmed
                    end

                    -- TODO: major refactor this mess
                    if funcSuccess(outResult,original,dialog) then
                        print("success, returned")
                        df.releaseObject(newId)
                        return
                    else
                        print("Fail, repeating")
                    end
                elseif (signal == hookedCancel) then
                    print("Cancel clicked")
                    hideDlg()
                    df.releaseObject(newId)
                    print("fail, returned")
                    return
                else
                    assert( false, "No such signal? " .. signal )
                    return
                end
            elseif (outBranch == "exited") then
                print("Exit vanilla")
                df.releaseObject(newId)
                return
            else
                assert(false, "lolwut?")
                return
            end
        end
    end)


    df.getCurrentEntityId = function()
        local mret = df.messageRetValues(mainWnd,
                        VSig("MWI_QueryCurrentEntityId"),VInt(-7),VBool(false))
        return mret._2, mret._3
    end

    df.getCurrentFileParent = function()
        return df.messageRetValues(mainWnd,
            VSig("MWI_QueryCurrentFileParent"),VInt(-7))._2
    end
    df.setStatus = function(statText)
        setStatus(dg.ctx,dg.mainWnd,statText)
    end
    df.deleteSelectedDir = function()
        df.message(dg.mainWnd,VSig("MWI_InDeleteSelectedDir"))
    end
    df.execSqliteOnHandler = function(sqliteHandler,theStatement)
        df.messageAsync(sqliteHandler,
            VSig("ASQL_Execute"),
            VString(theStatement))
    end

    df.addNewFileUnderCurrentDir = instrument(function(data,dialog,retCallback)
        local thisCorout = coroutine.running()
        local currentEntityId = df.getCurrentEntityId()
        local currentDirIdWhole = whole(currentEntityId)

        local asyncSqlite = dg.currentAsyncSqlite
        assert(not messageablesEqual(VMsgNil(),asyncSqlite),
            "No async sqlite." )

        -- disable Ok button for split
        -- second of async operations
        df.message(
            dialog,
            VSig("INDLG_InSetControlEnabled"),
            VString("okButton"),
            VBool(false)
        )

        local inputFail = function(message)
            df.messageBoxWParent(
                "Invalid input",
                message,
                dialog
            )
            df.message(
                dialog,
                VSig("INDLG_InSetControlEnabled"),
                VString("okButton"),
                VBool(true)
            )
        end

        local retCbCheck = function(ans)
            if (retCallback ~= nil) then
                retCallback(ans)
            end
        end

        -- !! check first
        local condition = sqlCheckForForbiddenFileNamesUpdate(currentDirIdWhole,-1,data.name)

        df.messageAsyncWCallback(
            asyncSqlite,
            resumerCallbackValues(thisCorout),
            VSig("ASQL_OutSingleNum"),
            VString(condition),
            VInt(-1),
            VBool(false)
        )

        local val = coroutine.yield()
        local success = val._4
        assert( success, "YOU GET NOTHING, YOU LOSE" )
        local case = val._3
        --[[
        case 1 - file with such name exists in current dir
        case 2 - file with forbidden tail name exists in current dir
        case 3 - directory with such name exists in current dir
        case 4 - directory with forbidden tail name exists in current dir
        --]]
        if (case == 1) then
            inputFail( "File '" .. data.name .. "' already"
                .. " exists under current directory.")
            retCbCheck(false)
            return false
        elseif (case == 2) then
            inputFail("Name '" .. data.name .. "' is forbidden.")
            retCbCheck(false)
            return false
        elseif (case == 3) then
            inputFail( "Directory with name '" .. data.name .. "' already"
                .. " exists under current directory.")
            retCbCheck(false)
            return false
        elseif (case == 4) then
            inputFail("Name '" .. data.name .. "' is forbidden.")
            retCbCheck(false)
            return false
        end

        if (case ~= 0) then
            assert( false, "Say what cholo?" )
            retCbCheck(false)
            return false
        end

        -- db validation succeeded
        local statement =
            sqlAddNewFileQuery(currentDirIdWhole,data.name,
                data.size,data.hash,data.mirrors)

        df.messageAsyncWCallback(
            asyncSqlite,
            -- you know, I'd love a feature
            -- in sqlite to query something
            -- right after insert, that'd be great.
            resumerCallbackValues(thisCorout),
            VSig("ASQL_Execute"),
            VString(statement)
        )

        -- nap: just wait for execution, no values returned
        coroutine.yield()

        local lastFileId = sqlFileIdSelect(currentDirIdWhole,data.name) .. ";"
        df.messageAsyncWCallback(
            asyncSqlite,
            resumerCallbackValues(thisCorout),
            VSig("ASQL_OutSingleNum"),
            VString(lastFileId),
            VInt(-1),
            VBool(false)
        )

        -- nap: end, we got file id, not update it
        local tbl = coroutine.yield()
        assert( tbl._4, "Aww, zigga nease..." )
        local theId = tbl._3
        df.message(
            mainWnd,
            VSig("MWI_InAddNewFileInCurrent"),
            VInt(theId),
            VInt(currentEntityId),
            VString(data.name),
            VDouble(tonumber(data.size)),
            VString(data.hash)
        )
        df.hideDialog(dialog)
        df.updateRevision()

        -- TODO: how to reflect db failures to dialog?
        retCbCheck(true)
        return true
    end)

    df.updateFileFromDiff = function(fileId,currentDirId,diffTable,orig,dialog,retCallback)
        -- diffTable:
        -- finished - did finish?
        -- name - the name
        -- mirrors - the mirror string
        -- size - file size (no by default)
        -- hash - hash (no by default)
        local fileIdWhole = whole(fileId)
        local currentDirIdWhole = whole(currentDirId)

        local retCbCheck = function(ans)
            if (retCallback ~= nil) then
                retCallback(ans)
            end
        end

        local missing = function(...)
            local props = {...}
            for k,v in ipairs(props) do
                if (diffTable[v] ~= nil) then
                    return false
                end
            end
            return true
        end

        local hideDlg = function()
            df.hideDialog(dialog)
        end

        if (missing("name","mirrors","size","hash")) then
            -- nothing changed, no worries
            hideDlg()
            retCbCheck(true)
            return true
        end

        local asyncSqlite = dg.currentAsyncSqlite

        local updateFunction = function()
            local outString = sqlUpdateFileQuery(fileIdWhole,diffTable.name,
                diffTable.size,diffTable.hash,diffTable.mirrors)

            df.messageAsync(
                asyncSqlite,
                VSig("ASQL_Execute"),
                VString(outString)
            )

            local merged = mergeTables(orig,diffTable)
            local toUp = -1
            if (merged.size ~= "") then
                toUp = tonumber(merged.size)
            end

            df.message(
                mainWnd,
                VSig("MWI_InSetCurrentFileValues"),
                VInt(fileId),
                VInt(currentDirId),
                VString(merged.name),
                VDouble(toUp),
                VString(merged.hash)
            )
            hideDlg()
            df.updateRevision()
        end

        local inputFail = function(message)
            df.messageBoxWParent(
                "Invalid input",
                message,
                dialog
            )
            df.message(
                dialog,
                VSig("INDLG_InSetControlEnabled"),
                VString("okButton"),
                VBool(true)
            )
        end

        if (nil ~= diffTable.name) then
            -- validate if file name is forbidden
            df.message(
                dialog,
                VSig("INDLG_InSetControlEnabled"),
                VString("okButton"),
                VBool(false)
            )

            local currentName = diffTable.name
            local validationQuery =
                sqlCheckForForbiddenFileNamesUpdate(
                    currentDirIdWhole,fileIdWhole,currentName)
            df.messageAsyncWCallback(
                asyncSqlite,
                function(out)
                    local tbl = out:values()
                    local isGood = tbl._4
                    local case = tbl._3

                    assert( isGood, "Take sqlite 101, sucker." )
                    if (case == 0) then
                        updateFunction()
                        retCbCheck(true)
                        return
                    end

                    if (case == 1) then
                        inputFail( "File '" .. currentName .. "' already"
                            .. " exists under current directory.")
                        retCbCheck(false)
                        return
                    elseif (case == 2) then
                        inputFail("Name '" .. currentName .. "' is forbidden.")
                        retCbCheck(false)
                        return
                    elseif (case == 3) then
                        inputFail("Directory with name '" .. currentName
                            .. "' already exists under current directory.")
                        retCbCheck(false)
                        return
                    elseif (case == 4) then
                        inputFail("Name '" .. currentName .. "' is forbidden.")
                        retCbCheck(false)
                        return
                    end

                    assert( false, "You stepped in the wrong neighbourhood, bro..." )
                end,
                VSig("ASQL_OutSingleNum"),
                VString(validationQuery),
                VInt(-1),
                VBool(false)
            )
        else
            updateFunction()
        end

    end

    df.addFrameEndFunction(df.updateRevisionGui)
    df.addFrameEndFunction(df.updateSessionWidget)
    df.addFrameEndFunction(df.updateDownloadSpeed)

    df.addFrameEndFunction(function()
        dg.persistentSettings:persist()
    end)

    df.noSafelistState()

    ctx:attachContextTo(mainWnd)
    dg.sessionWidget = df.messageRetValues(mainWnd,
        VSig("MWI_QueryDownloadSessionWidget"),VMsg(nil))._2

    df.resetVarsForSafelist()

    df.appendSessionLog = function(dlModel,theStr)
        dlModel:appendLog(theStr)
        dlModel.loggedErrors = true
    end

    df.okCancelDialog = function(title,text,closure)
        -- closure answers: ok, cancel
        local nId = df.newObjectId()

        local handler = df.makeLuaMatchHandler(
            VMatch(function(natPack,val)
                local outAns = val:values()._2
                if (outAns == 0) then
                    closure("ok")
                elseif (outAns == -1) then
                    -- cancel or exit
                    closure("cancel")
                else
                    closure("")
                end
                df.releaseObject(nId)
            end,"GDS_OutNotifyAnswer","int")
        )

        df.retainObjectWId(nId,handler)

        df.message(
            df.dialogService(),
            VSig("GDS_OkCancelDialog"),
            VMsg(df.mainWnd()),
            VString(title),
            VString(text),
            VMsg(handler)
        )
    end

    mainWindowPushButtonHandler = df.makeLuaMatchHandler(
        VMatch(function()
            local oneOffSteal = dg.oneOffFunctions
            dg.oneOffFunctions = {}
            for k,v in ipairs(oneOffSteal) do
                v()
            end

            for k,v in ipairs(dg.frameEndFunctions) do
                v()
            end
            --print('Draw ended!')
        end,"MWI_OutDrawEnd"),
        VMatch(function(nat,val)
            local index = val:values()._2
            dg.mainWndButtonHandlers[index]()
        end,"GWI_GBT_OutClickEvent","int"),
        VMatch(function()
            local mainModel = df.namedMessageable("mainModel")
            local asyncSqlite = dg.currentAsyncSqlite
            if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                return
            end
            df.message(mainModel,
                VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
        end,"MWI_OutNewFileSignal"),
        VMatch(instrument(function(natpack,val)
            local thisCorout = coroutine.running()
            local inId = val:values()._2

            dg.currentDirId = inId
            local currentDirIdWhole = whole(dg.currentDirId)

            if (dg.currentDirId > 0 and dg.shouldMoveFile == true) then
                dg.shouldMoveFile = false

                local _, isDir = df.getCurrentEntityId()
                if (not isDir) then
                    df.messageBox(
                        "Cannot move!",
                        "Cannot move under a file."
                    )
                    return
                end

                setStatus(ctx,mainWnd,"")
                local toMove = dg.fileToMove
                local toMoveWhole = whole(toMove)
                local asyncSqlite = dg.currentAsyncSqlite
                assert( not messageablesEqual(VMsgNil(),asyncSqlite),
                    "Huh cholo?" )

                local condition = sqlMoveFileValidation(toMoveWhole,currentDirIdWhole)

                df.messageAsyncWCallback(
                    asyncSqlite,
                    resumerCallbackValues(thisCorout),
                    VSig("ASQL_OutSingleRow"),
                    VString(condition),
                    VString(""),
                    VBool(false)
                )

                local val = coroutine.yield()
                local success = val._4
                assert( success, "Back to sqlite school sucker." )
                local outRow = val._3
                local split = string.split(outRow,"|")
                local case = tonumber(split[1])
                local fileName = split[2]
                local fileSize = tonumber(split[3])
                local hash = split[4]
                if (case == 1) then
                    df.messageBoxWParent(
                        "Invalid move",
                        "File with such name already"
                        .. " exists under that directory.",
                        mainWnd
                    )
                elseif (case == 2) then
                    df.messageBoxWParent(
                        "Invalid move",
                        "File cannot be moved under"
                        .. " this directory.",
                        mainWnd
                    )
                elseif (case == 0) then
                    local dupeNameQuery =
                        sqlCheckForForbiddenFileNamesUpdate(currentDirIdWhole,-1,fileName)

                    df.messageAsyncWCallback(
                        asyncSqlite,
                        resumerCallbackValues(thisCorout),
                        VSig("ASQL_OutSingleNum"),
                        VString(dupeNameQuery),
                        VInt(-1),
                        VBool(false)
                    )

                    local nameValRes = coroutine.yield()
                    assert( nameValRes._4, "Back to sqlite school, sucker: |" ..  dupeNameQuery .. "|" )

                    local caseDupe = nameValRes._3
                    if (caseDupe == 1) then
                        df.messageBox(
                            "Cannot move!",
                            "File with such name already exists under this directory."
                        )
                        return
                    elseif (caseDupe == 2) then
                        df.messageBox(
                            "Cannot move!",
                            "Name is forbidden under that directory."
                        )
                        return
                    elseif (caseDupe == 3) then
                        df.messageBox(
                            "Cannot move!",
                            "Directory with such name already exists under this directory."
                        )
                        return
                    elseif (caseDupe == 4) then
                        df.messageBox(
                            "Cannot move!",
                            "Name is forbidden under that directory."
                        )
                        return
                    end

                    local updateQuery = sqlMoveFileStatement(toMoveWhole,currentDirIdWhole)

                    df.messageAsync(
                        asyncSqlite,
                        VSig("ASQL_Execute"),
                        VString(updateQuery)
                    )
                    df.message(
                        mainWnd,
                        VSig("MWI_InAddNewFileInCurrent"),
                        VInt(toMove),
                        VInt(dg.currentDirId),
                        VString(fileName),
                        VDouble(fileSize),
                        VString(hash)
                    )
                    df.message(
                        mainWnd,
                        VSig("MWI_InDeleteSelectedDir"),
                        VInt(1)
                    )
                    df.updateRevision()
                else
                    assert( false, "Huh?!?" )
                end
                return
            end

            if (dg.currentDirToMoveId > 0 and dg.shouldMoveDir == true) then
                dg.shouldMoveDir = false

                df.setStatus("")
                if (inId == dg.currentDirToMoveId) then
                    return
                end

                local _, isDir = df.getCurrentEntityId()
                if (not isDir) then
                    df.messageBox(
                        "Cannot move!",
                        "Cannot move under a file."
                    )
                    return
                end

                local inIdWhole = whole(inId)
                local currentDirToMoveIdWhole = whole(dg.currentDirToMoveId)

                local asyncSqlite = dg.currentAsyncSqlite
                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                    return
                end
                local mainWnd = df.namedMessageable("mainWindow")
                local mainModel = df.namedMessageable("mainModel")

                local condition = sqlMoveDirCondition(inIdWhole, currentDirToMoveIdWhole)

                df.messageAsyncWCallback(
                    asyncSqlite,
                    resumerCallbackValues(thisCorout),
                    VSig("ASQL_OutSingleNum"),
                    VString(condition),
                    VInt(-1),
                    VBool(false))

                local table = coroutine.yield()
                local value = table._3
                local success = table._4
                --assert( success, "Great success failed..." )
                if (value == 0) then
                    local dupeNameQuery =
                        sqlCheckForForbiddenFileNamesUpdateNoQuotes(
                            inIdWhole,-1,"(" .. sqlSelectDirNameById(currentDirToMoveIdWhole) .. ")")

                    df.messageAsyncWCallback(
                        asyncSqlite,
                        resumerCallbackValues(thisCorout),
                        VSig("ASQL_OutSingleNum"),
                        VString(dupeNameQuery),
                        VInt(-1),
                        VBool(false)
                    )

                    local nameValRes = coroutine.yield()
                    assert( nameValRes._4, "Back to sqlite school, sucker: |" ..  dupeNameQuery .. "|" )

                    local caseDupe = nameValRes._3
                    if (caseDupe == 1) then
                        df.messageBox(
                            "Cannot move!",
                            "File with such name already exists under this directory."
                        )
                        return
                    elseif (caseDupe == 2) then
                        df.messageBox(
                            "Cannot move!",
                            "Name is forbidden under that directory."
                        )
                        return
                    elseif (caseDupe == 3) then
                        df.messageBox(
                            "Cannot move!",
                            "Directory with such name already exists under this directory."
                        )
                        return
                    elseif (caseDupe == 4) then
                        df.messageBox(
                            "Cannot move!",
                            "Name is forbidden under that directory."
                        )
                        return
                    end

                    df.messageAsync(asyncSqlite,
                        VSig("ASQL_OutAffected"),
                        VString(sqlMoveDirStatement(currentDirToMoveIdWhole,inIdWhole)),
                        VInt(-1))
                    dg.currentDirToMoveId = -1
                    df.message(mainWnd,
                        VSig("MWI_InMoveChildUnderParent"),
                        VInt(-1))
                    df.updateRevision()
                elseif (value == 1) then
                    df.messageBox(
                        "Cannot move!",
                        "Directory to move cannot be a parent"
                        .. " of directory to move under."
                    )
                elseif (value == 2) then
                    df.messageBoxWParent(
                        "Cannot move!",
                        "Parent directory already"
                        .. " has directory with such name.",
                        mainWnd)
                elseif (value == 3) then
                    df.messageBox(
                        "Cannot move!",
                        "Directory is already under"
                        .. " this parent."
                    )
                else
                    assert( false, "Should not happen cholo..." )
                end
                return
            end
        end),"MWI_OutDirChangedSignal","int"),
        VMatch(function()
            downloadSessionHandler(df,dg,sqlSelectAllFilesForSession())
        end,"MWI_OutDownloadSafelistButtonClicked"),
        VMatch(function()
            local dialogService = df.namedMessageable("dialogService")

            local afterPath = function(outPath)
                if (outPath ~= "") then
                    if (dg.currentSafelist:isSamePath(outPath)) then
                        df.messageBox(
                            "Already opened!",
                            "'" .. outPath ..
                            "' safelist is already opened."
                        )
                        return
                    end
                    dg.currentSafelist:setPath(outPath)
                    if (not dg.currentSafelist:isEmpty()) then
                        df.noSafelistState() -- prevent user from doing
                                          -- anything for split second
                    end

                    local openNew = function()
                        local mainModel = df.namedMessageable("mainModel")

                        dg.currentAsyncSqlite = df.newAsqlite(outPath)

                        df.message(mainModel,
                            VSig("MMI_InLoadFolderTree"),
                            VMsg(dg.currentAsyncSqlite),VMsg(mainWnd))
                        df.onSafelistState()
                        df.updateRevision()
                    end

                    local asql = dg.currentAsyncSqlite
                    if (nil ~= asql) then
                        df.messageAsyncWCallback(
                            dg.currentAsyncSqlite,
                            function()
                                openNew()
                            end,
                            VSig("ASQL_Shutdown"))
                    else
                        openNew()
                    end
                end
            end

            local nId = df.newObjectId()

            local handler = df.makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local outPath = val:values()._2
                    local folder = string.match(outPath,".+/")
                    if (folder ~= nil) then
                        dg.persistentSettings:setValue(
                            "safelists.lastopen",folder)
                    end
                    afterPath(outPath)
                    df.releaseObject(nId)
                end,"GDS_OutNotifyPath","string")
            )

            df.retainObjectWId(nId,handler)

            df.message(dialogService,
                VSig("GDS_FileChooserDialog"),
                VMsg(mainWnd),
                VString("Select safelist to open."),
                VString("*.safelist"),
                VString(dg.persistentSettings:getValueDefault(
                    "safelists.lastopen",examplesPath)),
                VMsg(handler))
        end,"MWI_OutOpenSafelistButtonClicked"),
        VMatch(instrument(function()
            local thisCorout = coroutine.running()
            local dialogService = df.namedMessageable("dialogService")

            local nId = df.newObjectId()

            local handler = df.makeLuaMatchHandler(
                VMatch(resumerCallbackWBranch("success",thisCorout),
                    "GDS_OutNotifyPath","string")
            )

            df.retainObjectWId(nId,handler)

            df.message(dialogService,
                VSig("GDS_FileSaverDialog"),
                VMsg(mainWnd),
                VString("Select new safelist path"),
                VMsg(handler)
            )

            local outBranch, _, val = coroutine.yield()

            if outBranch ~= "success" then
                return
            end

            local outPath = val:values()._2

            if (outPath == "") then
                return
            end

            if (not string.ends(string.lower(outPath),".safelist")) then
                outPath = outPath .. ".safelist"
            end

            local ifContinue = function()
                local openNew = function()
                    local mainModel = df.namedMessageable("mainModel")

                    dg.currentAsyncSqlite = df.newSafelist(outPath)
                    local new = dg.currentAsyncSqlite
                    df.resetVarsForSafelist()
                    df.message(mainModel,
                        VSig("MMI_InLoadFolderTree"),
                        VMsg(new),VMsg(mainWnd))
                    df.updateRevision()
                    df.onSafelistState()
                end

                local prev = dg.currentAsyncSqlite
                if (nil ~= prev) then
                    df.messageAsyncWCallback(
                        prev,
                        openNew,
                        VSig("ASQL_Shutdown"))
                    return
                end

                openNew()
            end

            df.noSafelistState()

            df.messageAsyncWCallback(
                writer,
                resumerCallbackValues(thisCorout),
                VSig("RFW_DoesFileExist"),
                VString(outPath),
                VBool(false)
            )

            local tbl = coroutine.yield()
            local exists = tbl._3
            if (not exists) then
                ifContinue()
            else
                local afterAnswer = function(response)

                    if (response == "ok") then
                        df.messageAsyncWCallback(
                            writer,
                            ifContinue,
                            VSig("RFW_DeleteFile"),
                            VString(outPath)
                        )
                    elseif (response == "cancel" or response == "exit") then
                        df.noSafelistState()
                    else
                        assert( false, "Wrong neighbourhood, milky." )
                        df.noSafelistState()
                    end

                end

                df.okCancelDialog(
                    "Safelist exists",
                    "Safelist already exists. Overwrite it? (data will be lost)",
                    afterAnswer)
            end
        end),"MWI_OutCreateSafelistButtonClicked"),
        VMatch(function()

            local dialogService = df.namedMessageable("dialogService")

            local afterPath = function(thePath)

                if (dg.currentSessions[thePath] == "t") then
                    df.messageBox(
                        "In progress",
                        "Safelist already being downloaded."
                    )
                    return
                end

                dg.currentSessions[thePath] = "t"
                local currSess = dg.dm:newSession()
                local newId = df.newObjectId()

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
                    VMatch(function()
                        print('Downloaded!')
                        dg.currentSessions[thePath] = nil
                        --dg.dm:incRevision()
                        --dg.dm:dropSession(currSess)
                        --df.releaseObject(newId)
                    end,"SLD_OutDone"),
                    VMatch(function(natPack,val)
                        -- dont care, arbitrary safelist
                        -- resumed
                    end,"SLD_OutHashUpdate","int","string"),
                    VMatch(function(natPack,out)
                        -- dont care, arbitrary safelist
                        -- resumed
                    end,"SLD_OutSizeUpdate","int","double"),
                    VMatch(function(natPack,val)
                        local vals = val:values()
                        print("The total: " .. vals._2)
                        currSess:setTotalDownloads(vals._2)
                    end,"SLD_OutTotalDownloads","int")
                )

                df.retainObjectWId(newId,handler)

                local dlFactory = df.namedMessageable("dlSessionFactory")
                local dlHandle = df.messageRetValues(dlFactory,
                    VSig("SLDF_InNewAsync"),
                    VString(thePath),
                    VMsg(handler),
                    VMsg(nil)
                )._4
                assert( dlHandle ~= nil )

            end

            local nId = df.newObjectId()

            local handler = df.makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local outPath = val:values()._2
                    afterPath(outPath)
                    df.releaseObject(nId)
                end,"GDS_OutNotifyPath","string")
            )

            df.retainObjectWId(nId,handler)

            df.message(dialogService,
                VSig("GDS_FileChooserDialog"),
                VMsg(mainWnd),
                VString("Select safelist session to resume."),
                VString("safelist_session"),
                VMsg(handler)
            )
        end,"MWI_OutResumeDownloadButtonClicked"),
        VMatch(function(natPack,val)
            local thisState = val:values()._2
            if (thisState ~= prevToggleState) then
                prevToggleState = thisState
                df.message(mainWnd,
                    VSig("MWI_InRevealDownloads"),VBool(thisState))
            end
        end,"MWI_OutShowDownloadsToggled","bool"),
        VMatch(function()
            local menuModelHandler = fileBrowserRightClickHandler(dg,df)
            df.message(mainWnd,VSig("MWI_PMM_ShowMenu"),VMsg(menuModelHandler))
        end,"MWI_OutRightClickFolderList")
    )

    df.message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))


    downloadUpdateModel = df.makeLuaMatchHandler(
        VMatch(function(natPack,vtree)
            local values = vtree:values()
            local sessNum = values._2 + 1
            local dlNum = values._3 + 1
            -- man, that lua inconsitency with
            -- 1 based arrays drives me nuts

            local sess = dg.dm:nthSession(sessNum)
            local download = sess:nthDownload(dlNum)
            local progress = roundFloatStr(download:getProgress() * 100,2)
            local done = download:getDone()
            local total = download:getTotal()
            local thelabel = download:getPath() .. " ("
                .. humanReadableBytes(done) .. " out of " .. humanReadableBytes(total)
                .. ", " .. progress .. "%)"

            natPack:setSlot(4,VString(thelabel))
            natPack:setSlot(5,VDouble(download:getProgress()))
        end,"DLMDL_QueryDownloadLabelAndProgress","int","int","string","double"),
        VMatch(function(natPack)
            natPack:setSlot(2,VInt( dg.dm:sessionCount() ))
        end,"DLMDL_QueryCount","int"),
        VMatch(function(natPack,vtree)
            local sessN = vtree:values()._2 + 1
            local sess = dg.dm:nthSession(sessN)
            local count = sess:activeDownloadCount()
            natPack:setSlot(3,VInt(count))
        end,"DLMDL_QuerySessionDownloadCount","int","int"),
        VMatch(function(natPack,vtree)
            local sessN = vtree:values()._2 + 1
            local theLabel = "Session #" ..
                dg.dm:nthSessionNum(sessN)
            natPack:setSlot(3,VString(theLabel))
        end,"DLMDL_QuerySessionTitle","int","string"),
        VMatch(function(natPack,vtree)
            local sessN = vtree:values()._2 + 1
            local sess = dg.dm:nthSession(sessN)
            local done = sess:doneDownloads()
            local doneWhole = whole(done)
            local total = sess:totalDownloads()
            local totalWhole = whole(total)
            if (total == 0) then
                total = 1
            end
            local prog = done / total
            local progRounded = tonumber(
                string.format("%.2f",prog * 100))
            local theLabel = doneWhole .. " out of "
                .. totalWhole .. " downloads done (" ..
                progRounded .. "%)"

            if (sess.loggedErrors) then
                theLabel = theLabel .. " (errors logged)"
            end

            natPack:setSlot(3,VString(theLabel))
            natPack:setSlot(4,VDouble(prog))
        end,"DLMDL_QuerySessionTotalProgress","int","string","double"),
        VMatch(function(natPack,vtree)
            local sessN = vtree:values()._2 + 1
            local sess = dg.dm:nthSession(sessN)
            local consumed = sess:consumeLog()
            natPack:setSlot(3,VString(consumed))
        end,"DLMDL_QuerySessionLog","int","string")
    )
    df.message(mainWnd,VSig("MWI_InSetDownloadModel"),VMsg(downloadUpdateModel))

end
initAll()
initAll = nil
