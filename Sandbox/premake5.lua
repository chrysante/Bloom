project "Sandbox"
location "."
kind "SharedLib"
language "C++"

sysincludedirs {
    "../Bloom/include",
    "../Bloom/src", -- for now
}

files {
    "src/Sandbox/**.hpp",
    "src/Sandbox/**.cpp"
}

filter { "system:macosx", "configurations:Debug or Development" }
    xcodebuildsettings {
        ["ONLY_ACTIVE_ARCH"] = "YES"
    }
filter {}

filter "system:macosx"
    postbuildcommands {
    --    "{COPY} %{prj.location}/Resource/** %{cfg.targetdir}/Poppy.app/Contents/Resources"
    }
filter {}

links { 
    "Bloom",
    "Utility",
    --"ImGui",
    --"YAML"
}

--------------------------------------------------------------------------------------------
--- SandboxApp
--------------------------------------------------------------------------------------------
project "SandboxApp"
location "."
kind "WindowedApp"
language "C++"

sysincludedirs {
    "../Bloom/include",
    "../Bloom/src", -- for now
}

files "src/SandboxApp.cpp"

filter "system:macosx"
    files {
        "resource/platform/macOS/MainMenu.storyboard",
        "resource/platform/macOS/Sandbox.entitlements",
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

filter "system:macosx"
    postbuildcommands {
    --    "{COPY} %{prj.location}/Resource/** %{cfg.targetdir}/Poppy.app/Contents/Resources"
    }
filter {}

links { 
    "Bloom",
    "Sandbox"
}