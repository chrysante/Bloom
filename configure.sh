mkdir -p build/bin/debug
mkdir -p build/bin/checked
mkdir -p build/bin/release

#externals/...
mkdir -p build/int/glfw
cmake -S external/glfw -B Build/int/glfw -G Xcode

mkdir -p build/int/minizip
cmake -S external/minizip -B Build/int/minizip -G Xcode

mkdir -p build/int/assimp
cmake -S external/assimp -B Build/int/assimp -G Xcode

#externals
premake5 xcode4 --file=external/premake5.lua
