
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
    intOffHandler = function(node)
        return node.luaCtx:makeLuaMatchHandler(
            VMatch(function(natpack,val)
                local theId = val:values()._2
                node.parent.hookedEvents[theId].routine()
            end,"GWI_GBT_OutClickEvent","int"),
            VMatch(function(natpack,val)
                local theId = val:values()._2
                node.parent.hookedEvents[theId].routine()
            end,"GWI_GWT_OutValueChanged","int")
        )
    end
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

            local theHandler = self.parent.hookedEventTypes["singleclick"]
            if (theHandler == nil) then
                theHandler = GenericWidgetNode.intOffHandler(self)

                self.luaCtx:message(
                    self.parent.messageable,
                    VSig("GWI_SetNotifier"),
                    theHandler
                )
                self.parent.hookedEventTypes["singleclick"] = theHandler
            end

            self.parent.hookedEvents[theId] = {
                routine = theFunction,
                handler = theHandler
            }

            return theId
        end,
        hookTextChanged = function(self,theFunction)
            local theId = self.luaCtx:messageRetValues(
                self.messageable,
                VSig("GWI_GIT_HookTextChangedEvent"),
                VInt(-1)
            )._2

            local theHandler = self.parent.hookedEventTypes["singleclick"]
            if (theHandler == nil) then
                theHandler = GenericWidgetNode.intOffHandler(self)

                self.luaCtx:message(
                    self.parent.messageable,
                    VSig("GWI_SetNotifier"),
                    theHandler
                )
                self.parent.hookedEventTypes["singleclick"] = theHandler
            end

            self.parent.hookedEvents[theId] = {
                routine = theFunction,
                handler = theHandler
            }

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
        entryQueryValue = function(self)
            return self.luaCtx:messageRetValues(
                self.messageable,
                VSig("GWI_GIT_QueryValue"),
                VString("")
            )._2
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
        end,
        widgetSetActive = function(self,value)
            assert( type(value) == "boolean",
                "Boolean expected in set active.")

            self.luaCtx:message(
                self.messageable,
                VSig("GWI_GWT_SetActive"),
                VBool(value)
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
        setmetatable(res,MenuModel.mt)
        res.data = MenuModel.MenuTree.newComp(res,"ROOT","ROOT")
        return res
    end,
    MenuTree = {
        newComp = function(menModel,shortname,title)
            local res = {
                model = menModel,
                isLeaf = false,
                shortname = shortname,
                title = title,
                num = -2, -- by -2 we differentiate
                -- on native side that menu is composite
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
        end,
        enumerate = function(self)
            local container = {}
            self.data:dumpItems(container)
            container[1] = nil -- remove root

            return coroutine.create(
                function()
                    for k,v in pairs(container) do
                        coroutine.yield(v)
                    end
                    coroutine.yield({ -- signal end
                        shortname = "",
                        title = "",
                        num = -1
                    })
                end
            )
        end,
        makeMessageable = function(self,ctx)
            local corout = self:enumerate()
            local handler = ctx:makeLuaMatchHandler(
                VMatch(function(natPack,val)
                    local status,nextVal =
                        coroutine.resume(corout)
                    natPack:setSlot(2,VInt(nextVal.num))
                    natPack:setSlot(3,VString(nextVal.shortname))
                    natPack:setSlot(4,VString(nextVal.title))
                end,"GWI_GMIT_QueryNextNode","int","string","string"),
                VMatch(function(natPack,val)
                    local idx = val:values()._2
                    local outFunc = self.callbacks[idx]
                    assert( nil ~= outFunc, "No menu function with such index" )
                    outFunc()
                end,"GWI_GMIT_OutIndexClicked","int")
            )
            return handler
        end
    }
}

MenuModel.MenuTree.mt = {
    __index = {
        appendSubComp = function(self,shortname,title)
            assert( self.isLeaf == false,
                "Can only append to composite submenus." )
            local newComp = MenuModel.MenuTree.newComp(
                self.model,shortname,title)
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
                self.model,shortname,title,num)
            table.insert(self.data,newLeaf)
        end,
        dumpItems = function(self,tofill)
            table.insert(tofill,{
                shortname = self.shortname,
                title = self.title,
                num = self.num
            })
            if (not self.isLeaf) then
                for k,v in pairs(self.data) do
                    v:dumpItems(tofill)
                end
                table.insert(tofill,{
                    shortname = "",
                    title = "",
                    num = -3
                })
            end
        end
    }
}
