

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

