project "imgui"
location "."
kind "StaticLib"
language "C++"
filter "system:macosx" 
    xcodebuildsettings { 
        ["CLANG_ENABLE_OBJC_ARC"] = "YES"
    }
filter {}

includedirs {
    "."
}

files { 
    "imgui.h",
    "imgui_internal.h",
    "imgui.cpp",
    "imgui_demo.cpp",
    "imgui_draw.cpp",
    "imgui_tables.cpp",
    "imgui_widgets.cpp",
    "imgui.cpp",
    "ImGuizmo.cpp",
}

