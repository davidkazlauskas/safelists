
JSON = require('lua/JSON')

PersistentSettings = {
    -- saveFunction - (to call with json when saving)
    -- loadFunction - (called once in creation)
    new = function(saveFunction,loadFunction,interval)
        local updInt = interval
        if (nil == updInt) then
            updInt = 7
        end
        local output = {
            revision = 0,
            saveRevision = 0,
            saveFunction = saveFunction,
            updateinterval = updInt,
            lastSave = 0,
            settings = {}
        }
        setmetatable(output,PersistentSettings.mt)
        return output
    end
}

PersistentSettings.mt = {
    setValue = function(self,key,val)
        if (self.settings[key] ~= val) then
            self.revision = self.revision + 1
            self.settings[key] = val
        end
    end,
    getValue = function(self,key)
        return self.settings[key]
    end,
    -- persist settings,
    -- called on gui loop.
    -- task of this function is to defer
    -- save until settings stayed unmodified
    -- for a while and then save.
    -- if settings don't change then don't save.
    persist = function(self)
        if (self.revision == self.saveRevision) then
            -- nothing to do
            return
        end

        local currTime = os.time()
        if (currTime - self.lastSave > self.updateinterval) then
            local toSave = JSON:encode_pretty(self.settings)
            self.saveFunction(toSave)
        end
    end
}
