project "YAML"
location "."
kind "StaticLib"
language "C++"    

externalincludedirs ".."

files {
    "src/**.h",
    "src/**.hpp",
    "src/**.cpp"
}