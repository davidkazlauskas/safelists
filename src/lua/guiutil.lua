
-- on select function receives integer option selected,
-- -1 if none
function makePopupMenuModel(context,table,onSelectFunction)
    local theModel = table

    local menuModelHandler = context:makeLuaMatchHandler(
        VMatch(function(natpack,val)
            local num = val:values()._2 + 1
            natpack:setSlot(3,VString(theModel[num]))
        end,"MWI_PMM_QueryItem"),
        VMatch(function(natpack,val)
            natpack:setSlot(2,VInt(#theModel))
        end,"MWI_PMM_QueryCount","int"),
        VMatch(function(natPack,val)
            local res = val:values()._2
            onSelectFunction(res)
        end,"MWI_PMM_OutSelected","int")
    )

    return menuModelHandler
end

