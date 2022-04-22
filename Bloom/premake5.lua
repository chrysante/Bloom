---------- Bloom ----------
project "Bloom"
location "."
kind "SharedLib"
language "C++"

sysincludedirs {
    "../Vendor/Metal-cpp"
}

includedirs {
    "src"
}

files { 
    "src/Bloom/**.hpp",
    "src/Bloom/**.cpp"
}

removefiles { "src/Bloom/platform/**" }

filter  "system:macosx"
    files {
        "src/Bloom/platform/macOS/**",
        "src/Bloom/platform/Metal/**"
    }
filter {}

filter { "system:macosx", "configurations:Debug or Development" }
    xcodebuildsettings {
        ["ONLY_ACTIVE_ARCH"] = "YES"
    }
filter {}


filter "system:macosx" 
    xcodebuildsettings { 
        ["CLANG_ENABLE_OBJC_ARC"] = "YES",
        ["GCC_SYMBOLS_PRIVATE_EXTERN"] = "YES"
    }
filter {}

links { 
    "Utility"
}

filter "system:macosx"
    links {
        "AppKit.framework",
        "Metal.framework",
        "MetalKit.framework"
    }
filter {}

---------- BloomTest ----------
project "BloomTest"
location "."
kind "ConsoleApp"
language "C++"

sysincludedirs {

}

includedirs {
    "src"
}

files { 
    "Tests/**.hpp",
    "Tests/**.cpp"
}

filter { "system:macosx", "configurations:Debug or Development" }
    xcodebuildsettings {
        ["ONLY_ACTIVE_ARCH"] = "YES"
    }
filter {}

links { 
    "Bloom"
}