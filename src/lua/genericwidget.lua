
GenericWidget = {
    -- make generic widget from messageable
    putOn = function(strongMsg,context)
        if (nil == context) then
            context = luaContext()
        end
        local res = {
            messageable = strongMsg,
            nodeCache = {},
            luaCtx = context,
            hookedEvents = {},
            hookedEventTypes = {}
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
                    luaCtx = self.luaCtx,
                    parent = self
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
        getMessageable = function(self)
            return self.messageable
        end,
        -- hook button event. to notify
        -- object will now receive
        -- "GWI_GBT_OutClickEvent" with
        -- the integer returned.
        hookButtonClick = function(self,theFunction)
            local theId = self.luaCtx:messageRetValues(
                self.messageable,
                VSig("GWI_GBT_HookClickEvent"),
                VInt(-1)
            )._2
            local theHandler =
                self.luaCtx:makeLuaMatchHandler(
                    VMatch(function(natpack,val)
                        local theId = val:values()._2
                        self.parent.hookedEvents[theId].routine()
                    end,"GWI_GBT_OutClickEvent","int")
                )

            self.parent.hookedEvents[theId] = {
                routine = theFunction,
                handler = theHandler
            }

            if (self.parent.hookedEventTypes["singleclick"] == nil) then
                self.luaCtx:message(
                    self.parent.messageable,
                    VSig("GWI_SetNotifier"),
                    theHandler
                )
                self.parent.hookedEventTypes["singleclick"] = true
            end
            return theId
        end,
        notebookSwitchTab = function(self,index)
            assert( type(index) == "number", "Number value expected for tab." )
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GNT_SetCurrentTab"),
                VInt(index)
            )
        end,
        setVisible = function(self,value)
            assert( type(value) == "boolean", "True or false expected for visibility." )
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GWT_SetVisible"),
                VBool(value)
            )
        end,
        labelSetText = function(self,value)
            assert( type(value) == "string", "Text expected in set text." )
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GLT_SetValue"),
                VString(value)
            )
        end,
        buttonSetText = function(self,value)
            assert( type(value) == "string", "Text expected in set button text." )
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GBT_SetButtonText"),
                VString(value)
            )
        end,
        windowSetPosition = function(self,value)
            assert( type(value) == "string", "Position should be specified in enum string." )
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GWNT_SetWindowPosition"),
                VString(value)
            )
        end,
        windowSetTitle = function(self,value)
            assert( type(value) == "string", "Position should be specified in enum string." )
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GWNT_SetWindowTitle"),
                VString(value)
            )
        end,
        windowSetParent = function(self,msg)
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GWNT_SetWindowParent"),
                VMsg(msg)
            )
        end,
        menuBarSetModelStackless = function(self,msg)
            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GMIT_SetModelStackless"),
                VMsg(msg)
            )
        end
    }
}

MenuModel = {
    new = function()
        local res = {
            callbackCount = 1,
            callbacks = {},
            data = nil
        }
        res.data = MenuModel.MenuTree.newComp(res,"ROOT","ROOT")
        setmetatable(res,MenuModel.mt)
        return res
    end,
    MenuTree = {
        newComp = function(menModel,shortname,title)
            local res = {
                model = menModel,
                isLeaf = false,
                shortname = shortname,
                title = title,
                num = nil,
                data = {}
            }
            setmetatable(res,MenuModel.MenuTree.mt)
            return res
        end,
        newLeaf = function(menModel,shortname,title,num)
            local res = {
                model = menModel,
                isLeaf = true,
                shortname = shortname,
                title = title,
                num = num,
                data = nil
            }
            setmetatable(res,MenuModel.MenuTree.mt)
            return res
        end
    }
}

MenuModel.mt = {
    __index = {
        appendSubComp = function(self,shortname,title)
            return self.data:appendSubComp(
                shortname,title)
        end
    }
}

MenuModel.MenuTree.mt = {
    __index = {
        appendSubComp = function(self,shortname,title)
            assert( self.isLeaf == false,
                "Can only append to composite submenus." )
            local newComp = MenuModel.MenuTree.newComp(
                shortname,title)
            table.insert(self.data,newComp)
            return newComp -- for appending more
        end,
        appendSubLeaf = function(self,shortname,title,func)
            assert( self.isLeaf == false,
                "Can only append to composite submenus." )
            local num = self.model.callbackCount
            self.model.callbacks[num] = func
            self.model.callbackCount = self.model.callbackCount + 1
            local newLeaf = MenuModel.MenuTree.newLeaf(
                shortname,title,num)
            table.insert(self.data,newLeaf)
        end,
        dumpItems = function(self,table)
            table.insert(table,{
                shortname = self.shortname,
                title = self.title,
                num = self.num
            })
            if (not self.isLeaf) then
                for k,v in pairs(self.data) do
                    v:dumpItems(table)
                end
            end
        end
    }
}
