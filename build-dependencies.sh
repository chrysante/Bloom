#run ./configure first!

#externals/minizip ## has no premake5 script
cmake --build Build/int/minizip
cp Build/int/minizip/libminizip.a Build/Bin/Debug/
cp Build/int/minizip/libminizip.a Build/Bin/Checked/
cp Build/int/minizip/libminizip.a Build/Bin/Release/



#externals
xcodebuild -project external/assimp/assimp.xcodeproj
xcodebuild -project external/catch2/catch2.xcodeproj
xcodebuild -project external/glfw/GLFW.xcodeproj
xcodebuild -project external/imgui/imgui.xcodeproj
xcodebuild -project external/utility/utility.xcodeproj
xcodebuild -project external/yaml-cpp/YAML.xcodeproj

cp build/bin/release/** build/bin/debug/
cp build/bin/release/** build/bin/checked/




