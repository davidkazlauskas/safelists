
function setStatus(context,widget,text)
    context:message(widget,VSig("MWI_InSetStatusText"),VString(text))
end

function fullDownloadModelUpdate(ctx,wgt)
    ctx:message(wgt,VSig("DLMDL_InFullUpdate"))
end

