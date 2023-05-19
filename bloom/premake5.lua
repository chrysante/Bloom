project "Bloom"
location "."
kind "SharedLib"
language "C++"

includedirs {
    "src"
}

externalincludedirs {
    "../external/Metal-cpp",
    "../external/assimp/include",
    "../external/glfw/include",
    "../external/scatha/include",
}

files { 
    "src/Bloom/**.hpp",
    "src/Bloom/**.cpp"
}

removefiles { "src/Bloom/platform/**" }

links { 
    "YAML",
    "utility",
    "assimpd",
    "glfw3",
}

addScatha()

filter  "system:macosx"
    files {
        "src/Bloom/platform/Metal/**.h",
        "src/Bloom/platform/Metal/**.mm",
        "src/Bloom/platform/Metal/**.hpp",
        "src/Bloom/platform/Metal/**.cpp",

        "src/Bloom/platform/MacOS/**.h",
        "src/Bloom/platform/MacOS/**.mm",
        "src/Bloom/platform/MacOS/**.hpp",
        "src/Bloom/platform/MacOS/**.cpp",
    }

    xcodebuildsettings {
        ["GCC_SYMBOLS_PRIVATE_EXTERN"] = "YES"
    }

    links {
        "AppKit.framework",
        "Metal.framework",
        "IOKit.framework",
        "QuartzCore.framework"
    }
filter {}

removefiles "./**-depr.**"