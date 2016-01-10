
JSON = require('lua/JSON')

PersistentSettings = {
    new = function(ioOps,filename)
        local output = {
            revision = 0,
            saveRevision = 0,
            io = ioOps,
            settingsfile = filename,
            settings = {}
        }
        setmetatable(output,PersistentSettings.mt)
        return output
    end
}

PersistentSettings.mt = {
    setValue = function(self,key,val)

    end
}
