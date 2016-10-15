
function setStatus(context,widget,text)
    context:message(widget,VSig("MWI_InSetStatusText"),VString(text))
end

function fullDownloadModelUpdate(ctx,wgt)
    ctx:message(wgt,VSig("DLMDL_InFullUpdate"))
end

function setWidgetsEnabled(context,wnd,value,...)
    local values = {...}
    for k, v in pairs(values) do
        context:message(wnd,
            VSig("MWI_InSetWidgetEnabled"),
            VString(v),
            VBool(value)
        )
    end
end

