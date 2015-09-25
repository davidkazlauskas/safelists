
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
        keyDownload = function(self,key)
            return self.downloadTable[key]
        end
    }
}

function DownloadsModel:newSession()
    self.currentSession = self.currentSession + 1
    local theSession = self.currentSession
    local res = {
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
    self:enumerateSessions()
end

function DownloadsModel:sessionCount()
    return #self.enumerated
end

function DownloadsModel:nthSession(num)
    return self.enumerated[num]
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
        print('Update fired!')
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
            print('Draw ended!')
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
                local outRes = ctx:messageRetValues(mainWnd,VSig("MWI_InMoveChildUnderParent"),VInt(-1))._2
                if (outRes == 1) then
                    setStatus(ctx,mainWnd,"Directory to move is parent of selected directory.")
                    return
                elseif (outRes == 2) then
                    setStatus(ctx,mainWnd,"Directories are the same.")
                    return
                elseif (outRes ~= 0) then
                    setStatus(ctx,mainWnd,"Something bad happened...")
                    return
                end

                ctx:messageAsync(asyncSqlite,
                    VSig("ASQL_Execute"),
                    VString("UPDATE directories SET dir_parent=" .. inId
                        .. " WHERE dir_id=" .. currentDirId .. ";"))
                    currentDirId = -1
                updateRevision()
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

            local currSess = DownloadsModel:newSession()
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
                    print('Starting... ' .. valTree._2)
                    DownloadsModel:incRevision()
                    currSess:addDownload(newKey,newPath)
                end,"SLD_OutStarted","int","string"),
                VMatch(function(natpack,val)
                    local valTree = val:values()
                    local delKey = valTree._2
                    print('Ended! ' .. delKey)
                    DownloadsModel:incRevision()
                    currSess:removeDownload(delKey)
                end,"SLD_OutSingleDone","int"),
                VMatch(function()
                    print('Downloaded!')
                    DownloadsModel:incRevision()
                    DownloadsModel:dropSession(currSess)
                end,"SLD_OutDone"),
                VMatch(function(natPack,val)
                    local values = val:values()
                    local hash = values._3
                    local asyncSqlite = currentAsyncSqlite
                    print("New hash update: |" .. hash .. "|")
                    if (nil == asyncSqlite) then
                        return
                    end

                    local id = values._2

                    ctx:messageAsyncWCallback(asyncSqlite,
                        function (out)
                            local outVal = out:values()

                            print("Queried hash: " .. id .. " -> " .. outVal._3)

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
                VMatch(function()
                    print('Safelist session dun! Downloading...')
                    local dlHandle = ctx:messageRetValues(dlFactory,
                        VSig("SLDF_InNewAsync"),
                        VString(downloadPath),
                        VMsg(currDlSessionHandler),
                        VMsg(nil)
                    )._4
                    assert( dlHandle ~= nil )
                end,"SLDF_OutCreateSessionDone")
            )

            currDlSessionHandler = handler
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
                local factory = ctx:namedMessageable("asyncSqliteFactory")
                local mainModel = ctx:namedMessageable("mainModel")

                currentAsyncSqlite = ctx:messageRetValues(factory,
                    VSig("ASQLF_CreateNew"),
                    VString(outPath),VMsg(nil))._3

                ctx:message(mainModel,
                    VSig("MMI_InLoadFolderTree"),
                    VMsg(currentAsyncSqlite),VMsg(mainWnd))
                onSafelistState()
                updateRevision()
            end
            print('button blast!')
        end,"MWI_OutOpenSafelistButtonClicked"),
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
                            print("Selected dir: " .. currentDirId)
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
                            assert( false , "Rename not implemented cholo" )
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
                                    ctx:messageAsync(
                                        asyncSqlite,
                                        VSig("ASQL_Execute"),
                                        VString("INSERT INTO directories (dir_name,dir_parent)"
                                            .. " VALUES ('" .. outName .. "'," .. dirId .. ");")
                                    )
                                    -- todo: optimize, don't reload all
                                    ctx:message(mainModel,
                                        VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
                                        updateRevision()
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
        end,"DLMDL_QuerySessionDownloadCount","int","int")
    )
    ctx:message(mainWnd,VSig("MWI_InSetDownloadModel"),VMsg(downloadUpdateModel))

end
initAll()
initAll = nil
