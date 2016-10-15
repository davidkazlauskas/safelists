
--require('lua/mobdebug').start()

package.path = package.path .. ";" .. LUA_SCRIPTS_PATH .. "/?.lua"

require('util')
require('sqlite')
require('safelist-constants')
require('genericwidget')
require('settings')
require('messages')

-- on select function receives integer option selected,
-- -1 if none
function makePopupMenuModel(context,table,onSelectFunction)
    local theModel = table

    local menuModelHandler = context:makeLuaMatchHandler(
        VMatch(function(natpack,val)
            local num = val:values()._2 + 1
            natpack:setSlot(3,VString(theModel[num]))
        end,"MWI_PMM_QueryItem"),
        VMatch(function(natpack,val)
            natpack:setSlot(2,VInt(#theModel))
        end,"MWI_PMM_QueryCount","int"),
        VMatch(function(natPack,val)
            local res = val:values()._2
            onSelectFunction(res)
        end,"MWI_PMM_OutSelected","int")
    )

    return menuModelHandler
end

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
sessionWidget = nil
currentAsyncSqlite = nil

FrameEndFunctions = {}
OneOffFunctions = {}

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

function updateSessionWidget()
    -- global state of session move, should we move this?
    local wgt = sessionWidget
    if (nil == wgt) then
        return
    end

    if (DownloadsModel:isDirty()) then
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

    local quitApplication = function()
        ctx:message(
            mainWnd,
            VSig("MWI_InQuit")
        )
    end

    local openUrlInBrowser = function(url)
        local dialogService = ctx:namedMessageable("dialogService")
        ctx:message(
            dialogService,
            VSig("GDS_OpenUrlInBrowser"),
            VString(url)
        )
    end

    local constsTbl = {}
    constsTbl['ispaidmode'] = false

    local isPaidMode = function()
        return constsTbl['ispaidmode']
    end

    local isReleaseBuild = function()
        if (constsTbl['isrelease'] == nil) then
            local tmp = ctx:messageRetValues(
                globConsts,
                VSig("GLC_LookupString"),
                VString("isrelease"),
                VString(""),
                VInt(-1)
            )._3
            if (tmp == "true") then
                constsTbl['isrelease'] = true
            else
                constsTbl['isrelease'] = false
            end
        end

        return constsTbl['isrelease']
    end

    local globSetting =
        function(thename)
            return ctx:messageRetValues(
                globConsts,
                VSig("GLC_LookupString"),
                VString(thename),
                VString(""),
                VInt(-1)
            )._3
        end

    local settingsFileLocation = globSetting("settingspath")
    local examplesPath = globSetting("appdatapath") .. "/examples/"

    local persistentSettings = PersistentSettings.new(
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

    local loadTheme = function(name)
        local path = themes[name]
        assert( nil ~= path, "Theme doesn't exist." )
        loadCss(path)
        persistentSettings:setValue("safelists.theme",name)
    end

    -- menu bar model attempt
    local menuBarModelStuff = function()
        local mainWrapped = GenericWidget.putOn(mainWnd)
        local menuBar = mainWrapped:getWidget("mainWindowMenuBar")

        local luaModel = MenuModel.new()
        local another = luaModel:appendSubComp("settings","Settings")
        local help = luaModel:appendSubComp("help","Help")
        local bugRep = help:appendSubLeaf("report-bug","Report a bug",
            function()
                openUrlInBrowser("https://bugs.launchpad.net/safelists")
            end
        )
        local themes = another:appendSubComp("settings-themes","Themes")
        local adwaita = themes:appendSubComp("theme-adwaita","Adwaita")
        adwaita:appendSubLeaf("theme-adwaita-light","light",function()
            loadTheme("Adwaita (light)")
        end)
        adwaita:appendSubLeaf("theme-adwaita-dark","dark",function()
            loadTheme("Adwaita (dark)")
        end)
        themes:appendSubLeaf("theme-raleigh","Raleigh",function()
            loadTheme("Raleigh")
        end)
        local vertex = themes:appendSubComp("theme-vertex","Vertex")
        vertex:appendSubLeaf("theme-vertex-light","light",function()
            loadTheme("Vertex (light)")
        end)
        vertex:appendSubLeaf("theme-vertex-dark","dark",function()
            loadTheme("Vertex (dark)")
        end)
        themes:appendSubLeaf("theme-borderline-gtk","Borderline GTK",function()
            loadTheme("Borderline GTK")
        end)
        local win10 = themes:appendSubComp("theme-win-10","Windows 10")
        win10:appendSubLeaf("theme-win-10-light","light",function()
            loadTheme("Windows 10 (light)")
        end)
        win10:appendSubLeaf("theme-win-10-dark","dark",function()
            loadTheme("Windows 10 (dark)")
        end)

        another:appendSubLeaf("quit-application","Quit",quitApplication)

        local model = luaModel:makeMessageable(ctx)
        local id = objRetainer:retainNewId(model)

        menuBar:menuBarSetModelStackless(model)

        -- we need to stage this next because
        -- json settings are not already queried
        local setupCurrent = function()
            -- current default
            local currTheme = persistentSettings:getValue("safelists.theme")
            local defaultTheme = "Adwaita (light)"
            if (nil == currTheme) then
                loadTheme(defaultTheme)
            else
                loadTheme(currTheme)
            end
        end

        table.insert(OneOffFunctions,setupCurrent)
    end
    menuBarModelStuff()

    local safelistDependantWigets = {
        "dirList",
        "downloadButton"
    }

    local resetVarsForSafelist = function()
        currentDirId = -1
        currentDirToMoveId = -1
    end

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

    local updateRevision = function()
        HashRevisionModel.hashRevisionUpdate =
            HashRevisionModel.hashRevisionUpdate + 1
        FrameEndFunctions[2]()
    end

    local updateRevisionGui = function()
        if (HashRevisionModel.hashRevisionUpdate ==
            HashRevisionModel.hashRevisionDrawingUpdate)
        then
            return
        end

        HashRevisionModel.hashRevisionDrawingUpdate =
            HashRevisionModel.hashRevisionUpdate

        local sess = currentAsyncSqlite
        assert( nil ~= sess, "Sess is null for revision read..." )
        ctx:messageAsyncWCallback(sess,
            function(out)
                local values = out:values()
                local succeeded = values._4
                local outString = values._3
                assert( succeeded, "Great success!" )
                local split = outString:split("|")
                local outRes = "Safelist revision: " .. split[1] ..
                    ", last modification date: " .. split[2]
                ctx:message(mainWnd,VSig("MWI_InSetWidgetText"),
                    VString("safelistRevisionLabel"),VString(outRes))
            end,
            VSig("ASQL_OutSingleRow"),VString(sqlRevNumber()),
            VString("empty"),VBool(false))
    end

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

    local validateNewFileDialogFirst = function(result,dialog)
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

    local newFileDialog = function(funcSuccess)
        local dialogService = ctx:namedMessageable("dialogService")

        -- Return results:
        -- finished - did finish?
        -- name - the name
        -- mirrors - the mirror string
        -- size - file size (no by default)
        -- hash - hash (no by default)
        local outResult = {
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

        local newId = objRetainer:newId()
        local handler = ctx:makeLuaMatchHandler(
            VMatch(function(natpack,val)
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

                    outResult.finished = true
                    outResult.name = queryInput("fileNameInp")
                    outResult.mirrors = queryInput("mirrorsTextView")
                    outResult.size = queryInput("fileSizeInp")
                    outResult.hash = queryInput("fileHashInp")

                    funcSuccess(outResult,dialog)
                elseif (signal == hookedCancel) then
                    print("Cancel clicked")
                    hideDlg()
                    objRetainer:release(newId)
                else
                    assert( false, "No such signal? " .. signal )
                end
            end,"INDLG_OutGenSignalEmitted","int"),
            VMatch(function()
                print("Exit vanilla")
                objRetainer:release(newId)
            end,"INDLG_OutDialogExited")
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
    end

    local modifyFileDialog = function(fileId,funcSuccess)
        local fileIdWhole = whole(fileId)
        local dialogService = ctx:namedMessageable("dialogService")

        -- Return results:
        -- finished - did finish?
        -- name - the name
        -- mirrors - the mirror string
        -- size - file size (no by default)
        -- hash - hash (no by default)
        local outResult = {
            finished = false
        }

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
            VMatch(function(natpack,val)
                local signal = val:values()._2
                if (signal == hookedOk) then

                    local queryInput = function(value)
                        return ctx:messageRetValues(
                            dialog,
                            VSig("INDLG_QueryInput"),
                            VString(value),
                            VString(""))._3
                    end

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

                    funcSuccess(outResult,original,dialog)
                elseif (signal == hookedCancel) then
                    print("Cancel clicked")
                    hideDlg()
                    objRetainer:release(newId)
                else
                    assert( false, "No such signal? " .. signal )
                end
            end,"INDLG_OutGenSignalEmitted","int"),
            VMatch(function()
                print("Exit vanilla")
                objRetainer:release(newId)
            end,"INDLG_OutDialogExited")
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
        local asyncSqlite = currentAsyncSqlite

        ctx:messageAsyncWCallback(
            asyncSqlite,
            function(out)
                local tbl = out:values()
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
            end,
            VSig("ASQL_OutSingleRow"),
            VString(query),
            VString(""),
            VBool(false)
        )
    end


    local getCurrentEntityId = function()
        local mret = ctx:messageRetValues(mainWnd,
                        VSig("MWI_QueryCurrentEntityId"),VInt(-7),VBool(false))
        return mret._2, mret._3
    end

    local getCurrentFileParent = function()
        return ctx:messageRetValues(mainWnd,
            VSig("MWI_QueryCurrentFileParent"),VInt(-7))._2
    end

    local addNewFileUnderCurrentDir = function(data,dialog)

        local currentEntityId = getCurrentEntityId()
        local currentDirIdWhole = whole(currentEntityId)

        local asyncSqlite = currentAsyncSqlite
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

        local onSuccess = function()
            local statement =
                sqlAddNewFileQuery(currentDirIdWhole,data.name,
                    data.size,data.hash,data.mirrors)

            ctx:messageAsyncWCallback(
                asyncSqlite,
                function()
                    -- you know, I'd love a feature
                    -- in sqlite to query something
                    -- right after insert, that'd be great.
                    local lastFileId = currentFileIdSelect .. ";"
                    ctx:messageAsyncWCallback(
                        asyncSqlite,
                        function(val)
                            local tbl = val:values()
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
                            updateRevision()
                        end,
                        VSig("ASQL_OutSingleNum"),
                        VString(lastFileId),
                        VInt(-1),
                        VBool(false)
                    )
                end,
                VSig("ASQL_Execute"),
                VString(statement)
            )
        end

        ctx:messageAsyncWCallback(
            asyncSqlite,
            function(outres)
                local val = outres:values()
                local success = val._4
                assert( success, "YOU GET NOTHING, YOU LOSE" )
                local case = val._3
                if (case == 1) then
                    inputFail( "File '" .. data.name .. "' already"
                        .. " exists under current directory.")
                    return
                elseif (case == 2) then
                    inputFail("Name '" .. data.name .. "' is forbidden.")
                    return
                end

                if (case ~= 0) then
                    assert( false, "Say what cholo?" )
                    return
                end

                onSuccess()
            end,
            VSig("ASQL_OutSingleNum"),
            VString(condition),
            VInt(-1),
            VBool(false)
        )
    end

    local updateFileFromDiff = function(fileId,currentDirId,diffTable,orig,dialog)
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

        local asyncSqlite = currentAsyncSqlite

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
            updateRevision()
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

    table.insert(FrameEndFunctions,updateRevisionGui)
    table.insert(FrameEndFunctions,updateSessionWidget)
    table.insert(FrameEndFunctions,updateDownloadSpeed)

    table.insert(FrameEndFunctions,function()
        persistentSettings:persist()
    end)

    noSafelistState()

    ctx:attachContextTo(mainWnd)
    sessionWidget = ctx:messageRetValues(mainWnd,
        VSig("MWI_QueryDownloadSessionWidget"),VMsg(nil))._2

    resetVarsForSafelist()

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
        VMatch(function()
            local oneOffSteal = OneOffFunctions
            OneOffFunctions = {}
            for k,v in ipairs(oneOffSteal) do
                v()
            end

            for k,v in ipairs(FrameEndFunctions) do
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
            local asyncSqlite = currentAsyncSqlite
            if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                return
            end
            ctx:message(mainModel,
                VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
        end,"MWI_OutNewFileSignal"),
        VMatch(function(natpack,val)
            local inId = val:values()._2

            local currentDirId = inId
            local currentDirIdWhole = whole(currentDirId)

            local loadCurrentRoutine = function()
                local mainModel = ctx:namedMessageable("mainModel")
                local asyncSqlite = currentAsyncSqlite
                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                    return
                end
            end

            if (currentDirId > 0 and shouldMoveFile == true) then
                shouldMoveFile = false

                local _, isDir = getCurrentEntityId()
                if (not isDir) then
                    messageBox(
                        "Cannot move!",
                        "Cannot move under a file."
                    )
                    return
                end

                setStatus(ctx,mainWnd,"")
                local toMove = fileToMove
                local toMoveWhole = whole(toMove)
                local asyncSqlite = currentAsyncSqlite
                assert( not messageablesEqual(VMsgNil(),asyncSqlite),
                    "Huh cholo?" )

                local condition = sqlMoveFileValidation(toMoveWhole,currentDirIdWhole)

                ctx:messageAsyncWCallback(
                    asyncSqlite,
                    function(out)
                        local val = out:values()
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
                                VInt(currentDirId),
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
                            updateRevision()
                        else
                            assert( false, "Huh?!?" )
                        end
                    end,
                    VSig("ASQL_OutSingleRow"),
                    VString(condition),
                    VString(""),
                    VBool(false)
                )
                return
            end

            if (currentDirToMoveId > 0 and shouldMoveDir == true) then
                shouldMoveDir = false

                ctx:message(mainWnd,VSig("MWI_InSetStatusText"),VString(""))
                if (inId == currentDirToMoveId) then
                    return
                end

                local _, isDir = getCurrentEntityId()
                if (not isDir) then
                    messageBox(
                        "Cannot move!",
                        "Cannot move under a file."
                    )
                    return
                end

                local inIdWhole = whole(inId)
                local currentDirToMoveIdWhole = whole(currentDirToMoveId)

                local asyncSqlite = currentAsyncSqlite
                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                    return
                end
                local mainWnd = ctx:namedMessageable("mainWindow")
                local mainModel = ctx:namedMessageable("mainModel")

                local condition = sqlMoveDirCondition(inIdWhole, currentDirToMoveIdWhole)

                ctx:messageAsyncWCallback(
                    asyncSqlite,
                    function(outres)
                        local table = outres:values()
                        local value = table._3
                        local success = table._4
                        --assert( success, "Great success failed..." )
                        print("val|" .. value .. "|")
                        if (value == 0) then
                            ctx:messageAsync(asyncSqlite,
                                VSig("ASQL_OutAffected"),
                                VString(sqlMoveDirStatement(currentDirToMoveIdWhole,inIdWhole)),
                                VInt(-1))
                            currentDirToMoveId = -1
                            ctx:message(mainWnd,
                                VSig("MWI_InMoveChildUnderParent"),
                                VInt(-1))
                            updateRevision()
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
                    end,
                    VSig("ASQL_OutSingleNum"),
                    VString(condition),
                    VInt(-1),
                    VBool(false))
                return
            end
            loadCurrentRoutine()
        end,"MWI_OutDirChangedSignal","int"),
        VMatch(function()
            local dlFactory = ctx:namedMessageable("dlSessionFactory")
            local dialogService = ctx:namedMessageable("dialogService")
            local asyncSqlite = currentAsyncSqlite
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

                local currSess = DownloadsModel:newSession()
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
                            DownloadsModel:incRevision()
                            dl:setProgress(done,total)
                        end
                        local newBytes = values._5
                        downloadSpeedChecker:regBytes(newBytes)
                    end,"SLD_OutProgressUpdate","int","double","double","double"),
                    VMatch(function(natpack,val)
                        local valTree = val:values()
                        local newKey = valTree._2
                        local newPath = valTree._3
                        DownloadsModel:incRevision()
                        currSess:addDownload(newKey,newPath)
                    end,"SLD_OutStarted","int","string"),
                    VMatch(function(natpack,val)
                        local valTree = val:values()
                        local delKey = valTree._2
                        DownloadsModel:incRevision()
                        currSess:removeDownload(delKey)
                    end,"SLD_OutSingleDone","int"),
                    VMatch(function(natpack,val)
                        -- MAKE-PRETTY
                        local valTree = val:values()
                        local delKey = valTree._2
                        print("File not found brah: |" .. delKey .. "|")
                        DownloadsModel:incRevision()
                        currSess:removeDownload(delKey)
                    end,"SLD_OutFileNotFound","int"),
                    VMatch(function()
                        print('Downloaded!')
                        currentSessions[downloadPath] = nil
                        --DownloadsModel:incRevision()
                        --DownloadsModel:dropSession(currSess)
                        --objRetainer:release(newId)
                    end,"SLD_OutDone"),
                    VMatch(function(natpack,val)
                        -- back in the day use counts were incremented
                        -- but it's a waste of time because now hashes
                        -- don't verify that two safelists are the same
                        return
                    end,"SLD_OutMirrorUsed","int","string"),
                    VMatch(function(natPack,val)
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
                            function (out)
                                local outVal = out:values()
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
                                    updateRevision()
                                end
                            end,
                            VSig("ASQL_OutSingleRow"),
                            VString(sqlGetFileHash(idWhole)),
                            VString(""),
                            VBool(false))
                    end,"SLD_OutHashUpdate","int","string"),
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
                        updateRevision()
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

                        currentAsyncSqlite = newAsqlite(outPath)

                        ctx:message(mainModel,
                            VSig("MMI_InLoadFolderTree"),
                            VMsg(currentAsyncSqlite),VMsg(mainWnd))
                        onSafelistState()
                        updateRevision()
                    end

                    local asql = currentAsyncSqlite
                    if (nil ~= asql) then
                        ctx:messageAsyncWCallback(
                            currentAsyncSqlite,
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
                        persistentSettings:setValue(
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
                VString(persistentSettings:getValueDefault(
                    "safelists.lastopen",examplesPath)),
                VMsg(handler))
        end,"MWI_OutOpenSafelistButtonClicked"),
        VMatch(function()
            local dialogService = ctx:namedMessageable("dialogService")
            local afterPath = function(outPath)
                if (outPath == "") then
                    return
                end

                if (not string.ends(string.lower(outPath),".safelist")) then
                    outPath = outPath .. ".safelist"
                end

                local ifContinue = function()
                    local openNew = function()
                        local mainModel = ctx:namedMessageable("mainModel")

                        currentAsyncSqlite = newSafelist(outPath)
                        local new = currentAsyncSqlite
                        resetVarsForSafelist()
                        ctx:message(mainModel,
                            VSig("MMI_InLoadFolderTree"),
                            VMsg(new),VMsg(mainWnd))
                        updateRevision()
                        onSafelistState()
                    end

                    local prev = currentAsyncSqlite
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
                    function(out)
                        local tbl = out:values()
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
                    end,
                    VSig("RFW_DoesFileExist"),
                    VString(outPath),
                    VBool(false)
                )
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
                local currSess = DownloadsModel:newSession()
                local newId = objRetainer:newId()

                local handler = ctx:makeLuaMatchHandler(
                    VMatch(function(natPack,val)
                        local values = val:values()
                        local dl = currSess:keyDownload(values._2)
                        -- dead progress update
                        if (nil ~= dl) then
                            local done = values._3
                            local total = values._4
                            DownloadsModel:incRevision()
                            dl:setProgress(done,total)
                        end
                        local newBytes = values._5
                        downloadSpeedChecker:regBytes(newBytes)
                    end,"SLD_OutProgressUpdate","int","double","double","double"),
                    VMatch(function(natpack,val)
                        local valTree = val:values()
                        local newKey = valTree._2
                        local newPath = valTree._3
                        DownloadsModel:incRevision()
                        currSess:addDownload(newKey,newPath)
                    end,"SLD_OutStarted","int","string"),
                    VMatch(function(natpack,val)
                        local valTree = val:values()
                        local delKey = valTree._2
                        DownloadsModel:incRevision()
                        currSess:removeDownload(delKey)
                    end,"SLD_OutSingleDone","int"),
                    VMatch(function()
                        print('Downloaded!')
                        currentSessions[thePath] = nil
                        --DownloadsModel:incRevision()
                        --DownloadsModel:dropSession(currSess)
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
            local menuModel = nil

            local currentEntityId, isDir = getCurrentEntityId()
            if (currentEntityId > 0 and isDir) then
                menuModel = { "New directory", "Move directory", "Delete directory", "Rename directory", "New file" }
                if (currentEntityId == 1) then
                    -- root is unmovable, unrenamable and undeletable
                    menuModel = { "New directory", "New file" }
                end
                --"Download directory",
                -- TODO: implement download directory
            elseif (currentEntityId > 0 and not isDir) then
                menuModel = { "Edit file", "Delete file", "Move file" }
                --"Download file",
                -- TODO: localize labels not to depend on them
                -- TODO: implement download
            else
                return
            end

            local menuModelHandler = makePopupMenuModel(
                ctx,menuModel,
                function(result)
                    arraySwitch(result+1,menuModel,
                        arrayBranch("Move directory",function()
                            currentDirToMoveId = getCurrentEntityId()
                            if (currentDirToMoveId ~= -1) then
                                ctx:message(mainWnd,
                                    VSig("MWI_InSetStatusText"),
                                    VString("Press on node under which to move"))
                                shouldMoveDir = true
                            end
                        end),
                        arrayBranch("Delete directory",function()
                            currentDirId = getCurrentEntityId()
                            if (currentDirId ~= -1) then
                                if (currentDirId == 1) then
                                    setStatus(ctx,mainWnd,"Root cannot be deleted.")
                                    return
                                end
                                local asyncSqlite = currentAsyncSqlite
                                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                    return
                                end
                                local wholeDir = whole(currentDirId)
                                -- TODO: ask if really want to delete
                                ctx:messageAsync(asyncSqlite,
                                    VSig("ASQL_Execute"),
                                    VString(sqlDeleteDirectoryRecursively(wholeDir)))
                                ctx:message(mainWnd,VSig("MWI_InDeleteSelectedDir"))
                                currentDirId = -1
                                updateRevision()
                            else
                                setStatus(ctx,mainWnd,"No directory selected.")
                            end
                        end),
                        arrayBranch("Rename directory",function()
                            local dialog = ctx:namedMessageable("singleInputDialog")

                            local showOrHide = function(val)
                                ctx:message(dialog,VSig("INDLG_InShowDialog"),VBool(val))
                            end

                            local dirName = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
                            local dirId = getCurrentEntityId()

                            if (dirName == "[unselected]") then
                                setStatus(ctx,mainWnd,"No directory was selected to create new one.")
                                return
                            end

                            if (dirName == "root" and dirId == 1) then
                                setStatus(ctx,mainWnd,"Cannot rename root.")
                                return
                            end

                            ctx:message(dialog,VSig("INDLG_InSetLabel"),VString(
                                "Specify new folder name to rename  " .. dirName .. "."
                            ))

                            ctx:message(dialog,VSig("INDLG_InSetValue"),VString(dirName))

                            local handler = ctx:makeLuaMatchHandler(
                                VMatch(function()
                                    print("Ok renamed!")
                                    local outName = ctx:messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                                    -- more thorough user input check should be performed
                                    if (outName == "") then
                                        setStatus(ctx,mainWnd,"Some directory name must be specified.")
                                        return
                                    end

                                    local wholeDir = whole(dirId)

                                    local asyncSqlite = currentAsyncSqlite
                                    if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                        return
                                    end
                                    local mainWnd = ctx:namedMessageable("mainWindow")
                                    local mainModel = ctx:namedMessageable("mainModel")
                                    ctx:messageAsyncWCallback(
                                        asyncSqlite,
                                        function(output)
                                            local val = output:values()
                                            local affected = val._3
                                            if (affected > 0) then
                                                ctx:message(mainWnd,
                                                    VSig("MWI_InSetCurrentDirName"),
                                                    VString(outName))
                                            else
                                                messageBox(
                                                    "Duplicate name!",
                                                    "'" .. outName ..
                                                    "' already exists under current parent."
                                                )
                                            end
                                        end,
                                        VSig("ASQL_OutAffected"),
                                        VString(sqlUpdateDirectoryNameStatement(wholeDir,outName)),
                                        VInt(-1)
                                    )
                                    showOrHide(false)
                                end,"INDLG_OutOkClicked"),
                                VMatch(function()
                                    print("Cancel rename!")
                                    showOrHide(false)
                                end,"INDLG_OutCancelClicked")
                            )

                            ctx:message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
                            showOrHide(true)
                        end),
                        arrayBranch("New directory",function()
                            local dialog = ctx:namedMessageable("singleInputDialog")

                            local showOrHide = function(val)
                                ctx:message(dialog,VSig("INDLG_InShowDialog"),VBool(val))
                            end

                            local setDlgErr = function(val)
                                ctx:message(dialog,
                                    VSig("INDLG_InSetErrLabel"),VString(val))
                            end

                            ctx:message(dialog,
                                VSig("INDLG_InSetParent"),
                                VMsg(mainWnd))

                            local dirName = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
                            local dirId = getCurrentEntityId()
                            local dirIdWhole = whole(dirId)

                            if (dirName == "[unselected]") then
                                setStatus(ctx,mainWnd,"No directory was selected to create new one.")
                                return
                            end

                            ctx:message(dialog,VSig("INDLG_InSetLabel"),VString(
                                "Specify new folder name to create under " .. dirName .. "."
                            ))

                            local newId = objRetainer:newId()

                            local handler = ctx:makeLuaMatchHandler(
                                VMatch(function()
                                    print("Ok!")
                                    local outName = ctx:messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                                    -- more thorough user input check should be performed
                                    if (outName == "") then
                                        setDlgErr("Some directory name must be specified.")
                                        return
                                    end

                                    if (not isValidFilename(outName)) then
                                        setDlgErr("Directory name entered contains invalid characters.")
                                        return
                                    end

                                    setDlgErr("")

                                    local asyncSqlite = currentAsyncSqlite
                                    if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                        return
                                    end
                                    local mainWnd = ctx:namedMessageable("mainWindow")
                                    local mainModel = ctx:namedMessageable("mainModel")

                                    local theQuery = sqlNewDirectoryStatement(dirIdWhole,outName)

                                    ctx:messageAsyncWCallback(
                                        asyncSqlite,
                                        function(res)
                                            local num = res:values()._3
                                            if (num > 0) then
                                                ctx:messageAsyncWCallback(
                                                    asyncSqlite,
                                                    function(back)
                                                        local newId = back:values()._3
                                                        ctx:message(mainWnd,
                                                            VSig("MWI_InAddChildUnderCurrentDir"),
                                                            VString(outName),VInt(newId))
                                                    end,
                                                    VSig("ASQL_OutSingleNum"),
                                                    VString("SELECT dir_id FROM"
                                                        .. " directories WHERE"
                                                        .. " rowid=last_insert_rowid();"),
                                                    VInt(-1),
                                                    VBool(false)
                                                )
                                                updateRevision()
                                            else
                                                messageBox(
                                                    "Duplicate name!",
                                                    "'" .. outName ..
                                                    "' already exists under current directory."
                                                )
                                            end
                                        end,
                                        VSig("ASQL_OutAffected"),
                                        VString(theQuery),
                                        VInt(-1)
                                    )
                                    -- todo: optimize, don't reload all
                                    showOrHide(false)
                                    objRetainer:release(newId)
                                end,"INDLG_OutOkClicked"),
                                VMatch(function()
                                    print("Cancel!")
                                    showOrHide(false)
                                    objRetainer:release(newId)
                                end,"INDLG_OutCancelClicked")
                            )

                            objRetainer:retain(newId,handler)

                            ctx:message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
                            showOrHide(true)
                        end),
                        arrayBranch("New file",function()
                            print("New file clicked")
                            newFileDialog(
                                function(result,dialog)
                                    local firstValidation =
                                        validateNewFileDialogFirst(result,dialog)
                                    if (not firstValidation) then
                                        return
                                    end

                                    -- great success, form validation passed
                                    addNewFileUnderCurrentDir(result,dialog)
                                end
                            )
                        end),
                        arrayBranch("Edit file",function()
                            local dirId = getCurrentFileParent()
                            modifyFileDialog(
                                currentEntityId,
                                function(result,orig,dialog)
                                    local firstValidation =
                                        validateNewFileDialogFirst(result,dialog)
                                    if (not firstValidation) then
                                        return
                                    end

                                    updateFileFromDiff(currentEntityId,dirId,result,orig,dialog)
                                end
                            )
                        end),
                        arrayBranch("Move file",function()
                            setStatus(ctx,mainWnd,"Select folder to move file to.")
                            fileToMove = currentEntityId
                            shouldMoveFile = true
                        end),
                        arrayBranch("Delete file",function()
                            local currentFileId = getCurrentEntityId()
                            -- we know that this is file because
                            -- we wouldn't see this menu
                            -- TODO: ask if really want to delete
                            if (currentFileId ~= -1) then
                                local asyncSqlite = currentAsyncSqlite
                                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                    return
                                end
                                ctx:messageAsync(asyncSqlite,
                                    VSig("ASQL_Execute"),
                                    VString("DELETE FROM files WHERE file_id=" .. whole(currentFileId) .. ";"))
                                ctx:message(mainWnd,VSig("MWI_InDeleteSelectedDir"))
                                updateRevision()
                            else
                                setStatus(ctx,mainWnd,"No directory selected.")
                            end
                        end)
                    )
                end
            )
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

            local sess = DownloadsModel:nthSession(sessNum)
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
            natPack:setSlot(2,VInt( DownloadsModel:sessionCount() ))
        end,"DLMDL_QueryCount","int"),
        VMatch(function(natPack,vtree)
            local sessN = vtree:values()._2 + 1
            local sess = DownloadsModel:nthSession(sessN)
            local count = sess:activeDownloadCount()
            natPack:setSlot(3,VInt(count))
        end,"DLMDL_QuerySessionDownloadCount","int","int"),
        VMatch(function(natPack,vtree)
            local sessN = vtree:values()._2 + 1
            local theLabel = "Session #" ..
                DownloadsModel:nthSessionNum(sessN)
            natPack:setSlot(3,VString(theLabel))
        end,"DLMDL_QuerySessionTitle","int","string"),
        VMatch(function(natPack,vtree)
            local sessN = vtree:values()._2 + 1
            local sess = DownloadsModel:nthSession(sessN)
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
            local sess = DownloadsModel:nthSession(sessN)
            local consumed = sess:consumeLog()
            natPack:setSlot(3,VString(consumed))
        end,"DLMDL_QuerySessionLog","int","string")
    )
    ctx:message(mainWnd,VSig("MWI_InSetDownloadModel"),VMsg(downloadUpdateModel))

end
initAll()
initAll = nil
