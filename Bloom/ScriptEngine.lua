project "ScriptEngine"
location "."
kind "StaticLib"
language "C++"

sysincludedirs {

}

includedirs {
    "src"
}

files { 
    "src/Bloom/ScriptEngine/**.hpp",
    "src/Bloom/ScriptEngine/**.cpp"
}

filter { "system:macosx", "configurations:Debug or Development" }
    xcodebuildsettings {
        ["ONLY_ACTIVE_ARCH"] = "YES"
    }
filter {}

filter "system:macosx" 
    xcodebuildsettings { 
        ["GCC_SYMBOLS_PRIVATE_EXTERN"] = "YES"
    }
filter {}

links {
    "Utility"
}

filter "system:macosx"
    links {

    }
filter {}

