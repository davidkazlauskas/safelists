
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

revealDownloads = false
sessionWidget = nil

DownloadsModel = {
    sessions = {},
    enumerated = {},
    progressTotal = 0,
    currentSession = 0
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

function updateSessionWidget()
    local wgt = sessionWidget
    if (nil == wgt) then
        return
    end

    local ctx = luaContext()
    ctx:message(wgt,
        VSig("DLMDL_InFullUpdate"))
end

initAll = function()

    local ctx = luaContext()
    local mainWnd = ctx:namedMesseagable("mainWindow")

    ctx:attachContextTo(mainWnd)
    sessionWidget = ctx:messageRetValues(mainWnd,
        VSig("MWI_QueryDownloadSessionWidget"),VMsg(nil))._2

    currentDirId = -1

    mainWindowPushButtonHandler = ctx:makeLuaMatchHandler(
        VMatch(function()
            updateSessionWidget()
            print('Draw ended!')
        end,"MWI_OutDrawEnd"),
        VMatch(function()
            local mainModel = ctx:namedMesseagable("mainModel")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            ctx:message(mainModel,
                VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
            -- Testing, reveal
            revealDownloads = not revealDownloads
            ctx:message(mainWnd,
                VSig("MWI_InRevealDownloads"),VBool(revealDownloads))
        end,"MWI_OutNewFileSignal"),
        VMatch(function(natpack,val)
            local inId = val:values()._2

            if (currentDirId > 0) then
                ctx:message(mainWnd,VSig("MWI_InSetStatusText"),VString(""))
                if (inId == currentDirId) then
                    return
                end

                local mainWnd = ctx:namedMesseagable("mainWindow")
                local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
                local mainModel = ctx:namedMesseagable("mainModel")
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
                return
            end

            local mainModel = ctx:namedMesseagable("mainModel")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            ctx:message(mainModel,
                VSig("MMI_InLoadFileList"),VInt(inId),
                VMsg(asyncSqlite),VMsg(mainWnd))
        end,"MWI_OutDirChangedSignal","int"),
        VMatch(function()
            currentDirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2
            print("Selected dir: " .. currentDirId)
            if (currentDirId ~= -1) then
                ctx:message(mainWnd,
                    VSig("MWI_InSetStatusText"),
                    VString("Press on node under which to move"))
            end
        end,"MWI_OutMoveButtonClicked"),
        VMatch(function()
            currentDirId = ctx:messageRetValues(mainWnd,VSig("MWI_QueryCurrentDirId"),VInt(-7))._2
            if (currentDirId ~= -1) then
                if (currentDirId == 1) then
                    setStatus(ctx,mainWnd,"Root cannot be deleted.")
                end
                local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
                ctx:messageAsync(asyncSqlite,
                    VSig("ASQL_Execute"),
                    VString("DELETE FROM directories WHERE dir_id=" .. currentDirId .. ";"))
                ctx:message(mainWnd,VSig("MWI_InDeleteSelectedDir"))
                currentDirId = -1
            else
                setStatus(ctx,mainWnd,"No directory selected.")
            end
        end,"MWI_OutDeleteDirButtonClicked"),
        VMatch(function()
            local dialog = ctx:namedMesseagable("singleInputDialog")

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

                    local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
                    local mainWnd = ctx:namedMesseagable("mainWindow")
                    local mainModel = ctx:namedMesseagable("mainModel")
                    ctx:messageAsync(
                        asyncSqlite,
                        VSig("ASQL_Execute"),
                        VString("INSERT INTO directories (dir_name,dir_parent)"
                            .. " VALUES ('" .. outName .. "'," .. dirId .. ");")
                    )
                    -- todo: optimize, don't reload all
                    ctx:message(mainModel,
                        VSig("MMI_InLoadFolderTree"),VMsg(asyncSqlite),VMsg(mainWnd))
                    showOrHide(false)
                end,"INDLG_OutOkClicked"),
                VMatch(function()
                    print("Cancel!")
                    showOrHide(false)
                end,"INDLG_OutCancelClicked")
            )

            ctx:message(dialog,VSig("INDLG_InSetNotifier"),VMsg(handler))
            showOrHide(true)
        end,"MWI_OutNewDirButtonClicked"),
        VMatch(function()
            local dlFactory = ctx:namedMesseagable("dlSessionFactory")
            local asyncSqlite = ctx:namedMesseagable("asyncSqliteCurrent")
            local currSess = DownloadsModel:newSession()
            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local values = val:values()
                    local dl = currSess:keyDownload(values._2)
                    local ratio = values._3 / values._4
                    dl:setProgress(ratio)
                end,"SLD_OutProgressUpdate","int","double","double"),
                VMatch(function(natpack,val)
                    local valTree = val:values()
                    local newKey = valTree._2
                    local newPath = valTree._3
                    print('Starting... ' .. valTree._2)
                    currSess:addDownload(newKey,newPath)
                end,"SLD_OutStarted","int","string"),
                VMatch(function(natpack,val)
                    local valTree = val:values()
                    local delKey = valTree._2
                    print('Ended! ' .. delKey)
                    currSess:removeDownload(delKey)
                end,"SLD_OutSingleDone","int"),
                VMatch(function()
                    print('Downloaded!')
                    DownloadsModel:dropSession(currSess)
                end,"SLD_OutDone"),
                VMatch(function()
                    print('Safelist session dun! Downloading...')
                    local dlHandle = ctx:messageRetValues(dlFactory,
                        VSig("SLDF_InNewAsync"),
                        VString("downloadtest1/safelist_session"),
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
                VString("downloadtest1/safelist_session")
            )
        end,"MWI_OutDownloadSafelistButtonClicked")
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
