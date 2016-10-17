
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
        end
    },
    new = function()
        local res = {
            count = 0,
            table = {}
        }
        setmetatable(res,ObjectRetainer)
        return res
    end
}

-- download speed
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

-- struct that holds current safelist path
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
