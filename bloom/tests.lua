project "BloomTests"
location "."
kind "ConsoleApp"
language "C++"

includedirs {
    "src"
}

files { 
    "tests/**.hpp",
    "tests/**.cpp",
}

links {
    "Catch2",
    "Bloom", 
    "utility" 
   
}
