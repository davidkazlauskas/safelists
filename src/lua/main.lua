
require('lua/mobdebug').start()

setStatus = function(context,widget,text)
    context:message(widget,VSig("MWI_InSetStatusText"),VString(text))
end

function enumerateTable(table)
    local res = {}
    local index = 0
    for k,v in pairs(table) do
        index = index + 1
        res[index] = v
    end
    return res
end

function byteBelongsToHex(char)
    if (c >= 48 and c <= 57) then
        return true
    end

    if (c >= 65 and c <= 70) then
        return true
    end

    if (c >= 97 and c <= 102) then
        return true
    end

    return false
end

function isValidDumbHash256(string)
    if (#string ~= 64) then
        return false
    end

    for i = 1, #string do
        local c = string.byte(str:sub(i,i))
        if (not byteBelongsToHex(c)) then
            return false
        end
    end

    return true
end

-- LUA HAS NO SPLIT STRING FUNCTION? ARE YOU SERIOUS?
function string:split(delimiter)
  local result = { }
  local from = 1
  local delim_from, delim_to = string.find( self, delimiter, from )
  while delim_from do
    table.insert( result, string.sub( self, from , delim_from-1 ) )
    from = delim_to + 1
    delim_from, delim_to = string.find( self, delimiter, from )
  end
  table.insert( result, string.sub( self, from ) )
  return result
end

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

function arrayBranch(value,func)
    return {
        value=value,
        func=func
    }
end

function arraySwitch(value,table,...)
    local branches = {...}
    local toFind = table[value]
    for k,v in pairs(branches) do
        if v.value == toFind then
            return v.func()
        end
    end
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
                progress=0,
                speed=0 -- bytes/sec
            }
            setmetatable(res,SingleDownload)
            return res
        end,
        getProgress = function(self)
            return self.progress
        end,
        getPath = function(self)
            return self.filePath
        end,
        setProgress = function(self,progress)
            self.progress = progress
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
        key = theSession
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
    local wgt = sessionWidget
    if (nil == wgt) then
        return
    end

    if (DownloadsModel:isDirty()) then
        --print('Update fired!')
        local ctx = luaContext()
        ctx:message(wgt,
            VSig("DLMDL_InFullUpdate"))
    end
end

function setWidgetsEnabled(context,wnd,value,...)
    local values = {...}
    for k, v in pairs(values) do
        context:message(wnd,
            VSig("MWI_InSetWidgetEnabled"),
            VString(v),
            VBool(value)
        )
    end
end

initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMessageable("mainWindow")

    local safelistDependantWigets = {
        "dirList",
        "fileList",
        "downloadButton"
    }

    local noSafelistState = function()
        setWidgetsEnabled(
            ctx,mainWnd,
            false,
            unpack(safelistDependantWigets)
        )
    end

    local onSafelistState = function()
        setWidgetsEnabled(
            ctx,mainWnd,
            true,
            unpack(safelistDependantWigets)
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
            VSig("ASQL_OutSingleRow"),VString(
                "SELECT revision_number,datetime(modification_date,'unixepoch','localtime')" ..
                " FROM metadata;"
            ),VString("empty"),VBool(false))
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

    local messageBox = function(title,message)
        local dialogService =
            ctx:namedMessageable("dialogService")
        ctx:message(
            dialogService,
            VSig("GDS_AlertDialog"),
            VMsg(mainWnd),
            VString(title),
            VString(message))
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

        local isHidden = false
        local hideDlg = function()
            if (not isHidden) then
                isHidden = true
                ctx:message(
                    dialog,
                    VSig("INDLG_InHideDialog")
                )
            end
        end

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

                    if (funcSuccess(outResult)) then
                        hideDlg()
                    end
                elseif (signal == hookedCancel) then
                    print("Cancel clicked")
                    hideDlg()
                else
                    assert( false, "No such signal? " .. signal )
                end
            end,"INDLG_OutGenSignalEmitted","int"),
            VMatch(function()
                print("Exit vanilla")
                hideDlg()
            end,"INDLG_OutDialogExited")
        )

        ctx:message(
            dialog,
            VSig("INDLG_InSetNotifier"),
            VMsg(handler)
        )

        ctx:message(
            dialog,
            VSig("INDLG_InShowDialog")
        )
    end

    table.insert(FrameEndFunctions,updateRevisionGui)
    table.insert(FrameEndFunctions,updateSessionWidget)

    noSafelistState()

    ctx:attachContextTo(mainWnd)
    sessionWidget = ctx:messageRetValues(mainWnd,
        VSig("MWI_QueryDownloadSessionWidget"),VMsg(nil))._2

    currentDirId = -1

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
        VMatch(function()
            for k,v in ipairs(FrameEndFunctions) do
                v()
            end
            --print('Draw ended!')
        end,"MWI_OutDrawEnd"),
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

            if (currentDirId > 0 and shouldMoveDir == true) then
                shouldMoveDir = false
                ctx:message(mainWnd,VSig("MWI_InSetStatusText"),VString(""))
                if (inId == currentDirId) then
                    return
                end

                local asyncSqlite = currentAsyncSqlite
                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                    return
                end
                local mainWnd = ctx:namedMessageable("mainWindow")
                local mainModel = ctx:namedMessageable("mainModel")

                local condition =
                       " SELECT CASE"
                    --.. " WHEN (" .. currentDirId .. " IN"
                    --.. " ("
                    --.. "     WITH RECURSIVE"
                    --.. "     children(d_id) AS ("
                    --.. "           SELECT dir_id FROM directories "
                    --.. "               WHERE dir_parent=" .. inId
                    --.. "           UNION ALL"
                    --.. "           SELECT dir_id"
                    --.. "           FROM directories JOIN children ON "
                    --.. "              directories.dir_parent=children.d_id "
                    --.. "     ) SELECT d_id FROM children"
                    --.. " )) THEN 1"
                    .. " WHEN ((SELECT dir_parent FROM directories"
                    .. "     WHERE dir_id=" .. currentDirId
                    .. "     ) = " .. inId .. ") THEN 3"
                    .. " WHEN (" .. "(SELECT dir_name FROM"
                    .. "     directories WHERE dir_id=" .. currentDirId .. ") IN"
                    .. "     ( SELECT dir_name FROM directories WHERE"
                    .. "     dir_parent=" .. inId .. ")) THEN 2"
                    .. " ELSE 0"
                    .. " END;"

                ctx:messageAsyncWCallback(
                    asyncSqlite,
                    function(outres)
                        local table = outres:values()
                        local value = table._3
                        local success = table._4
                        assert( success, "Great success failed..." )
                        if (value == 0) then
                            ctx:messageAsync(asyncSqlite,
                                VSig("ASQL_OutAffected"),
                                VString("UPDATE directories SET dir_parent="
                                    .. inId .. " WHERE dir_id=" .. currentDirId .. ";"),
                                VInt(-1))
                            currentDirId = -1
                            ctx:message(mainWnd,
                                VSig("MWI_InMoveChildUnderParent"),
                                VInt(-1))
                            updateRevision()
                        elseif (value == 1) then
                            messageBox(
                                "Cannot move!",
                                "Directory to move cannot be a parent"
                                .. " of directory to move under."
                            )
                        --elseif (value == 2) then
                            --local dialogService =
                                --ctx:namedMessageable("dialogService")
                            --ctx:message(
                                --dialogService,
                                --VSig("GDS_AlertDialog"),
                                --VMsg(mainWnd),
                                --VString("Cannot move!"),
                                --VString("Parent directory already has"
                                    --.. " directory with such name."))
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

            local mainModel = ctx:namedMessageable("mainModel")
            local asyncSqlite = currentAsyncSqlite
            if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                return
            end
            ctx:message(mainModel,
                VSig("MMI_InLoadFileList"),VInt(inId),
                VMsg(asyncSqlite),VMsg(mainWnd))
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

            local outVal = ctx:messageRetValues(dialogService,
                VSig("GDS_DirChooserDialog"),
                VMsg(mainWnd),
                VString("Select download location."),
                VString(""))

            local downloadPath = outVal._4
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
            local newId = objRetainer:newId()
            local handlerWeak = nil
            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local values = val:values()
                    local dl = currSess:keyDownload(values._2)
                    local ratio = values._3 / values._4
                    DownloadsModel:incRevision()
                    dl:setProgress(ratio)
                end,"SLD_OutProgressUpdate","int","double","double"),
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
                    currentSessions[downloadPath] = nil
                    DownloadsModel:incRevision()
                    DownloadsModel:dropSession(currSess)
                    objRetainer:release(newId)
                end,"SLD_OutDone"),
                VMatch(function(natPack,val)
                    local values = val:values()
                    local hash = values._3
                    local asyncSqlite = currentAsyncSqlite
                    if (nil == asyncSqlite) then
                        return
                    end

                    local id = values._2

                    ctx:messageAsyncWCallback(asyncSqlite,
                        function (out)
                            local outVal = out:values()

                            assert( outVal._4, "Query failed..." )
                            assert( outVal._3 == "", "Hash collision, hash is different."
                                .. " (todo: handle this case)" )

                            ctx:messageAsync(
                                asyncSqlite,
                                VSig("ASQL_Execute"),
                                VString("UPDATE files SET file_hash_256='"
                                    .. hash .. "' WHERE file_id='" .. id .. "';")
                            )
                            updateRevision()
                        end,
                        VSig("ASQL_OutSingleRow"),
                        VString("SELECT file_hash_256 FROM files WHERE file_id='"
                            .. id .. "';"),
                        VString(""),
                        VBool(false))
                end,"SLD_OutHashUpdate","int","string"),
                VMatch(function(natPack,out)
                    local val = out:values()
                    local asyncSqlite = currentAsyncSqlite
                    local id = val._2
                    local newSize = val._3
                    -- size collision already checked with assert
                    ctx:messageAsync(
                        asyncSqlite,
                        VSig("ASQL_Execute"),
                        VString("UPDATE files SET file_size='"
                            .. newSize .. "' WHERE file_id='" .. id .. "';")
                    )
                    updateRevision()
                end,"SLD_OutSizeUpdate","int","double"),
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
                end,"SLDF_OutTotalDownloads","int")
            )

            handlerWeak = handler:getWeak()
            objRetainer:retain(newId,handler)

            ctx:message(dlFactory,
                VSig("SLDF_CreateSession"),
                VMsg(asyncSqlite),
                VMsg(handler),
                VString(downloadPath)
            )
        end,"MWI_OutDownloadSafelistButtonClicked"),
        VMatch(function()
            local dialogService = ctx:namedMessageable("dialogService")
            local outVal = ctx:messageRetValues(dialogService,
                VSig("GDS_FileChooserDialog"),
                VMsg(mainWnd),
                VString("Select safelist to open."),
                VString("*.safelist"),
                VString(""))

            local outPath = outVal._5
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
        end,"MWI_OutOpenSafelistButtonClicked"),
        VMatch(function()

            local dialogService = ctx:namedMessageable("dialogService")
            local outVal = ctx:messageRetValues(dialogService,
                VSig("GDS_FileChooserDialog"),
                VMsg(mainWnd),
                VString("Select safelist session to resume."),
                VString("safelist_session"),
                VString("")
            )

            local thePath = outVal._5

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
                    local ratio = values._3 / values._4
                    DownloadsModel:incRevision()
                    dl:setProgress(ratio)
                end,"SLD_OutProgressUpdate","int","double","double"),
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
                    DownloadsModel:incRevision()
                    DownloadsModel:dropSession(currSess)
                    objRetainer:release(newId)
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
                end,"SLDF_OutTotalDownloads","int")
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
            local menuModel = { "New directory", "Move", "Delete", "Rename" }
            local menuModelHandler = makePopupMenuModel(
                ctx,menuModel,
                function(result)
                    arraySwitch(result+1,menuModel,
                        arrayBranch("Move",function()
                            currentDirId = ctx:messageRetValues(mainWnd,
                                VSig("MWI_QueryCurrentDirId"),VInt(-7))._2
                            if (currentDirId ~= -1) then
                                ctx:message(mainWnd,
                                    VSig("MWI_InSetStatusText"),
                                    VString("Press on node under which to move"))
                                shouldMoveDir = true
                            end
                        end),
                        arrayBranch("Delete",function()
                            currentDirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2
                            if (currentDirId ~= -1) then
                                if (currentDirId == 1) then
                                    setStatus(ctx,mainWnd,"Root cannot be deleted.")
                                    return
                                end
                                local asyncSqlite = currentAsyncSqlite
                                if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                    return
                                end
                                ctx:messageAsync(asyncSqlite,
                                    VSig("ASQL_Execute"),
                                    VString("DELETE FROM directories WHERE dir_id=" .. currentDirId .. ";"))
                                ctx:message(mainWnd,VSig("MWI_InDeleteSelectedDir"))
                                currentDirId = -1
                                updateRevision()
                            else
                                setStatus(ctx,mainWnd,"No directory selected.")
                            end
                        end),
                        arrayBranch("Rename",function()
                            local dialog = ctx:namedMessageable("singleInputDialog")

                            local showOrHide = function(val)
                                ctx:message(dialog,VSig("INDLG_InShowDialog"),VBool(val))
                            end

                            local dirName = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
                            local dirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2

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
                                        VString("UPDATE directories SET dir_name='" .. outName .. "'"
                                            .. " WHERE dir_id=" .. dirId .. " AND NOT EXISTS("
                                            .. " SELECT 1 FROM directories WHERE dir_name='".. outName
                                            .. "' AND dir_parent=(SELECT dir_parent FROM directories "
                                            .. " WHERE dir_id=" .. dirId .. " )" .. ");"
                                        ),
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

                            local dirName = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirName"),VString("?"))._2
                            local dirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2

                            if (dirName == "[unselected]") then
                                setStatus(ctx,mainWnd,"No directory was selected to create new one.")
                                return
                            end

                            ctx:message(dialog,VSig("INDLG_InSetLabel"),VString(
                                "Specify new folder name to create under " .. dirName .. "."
                            ))

                            local handler = ctx:makeLuaMatchHandler(
                                VMatch(function()
                                    print("Ok!")
                                    local outName = ctx:messageRetValues(dialog,VSig("INDLG_QueryInput"),VString("?"))._2
                                    -- more thorough user input check should be performed
                                    if (outName == "") then
                                        setStatus(ctx,mainWnd,"Some directory name must be specified.")
                                        return
                                    end

                                    local asyncSqlite = currentAsyncSqlite
                                    if (messageablesEqual(VMsgNil(),asyncSqlite)) then
                                        return
                                    end
                                    local mainWnd = ctx:namedMessageable("mainWindow")
                                    local mainModel = ctx:namedMessageable("mainModel")

                                    local theQuery =
                                           "INSERT INTO directories (dir_name,dir_parent)"
                                        .. " SELECT '" .. outName .. "', " .. dirId
                                        .. " WHERE NOT EXISTS("
                                        .. " SELECT 1 FROM directories WHERE dir_name='".. outName
                                        .. "' AND dir_parent=" .. dirId .. ");"

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
                                end,"INDLG_OutOkClicked"),
                                VMatch(function()
                                    print("Cancel!")
                                    showOrHide(false)
                                end,"INDLG_OutCancelClicked")
                            )

                            ctx:message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
                            showOrHide(true)
                        end)
                    )
                end
            )
            ctx:message(mainWnd,VSig("MWI_PMM_ShowMenu"),VMsg(menuModelHandler))
        end,"MWI_OutRightClickFolderList"),
        VMatch(function()
            print("Right files")
            local menuModel = { "New file" }
            local menuModelHandler = makePopupMenuModel(
                ctx,menuModel,
                function(result)
                    arraySwitch(result+1,menuModel,
                        arrayBranch("New file",function()
                            print("New file clicked")
                            newFileDialog(
                                function(result)
                                    assert( result.finished, "Should be good..." )
                                end
                            )
                        end)
                    )
                end
            )
            ctx:message(mainWnd,VSig("MWI_PMM_ShowMenu"),VMsg(menuModelHandler))
        end,"MWI_OutRightClickFileList")
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

            natPack:setSlot(4,VString(download:getPath()))
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
            local total = sess:totalDownloads()
            if (total == 0) then
                total = 1
            end
            local prog = done / total
            local progRounded = tonumber(
                string.format("%.2f",prog * 100))
            local theLabel = done .. " out of "
                .. total .. " downloads done (" ..
                progRounded .. "%)"
            natPack:setSlot(3,VString(theLabel))
            natPack:setSlot(4,VDouble(prog))
        end,"DLMDL_QuerySessionTotalProgress","int","string","double")
    )
    ctx:message(mainWnd,VSig("MWI_InSetDownloadModel"),VMsg(downloadUpdateModel))

end
initAll()
initAll = nil
