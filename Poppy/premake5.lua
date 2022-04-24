project "Poppy"
location "."
kind "WindowedApp"
language "C++"

includedirs {
    "src",
    "../Bloom/include",
    "../Bloom/src", -- for now
}

sysincludedirs {
    "../Vendor/imgui",  
    "../Vendor/Metal-cpp",
    "../Vendor/assimp/include"
}

files {
    "src/Poppy/**.h",
    "src/Poppy/**.hpp",
    "src/Poppy/**.cpp",
    "src/Poppy/**.c", -- for stb_image compiler
    "src/Poppy/**.metal"
}

filter "system:macosx"
    files {
        "resource/platform/macOS/MainMenu.storyboard",
        "resource/platform/macOS/Poppy.entitlements",
        "resource/platform/macOS/info.plist",
        "src/**.mm"
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
    xcodebuildsettings { 
        ["CLANG_ENABLE_OBJC_ARC"] = "YES"
    }

    xcodebuildsettings { 
        ["MTL_HEADER_SEARCH_PATHS"] = "../Bloom/src/ ../Vendor/Utility"
    }

    postbuildcommands {
        "{COPY} %{prj.location}/Resource/** %{cfg.targetdir}/Poppy.app/Contents/Resources"
    }
filter {}

links { 
    "Bloom",
    "Utility",
    "ImGui",
    "OpenFBX",
    "assimp",
    "YAML",
}

filter "system:macosx"
    links {
        "AppKit.framework",
        "Metal.framework",
        "MetalKit.framework",
        "GameController.framework",
        --"CoreImage.framework",
      --  "UniformTypeIdentifiers.framework"
    }
filter {}

include "PoppyTest.lua"