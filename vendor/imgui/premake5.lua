project "ImGui"
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
    "imgui.cpp",
    "imgui_demo.cpp",
    "imgui_draw.cpp",
    "imgui_tables.cpp",
    "imgui_widgets.cpp",
    "imgui.cpp",
    "ImGuizmo.cpp",
}

filter "system:macosx" 
    files {
        "backends/imgui_impl_osx.mm",
        "backends/imgui_impl_metal.mm"
    }
filter {}