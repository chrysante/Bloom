workspace "Bloom"

architecture "x86_64"

configurations {
    "Debug",
    "Development",
    "Release"
}

filter "configurations:Debug" 
    symbols "On"
filter "configurations:Development or Release"
    optimize "Speed"
    defines "NDEBUG"
filter {}

cppdialect "C++20"
staticruntime "On"

sysincludedirs {
    "Vendor", "Vendor/Utility"
}

filter "system:windows" 
    buildoptions { "/Zc:__cplusplus", "/Zc:preprocessor", "/MP" }
    systemversion ("latest")
filter "system:macosx"
    systemversion ("12.2") -- until i update
filter {}

targetdir("Build/Bin/%{cfg.longname}")
objdir("Build/Obj/%{cfg.longname}")

include "Bloom"
include "Poppy"
include "Sandbox"
include "Vendor/Utility/utility_project.lua"
include "Vendor/imgui"
include "Vendor/OpenFBX"
include "Vendor/yaml-cpp"