
JSON = require('lua/JSON')

PersistentSettings = {
    new = function(ioOps,filename,interval)
        local updInt = interval
        if (nil == updInt) then
            updInt = 7
        end
        local output = {
            revision = 0,
            saveRevision = 0,
            io = ioOps,
            settingsfile = filename,
            updateinterval = updInt,
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

}
