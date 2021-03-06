
JSON = require('JSON')

function JSON:onDecodeError(message,text,location,etc)
    -- http://replygif.net/i/828.gif
end

PersistentSettings = {
    -- saveFunction - (to call with json when saving)
    -- loadFunction - (called once in creation with function
    -- to update the object with json)
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
        --print("About to load settings...")
        loadFunction(function(outString)
            --print("Load function entered: |" .. outString .. "|")
            if (type(outString) == "string") then
                local decoded = JSON:decode(outString)
                if (decoded ~= nil and type(decoded) == "table") then
                    output.settings = decoded
                    --print("load success")
                end
            end
        end)
        return output
    end
}

PersistentSettings.mt = {
    __index = {
        setValue = function(self,key,val)
            if (self.settings[key] ~= val) then
                --print("changing settings")
                self.revision = self.revision + 1
                self.settings[key] = val
            end
        end,
        getValue = function(self,key)
            return self.settings[key]
        end,
        getValueDefault = function(self,key,defval)
            local first = self:getValue(key)
            if (first ~= nil) then
                return first
            else
                return defval
            end
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
            if (currTime - self.lastSave > self.updateinterval)
            then
                local toSave = JSON:encode_pretty(self.settings)
                self.lastSave = currTime
                self.saveRevision = self.revision
                --print("Persisting settings...")
                self.saveFunction(toSave)
            end
        end
    }
}
