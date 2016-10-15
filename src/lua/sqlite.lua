
function sqlRevNumber()
    return "SELECT revision_number,datetime(modification_date,'unixepoch','localtime')" ..
           " FROM metadata;"
end
