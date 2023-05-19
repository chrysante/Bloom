project "Poppy"
location "."
kind "WindowedApp"
language "C++"

includedirs {
    "src",
    "../Bloom/include",
    "../Bloom/src", -- for now
}

externalincludedirs {
    "../external/imgui",  
    "../external/glfw/include",  
}

files {
    "src/Poppy/**.h",
    "src/Poppy/**.hpp",
    "src/Poppy/**.cpp",
    "src/Poppy/**.c", -- for stb_image compiler
}

removefiles { "src/Poppy/Platform/**" }

links { 
    "Bloom",
    "utility",
    "imgui",
    "YAML",
}

addScatha()

filter "system:macosx"
    -- common mac stuff
    files {
        "src/Poppy/**.metal",

        "src/Poppy/Platform/MacOS/**.h",
        "src/Poppy/Platform/MacOS/**.hpp",
        "src/Poppy/Platform/MacOS/**.cpp",
        "src/Poppy/Platform/MacOS/**.m",
        "src/Poppy/Platform/MacOS/**.mm",
    }
    xcodebuildsettings { 
        ["MTL_HEADER_SEARCH_PATHS"] = "src ../Bloom/src/ ../Vendor/Utility"
    }

    -- mac app stuff
    files {
        "resource/platform/macOS/MainMenu.storyboard",
        "resource/platform/macOS/Poppy.entitlements",
        "resource/platform/macOS/info.plist"
    }
    xcodebuildsettings {
        ["INFOPLIST_FILE"] = "$(SRCROOT)/resource/Platform/macOS/info.plist"
    }
    postbuildcommands {
        "mkdir -p %{cfg.targetdir}/Poppy.app/Contents/Resources",
        "mkdir -p %{cfg.targetdir}/Poppy.app/Contents/Frameworks",
        "{COPY} %{prj.location}/Resource/** %{cfg.targetdir}/Poppy.app/Contents/Resources",
        "{COPY} %{cfg.targetdir}/**.dylib %{cfg.targetdir}/Poppy.app/Contents/Frameworks"
    }

    links {
        "AppKit.framework",
        "Metal.framework",
        "GameController.framework",
        "CoreImage.framework",
        
    }
filter {}

removefiles "./**-depr.**"