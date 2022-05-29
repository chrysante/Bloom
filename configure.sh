mkdir -p build/bin/debug
mkdir -p build/bin/checked
mkdir -p build/bin/release
mkdir -p build/int

#externals/minizip
mkdir build/int/minizip
cmake -S external/minizip -B Build/int/minizip

#externals
premake5 xcode4 --file=external/premake5.lua
