
GenericWidget = {
    -- make generic widget from messageable
    putOn = function(strongMsg,context)
        if (nil == context) then
            context = luaContext()
        end
        local res = {
            messageable = strongMsg,
            nodeCache = {},
            luaCtx = context
        }
        setmetatable(res,GenericWidget.mt)
        return res
    end
}

-- GenericWidget metatable
GenericWidget.mt = {
    __index = {
        -- get widget from glade tree according
        -- to it's id name. Returns GenericWidgetNode
        getWidget = function(self,name)
            assert( type(name) == "string",
                "Expected string for name." )

            local res = self.nodeCache[name]
            if (nil == res) then
                local msg = self.luaCtx:messageRetValues(
                    self.messageable,
                    VSig("GWI_GetWidgetFromTree"),
                    VString(name),
                    VMsg(nil)
                )._3

                res = {
                    messageable = msg,
                    luaCtx = self.luaCtx
                }

                setmetatable(res,GenericWidgetNode.mt)

                self.nodeCache[name] = res
            end

            return res
        end
    }
}

GenericWidgetNode = {
    -- no statics yet
}

GenericWidgetNode.mt = {
    __index = {
        -- hook button event. to notify
        -- object will now receive
        -- "GWI_GBT_OutClickEvent" with
        -- the integer returned.
        hookButtonClick = function(self)
            return self.luaCtx:messageRetValues(
                self.messageable,
                VSig("GWI_GBT_HookClickEvent"),
                VInt(-1)
            )._2
        end,
        notebookSwitchTab = function(self,index)
            assert( type(index) == "number", "Number value expected for tab." )
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GNT_SetCurrentTab"),
                VInt(index)
            )
        end
    }
}

