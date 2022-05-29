workspace "Dependencies"

architecture "x86_64"
cppdialect "C++20"
staticruntime "On"

configurations { "Release" }
optimize "On"

targetdir("../build/bin/%{cfg.longname}")
objdir("../build/obj/%{cfg.longname}")

sysincludedirs {
    ".", "utility"
}

systemversion ("latest")

include "Catch2"
include "utility"
include "imgui"
include "yaml-cpp"