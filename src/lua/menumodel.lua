
function setupMenuModel(df,dg)
    local mainWrapped = GenericWidget.putOn(dg.mainWnd)
    local menuBar = mainWrapped:getWidget("mainWindowMenuBar")

    local luaModel = MenuModel.new()
    local another = luaModel:appendSubComp("settings","Settings")
    local help = luaModel:appendSubComp("help","Help")
    local bugRep = help:appendSubLeaf("report-bug","Report a bug",
        function()
            df.openUrlInBrowser("https://bugs.launchpad.net/safelists")
        end
    )
    local themes = another:appendSubComp("settings-themes","Themes")
    local adwaita = themes:appendSubComp("theme-adwaita","Adwaita")
    adwaita:appendSubLeaf("theme-adwaita-light","light",function()
        df.loadTheme("Adwaita (light)")
    end)
    adwaita:appendSubLeaf("theme-adwaita-dark","dark",function()
        df.loadTheme("Adwaita (dark)")
    end)
    themes:appendSubLeaf("theme-raleigh","Raleigh",function()
        df.loadTheme("Raleigh")
    end)
    local vertex = themes:appendSubComp("theme-vertex","Vertex")
    vertex:appendSubLeaf("theme-vertex-light","light",function()
        df.loadTheme("Vertex (light)")
    end)
    vertex:appendSubLeaf("theme-vertex-dark","dark",function()
        df.loadTheme("Vertex (dark)")
    end)
    themes:appendSubLeaf("theme-borderline-gtk","Borderline GTK",function()
        df.loadTheme("Borderline GTK")
    end)
    local win10 = themes:appendSubComp("theme-win-10","Windows 10")
    win10:appendSubLeaf("theme-win-10-light","light",function()
        df.loadTheme("Windows 10 (light)")
    end)
    win10:appendSubLeaf("theme-win-10-dark","dark",function()
        df.loadTheme("Windows 10 (dark)")
    end)

    another:appendSubLeaf("quit-application","Quit",df.quitApplication)

    local model = luaModel:makeMessageable(dg.ctx)
    local id = df.retainObject(model)

    menuBar:menuBarSetModelStackless(model)

    -- we need to stage this next because
    -- json settings are not already queried
    local setupCurrent = function()
        -- current default
        local currTheme = dg.persistentSettings:getValue("safelists.theme")
        local defaultTheme = "Adwaita (light)"
        if (nil == currTheme) then
            df.loadTheme(defaultTheme)
        else
            df.loadTheme(currTheme)
        end
    end

    df.scheduleOneOffFunction(setupCurrent)
end

