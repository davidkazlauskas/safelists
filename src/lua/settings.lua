
JSON = require('lua/JSON')

PersistentSettings = {
    new = function(reader,filename)
        local output = {
            revision = 0,
            saveRevision = 0,
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
