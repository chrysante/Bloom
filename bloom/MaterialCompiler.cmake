add_library(matc SHARED)
set_target_properties(matc PROPERTIES LINKER_LANGUAGE CXX)
BLSetCompilerOptions(matc)

target_include_directories(matc
    PUBLIC
      src
)

target_link_libraries(matc
  PUBLIC
    utility
    vml
    range-v3
  PRIVATE
    termfmt
    magic_enum
)

set(matc_sources
  src/Bloom/MaterialCompiler/Context.cpp
  src/Bloom/MaterialCompiler/Context.h
  src/Bloom/MaterialCompiler/Fwd.h
  src/Bloom/MaterialCompiler/Lists.def
  src/Bloom/MaterialCompiler/MaterialGraph.cpp
  src/Bloom/MaterialCompiler/MaterialGraph.h
  src/Bloom/MaterialCompiler/Types.cpp
  src/Bloom/MaterialCompiler/Types.h
  src/Bloom/MaterialCompiler/Values.cpp
  src/Bloom/MaterialCompiler/Values.h
)

target_sources(matc
  PRIVATE
    ${matc_sources}
)
source_group(TREE ${PROJECT_SOURCE_DIR}/bloom/src/Bloom FILES ${matc_sources})

if(NOT BLOOM_BUILD_TESTS)
  return()
endif()

# matc-test
add_executable(matc-test)
set_target_properties(matc-test PROPERTIES LINKER_LANGUAGE CXX)
BLSetCompilerOptions(matc-test)

target_include_directories(matc-test
    PRIVATE
      src
      test
)

target_link_libraries(matc-test
  PRIVATE
    matc
    Catch2::Catch2WithMain
)

set(matc_test_sources
  test/MaterialCompiler/Compiler.t.cpp
)

target_sources(matc-test
  PRIVATE
    ${matc_test_sources}
)
source_group(TREE ${PROJECT_SOURCE_DIR}/bloom/test FILES ${matc_test_sources})
