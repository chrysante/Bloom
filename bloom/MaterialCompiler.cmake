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
    magic_enum
  PRIVATE
    termfmt
)

set(matc_sources
  src/Bloom/MaterialCompiler/BuildModule.cpp
  src/Bloom/MaterialCompiler/CodeGenerator.cpp
  src/Bloom/MaterialCompiler/Codegen.h
  src/Bloom/MaterialCompiler/Context.cpp
  src/Bloom/MaterialCompiler/Context.h
  src/Bloom/MaterialCompiler/FunctionBuilder.cpp
  src/Bloom/MaterialCompiler/FunctionBuilder.h
  src/Bloom/MaterialCompiler/Fwd.cpp
  src/Bloom/MaterialCompiler/Fwd.h
  src/Bloom/MaterialCompiler/GenerateSource.cpp
  src/Bloom/MaterialCompiler/Lists.def
  src/Bloom/MaterialCompiler/MaterialGraph.cpp
  src/Bloom/MaterialCompiler/MaterialGraph.h
  src/Bloom/MaterialCompiler/Module.cpp
  src/Bloom/MaterialCompiler/Module.h
  src/Bloom/MaterialCompiler/Preprocessor.cpp
  src/Bloom/MaterialCompiler/Preprocessor.h
  src/Bloom/MaterialCompiler/SourceEmitter.cpp
  src/Bloom/MaterialCompiler/SourceEmitter.h
  src/Bloom/MaterialCompiler/Types.cpp
  src/Bloom/MaterialCompiler/Types.h
  src/Bloom/MaterialCompiler/Values.cpp
  src/Bloom/MaterialCompiler/Values.h
)

target_compile_definitions(matc PRIVATE BLOOM_NO_SERIALIZATION)

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

target_compile_definitions(matc-test PRIVATE BLOOM_NO_SERIALIZATION)

set(matc_test_sources
  test/MaterialCompiler/Compiler.t.cpp
)

target_sources(matc-test
  PRIVATE
    ${matc_test_sources}
)
source_group(TREE ${PROJECT_SOURCE_DIR}/bloom/test FILES ${matc_test_sources})
