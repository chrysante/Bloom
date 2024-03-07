workspace "Bloom"

architecture "x86_64"
cppdialect "C++20"
staticruntime "On"

configurations {
    "Debug",
    "Checked",
    "Release"
}

function addScatha() 
    libdirs "../external/scatha/build/bin/%{cfg.buildcfg}"
    externalincludedirs { 
        "../external/scatha/include", 
        "../external/scatha/runtime/include", 
        "../external/scatha/external/APMath/include", 
        "../external/scatha/external/range-v3/include", 
        "../external/scatha/external/utility/include", 
    }
    links { 
        "libscatha.dylib",
        "runtime"
    }
end

filter "configurations:Debug" 
    symbols "On"
filter "configurations:Checked or Release"
    optimize "Speed"
    defines "NDEBUG"
filter {}

externalincludedirs {
    "external", "external/utility/include"
}

targetdir("build/bin/%{cfg.longname}")
objdir("build/obj/%{cfg.longname}")

-- platform windows
filter "system:windows" 
    buildoptions { "/Zc:__cplusplus", "/Zc:preprocessor", "/MP" }
-- platform macos
filter "system:macosx"
    xcodebuildsettings { 
        ["CLANG_ENABLE_OBJC_ARC"]      = "YES",
        ["INSTALL_PATH"]               = "@executable_path/../Frameworks",
        ["LD_RUNPATH_SEARCH_PATHS"]    = "@loader_path/../Frameworks"
    }
filter { "system:macosx", "configurations:Debug or Development" }
    xcodebuildsettings {
        ["ONLY_ACTIVE_ARCH"] = "YES"
    }
filter {}

-- Projects
include "Bloom"
include "Bloom/tests.lua"
include "Poppy"

-- Externals
include "external/imgui"
include "external/utility/lib.lua"
include "external/yaml-cpp/premake5.lua"
function removeDeprecated() 
    removefiles "./**-depr.**"
end