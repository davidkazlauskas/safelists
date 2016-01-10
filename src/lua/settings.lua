
JSON = require('lua/JSON')

PersistentSettings = {
    new = function(reader,filename)
        local output = {
            revision = 0,
            saveRevision = 0,
            settings = {}
        }
    end
}

PersistentSettings.mt = {
    setValue = function(self,key,val)

    end
}
