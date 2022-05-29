workspace "Bloom"

architecture "x86_64"
cppdialect "C++20"
staticruntime "On"

configurations {
    "Debug",
    "Checked",
    "Release"
}

filter "configurations:Debug" 
    symbols "On"
filter "configurations:Checked or Release"
    optimize "Speed"
    defines "NDEBUG"
filter {}

sysincludedirs {
    "external", "external/utility"
}

targetdir("build/bin/%{cfg.longname}")
objdir("build/obj/%{cfg.longname}")

systemversion ("latest")

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
--include "external/glfw"
include "external/imgui"
--include "external/utility"
--include "external/yaml-cpp"
--include "external/assimp"
function removeDeprecated() 
    removefiles "./**-depr.**"
end