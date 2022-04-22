function BloomApp(name)

    filter "system:macosx"
    files {
        "resource/platform/macOS/MainMenu.storyboard",
        "resource/platform/macOS/"..name..".entitlements",
        "resource/platform/macOS/info.plist"
    }
    xcodebuildsettings {
        ["INFOPLIST_FILE"] = "$(SRCROOT)/resource/Platform/macOS/info.plist"
    }
filter { "system:macosx", "configurations:Debug or Development" }
    xcodebuildsettings {
        ["ONLY_ACTIVE_ARCH"] = "YES"
    }
filter {}

end