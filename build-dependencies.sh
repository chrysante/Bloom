# run ./configure first!

./configure.sh

# externals/glfw (Has no premake5 script)
cmake --build Build/int/glfw
cmake --build Build/int/minizip
cmake --build Build/int/assimp

cp Build/int/glfw/src/Debug/libglfw3.a Build/Bin/Debug/
cp Build/int/glfw/src/Debug/libglfw3.a Build/Bin/Checked/
cp Build/int/glfw/src/Debug/libglfw3.a Build/Bin/Release/

cp Build/int/minizip/Debug/libminizip.a Build/Bin/Debug/
cp Build/int/minizip/Debug/libminizip.a Build/Bin/Checked/
cp Build/int/minizip/Debug/libminizip.a Build/Bin/Release/

cp Build/int/assimp/bin/Debug/**.dylib Build/Bin/Debug/
cp Build/int/assimp/bin/Debug/**.dylib Build/Bin/Checked/
cp Build/int/assimp/bin/Debug/**.dylib Build/Bin/Release/
cp Build/int/assimp/include/assimp/** external/assimp/include/assimp/

# externals/minizip (Has no premake5 script)
cmake --build Build/int/minizip
cp Build/int/minizip/libminizip.a Build/Bin/Debug/
cp Build/int/minizip/libminizip.a Build/Bin/Checked/
cp Build/int/minizip/libminizip.a Build/Bin/Release/

# Externals
xcodebuild -project external/assimp/assimp.xcodeproj
xcodebuild -project external/catch2/catch2.xcodeproj
xcodebuild -project external/glfw/GLFW.xcodeproj
xcodebuild -project external/imgui/imgui.xcodeproj
xcodebuild -project external/utility/utility.xcodeproj
xcodebuild -project external/yaml-cpp/YAML.xcodeproj
xcodebuild -project external/scatha/scatha.xcodeproj -configuration Release
xcodebuild -project external/scatha/runtime.xcodeproj -configuration Release

cp external/scatha/build/bin/Release/** build/bin/release/

cp build/bin/release/** build/bin/debug/
cp build/bin/release/** build/bin/checked/




