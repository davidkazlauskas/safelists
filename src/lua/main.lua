
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

ObjectRetainer = {
    __index = {
        newId = function(self)
            local id = self.count
            self.count = self.count + 1
            return id
        end,
        retain = function(self,id,object)
            self.table[id] = object
        end,
        retainNewId = function(self,object)
            local id = self:newId()
            self:retain(id,object)
            return id
        end,
        release = function(self,id)
            self.table[id] = nil
        end,
        new = function()
            local res = {
                count = 0,
                table = {}
            }
            setmetatable(res,ObjectRetainer)
            return res
        end
    }
}

objRetainer = ObjectRetainer.__index.new()

DownloadSpeedChecker = {
    __index = {
        new = function(samples)
            assert( type(samples) == "number", "expected num..." )
            assert( samples >= 0, "Zigga nease..." )
            local res = {
                iter = 0,
                intervals = {},
                samples = samples
            }
            for i=1,samples do
                table.insert(
                    res.intervals,
                    DownloadSpeedChecker.__index.newInterval()
                )
            end
            setmetatable(res,DownloadSpeedChecker)
            return res
        end,
        newInterval = function()
            return {
                unixStamp = 0,
                sum = 0
            }
        end,
        regBytes = function(self,bytes)
            local current = os.time()
            local mod = current % self.samples + 1
            if (self.iter ~= current) then
                self.iter = current
                self.intervals[mod].unixStamp = current
                self.intervals[mod].sum = 0
            end

            self.intervals[mod].sum = self.intervals[mod].sum + bytes
        end,
        bytesPerSec = function(self)
            local total = 0
            for k,v in ipairs(self.intervals) do
                total = total + v.sum
            end
            return total / self.samples
        end
    }
}

downloadSpeedChecker = DownloadSpeedChecker.__index.new(7)

CurrentSafelist = {
    __index = {
        isSamePath = function(self,path)
            return self.path == path
        end,
        setPath = function(self,path)
            self.path = path
        end,
        isEmpty = function(self)
            return self.path == ""
        end,
        new = function()
            local res = {
                path = ""
            }
            setmetatable(res,CurrentSafelist)
            return res
        end
    }
}

currentSafelist = CurrentSafelist.__index.new()

currentSessions = {}

revealDownloads = false

DomainGlobals = {
    currentAsyncSqlite = nil,
    shouldMoveDir = false,
    shouldMoveFile = false,
    ctx = nil,
    mainWnd = nil,
    oneOffFunctions = {},
    frameEndFunctions = {},
    sessionWidget = nil
}

DomainFunctions = {
    updateRevision = nil
}

dg = DomainGlobals
df = DomainFunctions

HashRevisionModel = {
    hashRevisionUpdate = 0,
    hashRevisionDrawingUpdate = 0
}

DownloadsModel = {
    sessions = {},
    enumerated = {},
    progressTotal = 0,
    progressDone = 0,
    currentSession = 0,
    revisionNum = 0,
    revisionUpdateNum = 0
}

dg.dm = DownloadsModel

SingleDownload = {
    __index = {
        newDownload = function (id,path)
            local res = {
                id=id,
                filePath=path,
                done=0,
                total=-1,
                speed=0 -- bytes/sec
            }
            setmetatable(res,SingleDownload)
            return res
        end,
        getProgress = function(self)
            return self.done / self.total
        end,
        getDone = function(self)
            return self.done
        end,
        getTotal = function(self)
            return self.total
        end,
        getPath = function(self)
            return self.filePath
        end,
        setProgress = function(self,bytesDone,bytesTotal)
            self.done = bytesDone
            self.total = bytesTotal
        end
    }
}

SingleSession = {
    __index = {
        addDownload = function(self,id,path)
            self.downloadTable[id] =
                SingleDownload.__index.newDownload(id,path)
            self:enumerateDownloads()
        end,
        removeDownload = function(self,id)
            self.downloadTable[id] = nil
            self.doneDownloadsNum = self.doneDownloadsNum + 1
            self:enumerateDownloads()
        end,
        enumerateDownloads = function(self)
            self.downloadEnum =
                enumerateTable(self.downloadTable)
        end,
        nthDownload = function(self,nthelement)
            return self.downloadEnum[nthelement]
        end,
        activeDownloadCount = function(self)
            return #self.downloadEnum
        end,
        doneDownloads = function(self)
            return self.doneDownloadsNum
        end,
        consumeLog = function(self)
            local res = self.pendingLog
            self.pendingLog = ""
            return res
        end,
        appendLog = function(self,newLog)
            self.pendingLog =
                self.pendingLog .. newLog .. "\n"
        end,
        totalDownloads = function(self)
            return self.totalDownloadsNum
        end,
        setTotalDownloads = function(self,num)
            self.totalDownloadsNum = num
        end,
        keyDownload = function(self,key)
            return self.downloadTable[key]
        end
    }
}

function DownloadsModel:newSession()
    self.currentSession = self.currentSession + 1
    self.progressDone = self.progressDone + 1
    local theSession = self.currentSession
    local res = {
        totalDownloadsNum = 0,
        doneDownloadsNum = 0,
        progress = 0,
        downloadTable = {},
        downloadEnum = {},
        key = theSession,
        pendingLog = ""
    }
    setmetatable(res,SingleSession)
    self.sessions[theSession] = res
    self:enumerateSessions()
    return res
end

function DownloadsModel:enumerateSessions()
    self.enumerated = enumerateTable(self.sessions)
end

function DownloadsModel:dropSession(sess)
    self.sessions[sess.key] = nil
    self.progressTotal = self.progressTotal + 1
    self:enumerateSessions()
end

function DownloadsModel:totalProgress()
    if (self.progressTotal == 0) then
        return self.progressTotal
    end
    return self.progressDone / self.progressTotal
end

function DownloadsModel:sessionCount()
    return #self.enumerated
end

function DownloadsModel:nthSession(num)
    return self.enumerated[num]
end

function DownloadsModel:nthSessionNum(num)
    return self.enumerated[num].key
end

function DownloadsModel:incRevision()
    self.revisionNum = self.revisionNum + 1
end

function DownloadsModel:tagUpdate()
    self.revisionUpdateNum = self.revisionNum
end

function DownloadsModel:isDirty()
    return self.revisionUpdateNum ~= self.revisionNum
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
    dg.objRetainer = objRetainer

    df.ctx = luaContext
    df.mainWnd = function() return dg.mainWnd end

    df.quitApplication = function()
        ctx:message(
            mainWnd,
            VSig("MWI_InQuit")
        )
    end

    df.openUrlInBrowser = function(url)
        local dialogService = ctx:namedMessageable("dialogService")
        ctx:message(
            dialogService,
            VSig("GDS_OpenUrlInBrowser"),
            VString(url)
        )
    end

    local constsTbl = {}
    constsTbl['ispaidmode'] = false

    df.globSetting =
        function(thename)
            return ctx:messageRetValues(
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
            ctx:messageAsync(
                writer,
                VSig("RFW_WriteStringToFileWDir"),
                VString(settingsFileLocation),
                VString(saveData),
                VInt(-1)
            )
        end,
        function(functionForLoading)
            ctx:messageAsyncWCallback(
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
        table.insert(dg.oneOffFunctions, newFunc)
    end

    local mainWndButtonHandlers = {}

    -- HOOK EXAMPLE
    --local awkButton = genMainWnd:getWidget("awkwardButton")
    --mainWndButtonHandlers[awkButton:hookButtonClick()] = function()
        --print("awk hooked")
    --end

    local loadCss = function(path)
        local cssMng = ctx:namedMessageable("themeManager")
        ctx:message(
            cssMng,
            VSig("THM_LoadTheme"),
            VString(path)
        )
    end

    local themes = {
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
        local path = themes[name]
        assert( nil ~= path, "Theme doesn't exist." )
        loadCss(path)
        dg.persistentSettings:setValue("safelists.theme",name)
    end

    -- menu bar model attempt
    local menuBarModelStuff = function()
        local mainWrapped = GenericWidget.putOn(dg.mainWnd)
        local menuBar = mainWrapped:getWidget("mainWindowMenuBar")

        local luaModel = MenuModel.new()
        local another = luaModel:appendSubComp("settings","Settings")
        local help = luaModel:appendSubComp("help","Help")
        local bugRep = help:appendSubLeaf("report-bug","Report a bug",
            function()
                df.openUrlInBrowser("https://bugs.launchpad.net/safelists")
            end
        )
        local themes = another:appendSubComp("settings-themes","Themes")
        local adwaita = themes:appendSubComp("theme-adwaita","Adwaita")
        adwaita:appendSubLeaf("theme-adwaita-light","light",function()
            df.loadTheme("Adwaita (light)")
        end)
        adwaita:appendSubLeaf("theme-adwaita-dark","dark",function()
            df.loadTheme("Adwaita (dark)")
        end)
        themes:appendSubLeaf("theme-raleigh","Raleigh",function()
            df.loadTheme("Raleigh")
        end)
        local vertex = themes:appendSubComp("theme-vertex","Vertex")
        vertex:appendSubLeaf("theme-vertex-light","light",function()
            df.loadTheme("Vertex (light)")
        end)
        vertex:appendSubLeaf("theme-vertex-dark","dark",function()
            df.loadTheme("Vertex (dark)")
        end)
        themes:appendSubLeaf("theme-borderline-gtk","Borderline GTK",function()
            df.loadTheme("Borderline GTK")
        end)
        local win10 = themes:appendSubComp("theme-win-10","Windows 10")
        win10:appendSubLeaf("theme-win-10-light","light",function()
            df.loadTheme("Windows 10 (light)")
        end)
        win10:appendSubLeaf("theme-win-10-dark","dark",function()
            df.loadTheme("Windows 10 (dark)")
        end)

        another:appendSubLeaf("quit-application","Quit",df.quitApplication)

        local model = luaModel:makeMessageable(dg.ctx)
        local id = objRetainer:retainNewId(model)

        menuBar:menuBarSetModelStackless(model)

        -- we need to stage this next because
        -- json settings are not already queried
        local setupCurrent = function()
            -- current default
            local currTheme = dg.persistentSettings:getValue("safelists.theme")
            local defaultTheme = "Adwaita (light)"
            if (nil == currTheme) then
                df.loadTheme(defaultTheme)
            else
                df.loadTheme(currTheme)
            end
        end

        df.scheduleOneOffFunction(setupCurrent)
    end
    menuBarModelStuff()

    local safelistDependantWigets = {
        "dirList",
        "downloadButton"
    }

    local resetVarsForSafelist = function()
        dg.currentDirToMoveId = -1
    end

    resetVarsForSafelist()

    local noSafelistState = function()
        setWidgetsEnabled(
            ctx,mainWnd,
            false,
            table.unpack(safelistDependantWigets)
        )
    end

    local onSafelistState = function()
        setWidgetsEnabled(
            ctx,mainWnd,
            true,
            table.unpack(safelistDependantWigets)
        )
    end

    df.updateRevision = function()
        HashRevisionModel.hashRevisionUpdate =
            HashRevisionModel.hashRevisionUpdate + 1
        df.updateSessionWidget()
    end

    df.addFrameEndFunction = function(another)
        table.insert(dg.frameEndFunctions,another)
    end

    local updateRevisionGui = instrument(function()
        if (HashRevisionModel.hashRevisionUpdate ==
            HashRevisionModel.hashRevisionDrawingUpdate)
        then
            return
        end

        HashRevisionModel.hashRevisionDrawingUpdate =
            HashRevisionModel.hashRevisionUpdate

        local thisCorout = coroutine.running()

        local sess = dg.currentAsyncSqlite
        assert( nil ~= sess, "Sess is null for revision read..." )

        ctx:messageAsyncWCallback(sess,
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

        ctx:message(mainWnd,VSig("MWI_InSetWidgetText"),
            VString("safelistRevisionLabel"),VString(outRes))

        end)

    local setDownloadSpeedGui = function(string)
        ctx:message(
            mainWnd,
            VSig("MWI_InSetDownloadText"),
            VString(string)
        )
    end

    local updateDownloadSpeed = function()
        downloadSpeedChecker:regBytes(0)
        local theSpeed = downloadSpeedChecker:bytesPerSec()

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

        setDownloadSpeedGui(speedString)
    end

    local newAsqlite = function(path)
        local factory = ctx:namedMessageable("asyncSqliteFactory")
        local shutdownGuard = ctx:namedMessageable("shutdownGuard")

        local result = ctx:messageRetValues(factory,
            VSig("ASQLF_CreateNew"),
            VString(path),VMsg(nil))._3

        ctx:message(shutdownGuard,
            VSig("GSI_AddNew"),
            VMsg(result))

        return result
    end

    local newSafelist = function(path)
        local res = newAsqlite(path)
        ctx:messageAsync(
            res,
            VSig("ASQL_Execute"),
            VString(newSafelistSchema())
        )
        return res
    end

    local messageBoxWParent = function(title,message,parent)
        local dialogService =
            ctx:namedMessageable("dialogService")

        local dialog =
            ctx:messageRetValues(
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

    local messageBox = function(title,message)
        local dialogService =
            ctx:namedMessageable("dialogService")

        local mainWrapped = GenericWidget.putOn(mainWnd)
        local mainAppWnd = mainWrapped:getWidget("mainAppWindow")

        messageBoxWParent(title,message,mainAppWnd:getMessageable())
    end

    df.messageBox = messageBox

    df.validateNewFileDialogFirst = function(result,dialog)
        assert( result.finished, "Should be good..." )

        if (result.name ~= nil and
            not isValidFilename(result.name)
        ) then
            messageBoxWParent(
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
                    messageBoxWParent(
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
                messageBoxWParent(
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
            messageBoxWParent(
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
            messageBoxWParent(
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

    df.newFileDialog = instrument(function(funcSuccess)
        local thisCorout = coroutine.running()
        local dialogService = ctx:namedMessageable("dialogService")

        local dialog = ctx:messageRetValues(
            dialogService,
            VSig("GDS_MakeGenericDialog"),
            VString("main"),
            VString("newFileDialog"),
            VMsg(nil)
        )._4

        local hookButton = function(widget)
            return ctx:messageRetValues(
                dialog,
                VSig("INDLG_HookButtonClick"),
                VString(widget),
                VInt(-1)
            )._3
        end

        local hookedOk = hookButton("okButton")
        local hookedCancel = hookButton("cancelButton")

        local hideDlg = function()
            ctx:message(
                dialog,
                VSig("INDLG_InHideDialog")
            )
        end

        local newId = objRetainer:newId()
        local handler = ctx:makeLuaMatchHandler(
            VMatch(resumerCallbackWBranch("answer", thisCorout),
                "INDLG_OutGenSignalEmitted","int"),
            VMatch(resumerCallbackWBranch("exited", thisCorout),
                "INDLG_OutDialogExited")
        )

        objRetainer:retain(newId,handler)

        ctx:message(
            dialog,
            VSig("INDLG_InSetNotifier"),
            VMsg(handler)
        )
        ctx:message(
            dialog,
            VSig("INDLG_InAlwaysAbove")
        )

        ctx:message(
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
                        return ctx:messageRetValues(
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
                        objRetainer:release(newId)
                    end
                elseif (signal == hookedCancel) then
                    print("Cancel clicked")
                    hideDlg()
                    objRetainer:release(newId)
                else
                    assert( false, "No such signal? " .. signal )
                end
            elseif (outBranch == "exited") then
                print("Exit vanilla")
                objRetainer:release(newId)
            else
                assert(false, "lolwut?")
            end
        end
    end)

    df.modifyFileDialog = instrument(function(fileId,funcSuccess)
        local fileIdWhole = whole(fileId)
        local dialogService = ctx:namedMessageable("dialogService")
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

        local dialog = ctx:messageRetValues(
            dialogService,
            VSig("GDS_MakeGenericDialog"),
            VString("main"),
            VString("newFileDialog"),
            VMsg(nil)
        )._4

        local hookButton = function(widget)
            return ctx:messageRetValues(
                dialog,
                VSig("INDLG_HookButtonClick"),
                VString(widget),
                VInt(-1)
            )._3
        end

        local hookedOk = hookButton("okButton")
        local hookedCancel = hookButton("cancelButton")

        local hideDlg = function()
            ctx:message(
                dialog,
                VSig("INDLG_InHideDialog")
            )
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

        local newId = objRetainer:newId()
        local handler = ctx:makeLuaMatchHandler(
            VMatch(resumerCallbackWBranch("answer", thisCorout),
                "INDLG_OutGenSignalEmitted","int"),
            VMatch(resumerCallbackWBranch("exited", thisCorout),
                "INDLG_OutDialogExited")
        )

        objRetainer:retain(newId,handler)

        ctx:message(
            dialog,
            VSig("INDLG_InSetNotifier"),
            VMsg(handler)
        )
        ctx:message(
            dialog,
            VSig("INDLG_InAlwaysAbove")
        )

        -- lookup actual data
        local query = sqlGetMirrorUsesForFile(fileIdWhole)
        local asyncSqlite = dg.currentAsyncSqlite

        ctx:messageAsyncWCallback(
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
            ctx:message(
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
                ctx:message(dialog,
                    VSig("INDLG_InSetControlEnabled"),
                    VString(name),
                    VBool(false))
            end

            offInput("fileSizeInp")
            offInput("fileHashInp")
        end

        ctx:message(
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
                        return ctx:messageRetValues(
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

                    if funcSuccess(outResult,original,dialog) then
                        print("success, returned")
                        objRetainer:release(newId)
                        return
                    else
                        print("Fail, repeating")
                    end
                elseif (signal == hookedCancel) then
                    print("Cancel clicked")
                    hideDlg()
                    objRetainer:release(newId)
                    print("fail, returned")
                    return
                else
                    assert( false, "No such signal? " .. signal )
                end
            elseif (outBranch == "exited") then
                print("Exit vanilla")
                objRetainer:release(newId)
            else
                assert(false, "lolwut?")
            end
        end
    end)


    local getCurrentEntityId = function()
        local mret = ctx:messageRetValues(mainWnd,
                        VSig("MWI_QueryCurrentEntityId"),VInt(-7),VBool(false))
        return mret._2, mret._3
    end

    local getCurrentFileParent = function()
        return ctx:messageRetValues(mainWnd,
            VSig("MWI_QueryCurrentFileParent"),VInt(-7))._2
    end
    -- export
    df.getCurrentEntityId = getCurrentEntityId
    df.getCurrentFileParent = getCurrentFileParent
    df.setStatus = function(statText)
        setStatus(dg.ctx,dg.mainWnd,statText)
    end
    df.deleteSelectedDir = function()
        dg.ctx:message(dg.mainWnd,VSig("MWI_InDeleteSelectedDir"))
    end
    df.execSqliteOnHandler = function(sqliteHandler,theStatement)
        dg.ctx:messageAsync(sqliteHandler,
            VSig("ASQL_Execute"),
            VString(theStatement))
    end

    local addNewFileUnderCurrentDir = instrument(function(data,dialog)
        local thisCorout = coroutine.running()
        local currentEntityId = getCurrentEntityId()
        local currentDirIdWhole = whole(currentEntityId)

        local asyncSqlite = dg.currentAsyncSqlite
        assert(not messageablesEqual(VMsgNil(),asyncSqlite),
            "No async sqlite." )

        -- disable Ok button for split
        -- second of async operations
        ctx:message(
            dialog,
            VSig("INDLG_InSetControlEnabled"),
            VString("okButton"),
            VBool(false)
        )

        local inputFail = function(message)
            messageBoxWParent(
                "Invalid input",
                message,
                dialog
            )
            ctx:message(
                dialog,
                VSig("INDLG_InSetControlEnabled"),
                VString("okButton"),
                VBool(true)
            )
        end

        -- !! check first
        local condition = sqlCheckForForbiddenFileNames(currentDirIdWhole,data.name)

        ctx:messageAsyncWCallback(
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
        if (case == 1) then
            inputFail( "File '" .. data.name .. "' already"
                .. " exists under current directory.")
            return false
        elseif (case == 2) then
            inputFail("Name '" .. data.name .. "' is forbidden.")
            return false
        end

        if (case ~= 0) then
            assert( false, "Say what cholo?" )
            return false
        end

        -- db validation succeeded
        local statement =
            sqlAddNewFileQuery(currentDirIdWhole,data.name,
                data.size,data.hash,data.mirrors)

        ctx:messageAsyncWCallback(
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
        ctx:messageAsyncWCallback(
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
        ctx:message(
            mainWnd,
            VSig("MWI_InAddNewFileInCurrent"),
            VInt(theId),
            VInt(currentEntityId),
            VString(data.name),
            VDouble(tonumber(data.size)),
            VString(data.hash)
        )
        ctx:message(
            dialog,
            VSig("INDLG_InHideDialog")
        )
        df.updateRevision()

        -- TODO: how to reflect db failures to dialog?
        return true
    end)

    df.addNewFileUnderCurrentDir = addNewFileUnderCurrentDir

    df.updateFileFromDiff = function(fileId,currentDirId,diffTable,orig,dialog)
        -- diffTable:
        -- finished - did finish?
        -- name - the name
        -- mirrors - the mirror string
        -- size - file size (no by default)
        -- hash - hash (no by default)
        local fileIdWhole = whole(fileId)
        local currentDirIdWhole = whole(currentDirId)

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
            ctx:message(
                dialog,
                VSig("INDLG_InHideDialog")
            )
        end

        if (missing("name","mirrors","size","hash")) then
            -- nothing changed, no worries
            hideDlg()
            return
        end

        local asyncSqlite = dg.currentAsyncSqlite

        local updateFunction = function()
            local outString = sqlUpdateFileQuery(fileIdWhole,diffTable.name,
                diffTable.size,diffTable.hash,diffTable.mirrors)

            ctx:messageAsync(
                asyncSqlite,
                VSig("ASQL_Execute"),
                VString(outString)
            )

            local merged = mergeTables(orig,diffTable)
            local toUp = -1
            if (merged.size ~= "") then
                toUp = tonumber(merged.size)
            end

            ctx:message(
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
            messageBoxWParent(
                "Invalid input",
                message,
                dialog
            )
            ctx:message(
                dialog,
                VSig("INDLG_InSetControlEnabled"),
                VString("okButton"),
                VBool(true)
            )
        end

        if (nil ~= diffTable.name) then
            -- validate if file name is forbidden
            ctx:message(
                dialog,
                VSig("INDLG_InSetControlEnabled"),
                VString("okButton"),
                VBool(false)
            )

            local currentName = diffTable.name
            local validationQuery =
                sqlCheckForForbiddenFileNamesUpdate(
                    currentDirIdWhole,fileIdWhole,currentName)
            ctx:messageAsyncWCallback(
                asyncSqlite,
                function(out)
                    local tbl = out:values()
                    local isGood = tbl._4
                    local case = tbl._3

                    assert( isGood, "Take sqlite 101, sucker." )
                    if (case == 0) then
                        updateFunction()
                        return
                    end

                    if (case == 1) then
                        inputFail( "File '" .. currentName .. "' already"
                            .. " exists under current directory.")
                        return
                    elseif (case == 2) then
                        inputFail("Name '" .. currentName .. "' is forbidden.")
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

    df.addFrameEndFunction(updateRevisionGui)
    df.addFrameEndFunction(df.updateSessionWidget)
    df.addFrameEndFunction(updateDownloadSpeed)

    df.addFrameEndFunction(function()
        dg.persistentSettings:persist()
    end)

    noSafelistState()

    ctx:attachContextTo(mainWnd)
    dg.sessionWidget = ctx:messageRetValues(mainWnd,
        VSig("MWI_QueryDownloadSessionWidget"),VMsg(nil))._2

    resetVarsForSafelist()

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
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
            mainWndButtonHandlers[index]()
        end,"GWI_GBT_OutClickEvent","int"),
        VMatch(function()
            local mainModel = ctx:namedMessageable("mainModel")
            local asyncSqlite = dg.currentAsyncSqlite
            if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                return
            end
            ctx:message(mainModel,
                VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
        end,"MWI_OutNewFileSignal"),
        VMatch(instrument(function(natpack,val)
            local thisCorout = coroutine.running()
            local inId = val:values()._2

            dg.currentDirId = inId
            local currentDirIdWhole = whole(dg.currentDirId)

            local loadCurrentRoutine = function()
                local mainModel = ctx:namedMessageable("mainModel")
                local asyncSqlite = dg.currentAsyncSqlite
                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                    return
                end
            end

            if (dg.currentDirId > 0 and dg.shouldMoveFile == true) then
                dg.shouldMoveFile = false

                local _, isDir = getCurrentEntityId()
                if (not isDir) then
                    messageBox(
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

                ctx:messageAsyncWCallback(
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
                    messageBoxWParent(
                        "Invalid move",
                        "File with such name already"
                        .. " exists under that directory.",
                        mainWnd
                    )
                    loadCurrentRoutine()
                elseif (case == 2) then
                    messageBoxWParent(
                        "Invalid move",
                        "File cannot be moved under"
                        .. " this directory.",
                        mainWnd
                    )
                    loadCurrentRoutine()
                elseif (case == 0) then
                    local updateQuery = sqlMoveFileStatement(toMoveWhole,currentDirIdWhole)

                    ctx:messageAsync(
                        asyncSqlite,
                        VSig("ASQL_Execute"),
                        VString(updateQuery)
                    )
                    ctx:message(
                        mainWnd,
                        VSig("MWI_InAddNewFileInCurrent"),
                        VInt(toMove),
                        VInt(dg.currentDirId),
                        VString(fileName),
                        VDouble(fileSize),
                        VString(hash)
                    )
                    ctx:message(
                        mainWnd,
                        VSig("MWI_InDeleteSelectedDir"),
                        VInt(1)
                    )
                    loadCurrentRoutine()
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
                    messageBox(
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
                local mainWnd = ctx:namedMessageable("mainWindow")
                local mainModel = ctx:namedMessageable("mainModel")

                local condition = sqlMoveDirCondition(inIdWhole, currentDirToMoveIdWhole)

                ctx:messageAsyncWCallback(
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
                print("val|" .. value .. "|")
                if (value == 0) then
                    ctx:messageAsync(asyncSqlite,
                        VSig("ASQL_OutAffected"),
                        VString(sqlMoveDirStatement(currentDirToMoveIdWhole,inIdWhole)),
                        VInt(-1))
                    dg.currentDirToMoveId = -1
                    ctx:message(mainWnd,
                        VSig("MWI_InMoveChildUnderParent"),
                        VInt(-1))
                    df.updateRevision()
                    loadCurrentRoutine()
                elseif (value == 1) then
                    messageBox(
                        "Cannot move!",
                        "Directory to move cannot be a parent"
                        .. " of directory to move under."
                    )
                elseif (value == 2) then
                    messageBoxWParent(
                        "Cannot move!",
                        "Parent directory already"
                        .. " has directory with such name.",
                        mainWnd)
                elseif (value == 3) then
                    messageBox(
                        "Cannot move!",
                        "Directory is already under"
                        .. " this parent."
                    )
                else
                    assert( false, "Should not happen cholo..." )
                end
                return
            end
            loadCurrentRoutine()
        end),"MWI_OutDirChangedSignal","int"),
        VMatch(function()
            local dlFactory = ctx:namedMessageable("dlSessionFactory")
            local dialogService = ctx:namedMessageable("dialogService")
            local asyncSqlite = dg.currentAsyncSqlite
            if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                assert( false, "Didn't expect download request" ..
                    " with null safelist.")
                return
            end

            local isCurrentDead = function()
                return ctx:messageRetValues(
                    asyncSqlite,
                    VSig("ASQL_IsDead"),
                    VBool(true)
                )._2
            end

            local afterDirectory = function(downloadPath)

                if (downloadPath == "") then
                    return
                end

                assert( downloadPath[#downloadPath] ~= "/",
                    "Don't expect slash at the end." )

                downloadPath = downloadPath .. "/safelist_session"

                if (currentSessions[downloadPath] == "t") then
                    messageBox(
                        "In progress",
                        "Safelist already being downloaded."
                    )
                    return
                end

                currentSessions[downloadPath] = "t"

                --print("Pre col: " .. collectgarbage('count'))
                --collectgarbage('collect')
                --print("Post col: " .. collectgarbage('count'))

                local currSess = dg.dm:newSession()
                currSess.loggedErrors = false

                local appendLog = function(theStr)
                    currSess:appendLog(theStr)
                    currSess.loggedErrors = true
                end

                local newId = objRetainer:newId()
                local handlerWeak = nil
                local handler = ctx:makeLuaMatchHandler(
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
                        downloadSpeedChecker:regBytes(newBytes)
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
                    VMatch(function(natpack,val)
                        -- MAKE-PRETTY
                        local valTree = val:values()
                        local delKey = valTree._2
                        print("File not found brah: |" .. delKey .. "|")
                        dg.dm:incRevision()
                        currSess:removeDownload(delKey)
                    end,"SLD_OutFileNotFound","int"),
                    VMatch(function()
                        print('Downloaded!')
                        currentSessions[downloadPath] = nil
                        --dg.dm:incRevision()
                        --dg.dm:dropSession(currSess)
                        --objRetainer:release(newId)
                    end,"SLD_OutDone"),
                    VMatch(function(natpack,val)
                        -- back in the day use counts were incremented
                        -- but it's a waste of time because now hashes
                        -- don't verify that two safelists are the same
                        return
                    end,"SLD_OutMirrorUsed","int","string"),
                    VMatch(instrument(function(natPack,val)
                        local thisCorout = coroutine.running()
                        local values = val:values()
                        local hash = values._3
                        if (nil == asyncSqlite or isCurrentDead()) then
                            return
                        end

                        local id = values._2
                        local idWhole = whole(id)
                        local theDl = currSess:keyDownload(id)
                        local thePath = theDl:getPath()

                        ctx:messageAsyncWCallback(asyncSqlite,
                            resumerCallbackValues(thisCorout),
                            VSig("ASQL_OutSingleRow"),
                            VString(sqlGetFileHash(idWhole)),
                            VString(""),
                            VBool(false))

                        local outVal = coroutine.yield()
                        local qhash = outVal._3

                        assert( outVal._4, "Query failed..." )
                        --assert( qhash ~= hash, "Hash collision, hash is different."
                            --.. " (todo: handle this case)" )

                        if (qhash ~= hash and qhash ~= "") then
                            appendLog(
                              "Hash mismatch: " .. thePath
                              .. " is reported to be of hash \"" .. qhash .. "\""
                              .. " but turns out to be \"" .. hash .. "\"."
                              .. " Are mirrors pointing to the same file?"
                            )
                        else
                            ctx:messageAsync(
                                asyncSqlite,
                                VSig("ASQL_Execute"),
                                VString(sqlUpdateFileHashStatement(idWhole,hash))
                            )
                            df.updateRevision()
                        end
                    end),"SLD_OutHashUpdate","int","string"),
                    VMatch(function(natPack,out)
                        if (isCurrentDead()) then
                            return
                        end

                        local val = out:values()
                        local id = val._2
                        local idWhole = whole(id)
                        local newSize = val._3
                        -- size collision already checked with assert
                        ctx:messageAsync(
                            asyncSqlite,
                            VSig("ASQL_Execute"),
                            VString(sqlUpdateFileSizeStatement(idWhole,newSize))
                        )
                        df.updateRevision()
                    end,"SLD_OutSizeUpdate","int","double"),
                    VMatch(function(natPack,out)
                        local val = out:values()
                        local id = val._2
                        local exp = val._3
                        local real = val._4
                        local theDl = currSess:keyDownload(id)
                        appendLog(
                          "Size mismatch: " .. theDl:getPath()
                          .. " is reported to be of size " .. whole(exp)
                          .. " but turns out to be " .. whole(real) .. "."
                          .. " Are mirrors pointing to the same file?"
                        )
                    end,"SLD_OutSizeMismatch","int","double","double"),
                    VMatch(function()
                        print('Safelist session dun! Downloading...')
                        local locked = handlerWeak:lockPtr()
                        assert( nil ~= locked , "Locking weak ptr gives nil..." )
                        local dlHandle = ctx:messageRetValues(dlFactory,
                            VSig("SLDF_InNewAsync"),
                            VString(downloadPath),
                            VMsg(locked),
                            VMsg(nil)
                        )._4
                        assert( dlHandle ~= nil )
                    end,"SLDF_OutCreateSessionDone"),
                    VMatch(function(natPack,val)
                        local vals = val:values()
                        print("The total: " .. vals._2)
                        currSess:setTotalDownloads(vals._2)
                    end,"SLD_OutTotalDownloads","int")
                )

                handlerWeak = handler:getWeak()
                objRetainer:retain(newId,handler)

                ctx:message(dlFactory,
                    VSig("SLDF_CreateSession"),
                    VMsg(asyncSqlite),
                    VMsg(handler),
                    VString(downloadPath)
                )

            end

            local nId = objRetainer:newId()

            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local outPath = val:values()._2
                    afterDirectory(outPath)
                    objRetainer:release(nId)
                end,"GDS_OutNotifyPath","string")
            )

            objRetainer:retain(nId,handler)

            local outVal = ctx:message(dialogService,
                VSig("GDS_DirChooserDialog"),
                VMsg(mainWnd),
                VString("Select download location."),
                VMsg(handler))
        end,"MWI_OutDownloadSafelistButtonClicked"),
        VMatch(function()
            local dialogService = ctx:namedMessageable("dialogService")

            local afterPath = function(outPath)
                if (outPath ~= "") then
                    if (currentSafelist:isSamePath(outPath)) then
                        messageBox(
                            "Already opened!",
                            "'" .. outPath ..
                            "' safelist is already opened."
                        )
                        return
                    end
                    currentSafelist:setPath(outPath)
                    if (not currentSafelist:isEmpty()) then
                        noSafelistState() -- prevent user from doing
                                          -- anything for split second
                    end

                    local openNew = function()
                        local mainModel = ctx:namedMessageable("mainModel")

                        dg.currentAsyncSqlite = newAsqlite(outPath)

                        ctx:message(mainModel,
                            VSig("MMI_InLoadFolderTree"),
                            VMsg(dg.currentAsyncSqlite),VMsg(mainWnd))
                        onSafelistState()
                        df.updateRevision()
                    end

                    local asql = dg.currentAsyncSqlite
                    if (nil ~= asql) then
                        ctx:messageAsyncWCallback(
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

            local nId = objRetainer:newId()

            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local outPath = val:values()._2
                    local folder = string.match(outPath,".+/")
                    if (folder ~= nil) then
                        dg.persistentSettings:setValue(
                            "safelists.lastopen",folder)
                    end
                    afterPath(outPath)
                    objRetainer:release(nId)
                end,"GDS_OutNotifyPath","string")
            )

            objRetainer:retain(nId,handler)

            ctx:message(dialogService,
                VSig("GDS_FileChooserDialog"),
                VMsg(mainWnd),
                VString("Select safelist to open."),
                VString("*.safelist"),
                VString(dg.persistentSettings:getValueDefault(
                    "safelists.lastopen",examplesPath)),
                VMsg(handler))
        end,"MWI_OutOpenSafelistButtonClicked"),
        VMatch(function()
            local dialogService = ctx:namedMessageable("dialogService")
            local afterPath = instrument(function(outPath)
                if (outPath == "") then
                    return
                end

                local thisCorout = coroutine.running()

                if (not string.ends(string.lower(outPath),".safelist")) then
                    outPath = outPath .. ".safelist"
                end

                local ifContinue = function()
                    local openNew = function()
                        local mainModel = ctx:namedMessageable("mainModel")

                        dg.currentAsyncSqlite = newSafelist(outPath)
                        local new = dg.currentAsyncSqlite
                        resetVarsForSafelist()
                        ctx:message(mainModel,
                            VSig("MMI_InLoadFolderTree"),
                            VMsg(new),VMsg(mainWnd))
                        df.updateRevision()
                        onSafelistState()
                    end

                    local prev = dg.currentAsyncSqlite
                    if (nil ~= prev) then
                        ctx:messageAsyncWCallback(
                            prev,
                            openNew,
                            VSig("ASQL_Shutdown"))
                        return
                    end

                    openNew()
                end

                noSafelistState()

                ctx:messageAsyncWCallback(
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
                    local dialogService =
                        ctx:namedMessageable("dialogService")

                    local afterAnswer = function(response)

                        if (response == 0) then
                            ctx:messageAsyncWCallback(
                                writer,
                                ifContinue,
                                VSig("RFW_DeleteFile"),
                                VString(outPath)
                            )
                        elseif (response == 1 or response == -1) then
                            noSafelistState()
                        else
                            assert( false, "Wrong neighbourhood, milky." )
                            noSafelistState()
                        end

                    end

                    local nId = objRetainer:newId()

                    local handler = ctx:makeLuaMatchHandler(
                        VMatch(function(natPack,val)
                            local outPath = val:values()._2
                            afterAnswer(outPath)
                            objRetainer:release(nId)
                        end,"GDS_OutNotifyAnswer","int")
                    )

                    objRetainer:retain(nId,handler)

                    ctx:message(
                        dialogService,
                        VSig("GDS_OkCancelDialog"),
                        VMsg(mainWnd),
                        VString("Safelist exists"),
                        VString("Safelist already exists."
                        .. " Overwrite it? (data will be lost)"),
                        VMsg(handler)
                    )
                end
            end)

            local nId = objRetainer:newId()

            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local outPath = val:values()._2
                    afterPath(outPath)
                    objRetainer:release(nId)
                end,"GDS_OutNotifyPath","string")
            )

            objRetainer:retain(nId,handler)

            ctx:message(dialogService,
                VSig("GDS_FileSaverDialog"),
                VMsg(mainWnd),
                VString("Select new safelist path"),
                VMsg(handler)
            )
        end,"MWI_OutCreateSafelistButtonClicked"),
        VMatch(function()

            local dialogService = ctx:namedMessageable("dialogService")

            local afterPath = function(thePath)

                if (currentSessions[thePath] == "t") then
                    messageBox(
                        "In progress",
                        "Safelist already being downloaded."
                    )
                    return
                end

                currentSessions[thePath] = "t"
                local currSess = dg.dm:newSession()
                local newId = objRetainer:newId()

                local handler = ctx:makeLuaMatchHandler(
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
                        downloadSpeedChecker:regBytes(newBytes)
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
                        currentSessions[thePath] = nil
                        --dg.dm:incRevision()
                        --dg.dm:dropSession(currSess)
                        --objRetainer:release(newId)
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

                objRetainer:retain(newId,handler)

                local dlFactory = ctx:namedMessageable("dlSessionFactory")
                local dlHandle = ctx:messageRetValues(dlFactory,
                    VSig("SLDF_InNewAsync"),
                    VString(thePath),
                    VMsg(handler),
                    VMsg(nil)
                )._4
                assert( dlHandle ~= nil )

            end

            local nId = objRetainer:newId()

            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local outPath = val:values()._2
                    afterPath(outPath)
                    objRetainer:release(nId)
                end,"GDS_OutNotifyPath","string")
            )

            objRetainer:retain(nId,handler)

            ctx:message(dialogService,
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
                ctx:message(mainWnd,
                    VSig("MWI_InRevealDownloads"),VBool(thisState))
            end
        end,"MWI_OutShowDownloadsToggled","bool"),
        VMatch(function()
            local menuModelHandler = fileBrowserRightClickHandler(dg,df)
            ctx:message(mainWnd,VSig("MWI_PMM_ShowMenu"),VMsg(menuModelHandler))
        end,"MWI_OutRightClickFolderList")
    )

    ctx:message(mainWnd,VSig("MWI_InAttachListener"),VMsg(mainWindowPushButtonHandler))


    downloadUpdateModel = ctx:makeLuaMatchHandler(
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
    ctx:message(mainWnd,VSig("MWI_InSetDownloadModel"),VMsg(downloadUpdateModel))

end
initAll()
initAll = nil
