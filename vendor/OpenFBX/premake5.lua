project "OpenFBX"
location "."
kind "SharedLib"
language "C++"

includedirs {
    "."
}

files { 
    "miniz.c",
    "ofbx.cpp"
}