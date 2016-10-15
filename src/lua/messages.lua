
function setStatus(context,widget,text)
    context:message(widget,VSig("MWI_InSetStatusText"),VString(text))
end

