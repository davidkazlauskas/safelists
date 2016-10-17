
SingleDownload = {
    __index = {
        newDownload = function(id,path)
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

DownloadsModel = {
    __index = {
        enumerateSessions = function(self)
            self.enumerated = enumerateTable(self.sessions)
        end,
        dropSession = function(self,sess)
            self.sessions[sess.key] = nil
            self.progressTotal = self.progressTotal + 1
            self:enumerateSessions()
        end,
        totalProgress = function(self)
            if (self.progressTotal == 0) then
                return self.progressTotal
            end
            return self.progressDone / self.progressTotal
        end,
        sessionCount = function(self)
            return #self.enumerated
        end,
        nthSession = function(self,num)
            return self.enumerated[num]
        end,
        nthSessionNum = function(self,num)
            return self.enumerated[num].key
        end,
        incRevision = function(self)
            self.revisionNum = self.revisionNum + 1
        end,
        tagUpdate = function(self)
            self.revisionUpdateNum = self.revisionNum
        end,
        isDirty = function(self)
            return self.revisionUpdateNum ~= self.revisionNum
        end,
        newSession = function(self)
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
    },
    newDownloadsModel = function()
        local res = {
            sessions = {},
            enumerated = {},
            progressTotal = 0,
            progressDone = 0,
            currentSession = 0,
            revisionNum = 0,
            revisionUpdateNum = 0
        }
        setmetatable(res,DownloadsModel)
        return res
    end
}
