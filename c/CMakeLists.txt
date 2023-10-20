cmake_minimum_required(VERSION 3.14)

project(
    lang
    VERSION 0.1.0
    LANGUAGES C
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
      FATAL_ERROR
      "In-source builds are not supported. "
      "Please read the BUILDING document before trying to build this project. "
      "You may need to delete 'CMakeCache.txt' and 'CMakeFiles/' first."
  )
endif()

# ---- Declare library ----

add_library(
    lang_lib OBJECT
    source/bytecode.c
    source/debug.c
    source/memory.c
    source/value.c
    source/vm.c
)
target_include_directories(
    lang_lib
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)
target_compile_features(lang_lib PUBLIC c_std_99)

# ---- Declare executable ----

add_executable(lang_exe source/main.c)
add_executable(lang::exe ALIAS lang_exe)
set_property(TARGET lang_exe PROPERTY OUTPUT_NAME lang)
target_compile_features(lang_exe PRIVATE c_std_99)
target_link_libraries(lang_exe PRIVATE lang_lib)


# ---- Other targets ----
include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

# ---- Tests ----
include(cmake/folders.cmake)
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
if(ENABLE_COVERAGE)
  include(cmake/coverage.cmake)
endif()