project "simplefilewatcher"
location "."
kind "SharedLib"
language "C++"
filter "system:macosx" 
    defines "FILEWATCHER_PLATFORM_KQUEUE"
filter {}

sysincludedirs {
    "include"
}

files { 
    "source/**"
}