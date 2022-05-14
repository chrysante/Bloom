project "PoppyTest"
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
    "PoppyLib",
    "YAML",
    "Utility",
}