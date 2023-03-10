# =======================================================================
#   WARNING    WARNING    WARNING    WARNING    WARNING    WARNING
# =======================================================================
#   Remember to put on SAFETY GOGGLES before looking at this file. You
#   are most certainly not expected to read or understand any of it.
# =======================================================================
#
# This CMake file is responsible for compiling dependency libraries and
# setting up suitable compiler flags for various platforms. You do not
# need to read or change anything in this file; see CMakeLists.txt in
# the main Nori folder instead.

include(CheckCXXCompilerFlag)

# Set a default build configuration (Release)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# Enable folders for projects in Visual Studio
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if (APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION=1")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  CHECK_CXX_COMPILER_FLAG("-std=c++17" HAS_CPP17_FLAG)
  if (HAS_CPP17_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")  
  else()
    message(FATAL_ERROR "Unsupported compiler -- requires C++17 support!")
  endif()

  # Prefer libc++ in conjunction with Clang
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    CHECK_CXX_COMPILER_FLAG("-stdlib=libc++" HAS_LIBCPP)
    if (HAS_LIBCPP)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
      message(STATUS "Nori: using libc++.")
    else()
      message(WARNING "libc++ is recommended in conjunction with clang. Please insteall the libc++ development headers, provided e.g. by the packages 'libc++-dev' and 'libc++abi-dev' on Debian/Ubuntu.")
    endif()
  endif()

  # Enable link time optimization and set the default symbol
  # visibility to hidden (very important to obtain small binaries)
  if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
    # Default symbol visibility
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
  endif()

  # Disable specific GCC 7 warnings
  if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-misleading-indentation -Wformat-truncation=0 -Wno-int-in-bool-context -Wimplicit-fallthrough=0")
  endif()
endif()

# Sanitize build environment for static build with C++11
if (MSVC)
  # Disable annoying secure CRT warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")

  # We'll select the TBB library ourselves
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D__TBB_NO_IMPLICIT_LINKAGE")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

    # Disable Eigen vectorization for Windows 32 bit builds (issues with unaligned access segfaults)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEIGEN_DONT_ALIGN")
  endif()

  # Static build
  set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
endif()

# Build zlib (only on Windows)
set(ZLIB_BUILD_STATIC_LIBS ON CACHE BOOL " " FORCE)
set(ZLIB_BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
add_subdirectory(zlib)

set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib" CACHE PATH " " FORCE)
if (CMAKE_GENERATOR STREQUAL "Ninja")
  set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib/zlibstatic.lib" CACHE FILEPATH " " FORCE)
else()
  set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib/$<CONFIGURATION>/zlibstatic.lib" CACHE FILEPATH " " FORCE)
endif()

set_property(TARGET zlibstatic PROPERTY FOLDER "dependencies")
include_directories(${ZLIB_INCLUDE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/zlib")
set(ZLIB_INCLUDE_DIR2 "${CMAKE_CURRENT_BINARY_DIR}/zlib")

# Build Thread Building Blocks (main shared libraries only)
set(TBB_BUILD_SHARED          OFF  CACHE BOOL " " FORCE)
set(TBB_BUILD_STATIC          ON   CACHE BOOL " " FORCE)
set(TBB_BUILD_TBBMALLOC       OFF  CACHE BOOL " " FORCE)
set(TBB_BUILD_TBBMALLOC_PROXY OFF  CACHE BOOL " " FORCE)
set(TBB_BUILD_TESTS           OFF  CACHE BOOL " " FORCE)
add_subdirectory(tbb)
set_property(TARGET tbb_static tbb_def_files PROPERTY FOLDER "dependencies")

# Build NanoGUI
# Use OpenGL backend for UI. In the future, this can be removed to use Metal on MacOS
set(NANOGUI_BACKEND "OpenGL" CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_SHARED  OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
add_subdirectory(nanogui)
set_property(TARGET glfw glfw_objects nanogui PROPERTY FOLDER "dependencies")

# Build the pugixml parser
add_library(pugixml STATIC pugixml/src/pugixml.cpp)
set_property(TARGET pugixml PROPERTY
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pugixml")
set_property(TARGET pugixml PROPERTY FOLDER "dependencies")

# Build tinyobjloader
set(TINYOBJLOADER_BUILD_TEST_LOADER OFF CACHE BOOL " " FORCE)
set(TINYOBJLOADER_BUILD_OBJ_STICHER  OFF CACHE BOOL " " FORCE)
add_subdirectory(tinyobjloader)

set(PCG32_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/pcg32)
set(TFM_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/tinyformat)
set(HYPOTHESIS_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/hypothesis)
set(GLFW_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/glfw/include)
set(GLEW_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/glew/include)
set(NANOVG_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/nanovg/src)
set(STB_IMAGE_WRITE_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/nanovg/example)
set(NANOGUI_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/include)
set(EIGEN_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/eigen)
set(TBB_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/tbb/include)
set(FILESYSTEM_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/filesystem)
set(PUGIXML_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src)
set(TINYOBJLOADER_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader)
set(TINYEXR_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyexr)

# Compile remainder of the codebase with compiler warnings turned on
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # Re-enable disabled warnings
  # if(CMAKE_CXX_FLAGS MATCHES "-Wno-[^ ]+")
  #   string(REGEX REPLACE "-Wno-[^ ]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  # endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-nested-anon-types -Wno-deprecated-register")
  endif()
endif()

set(CompilerFlags
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS
      CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELWITHDEBINFO COMPILE_DEFINITIONS U_CMAKE_BUILD_TYPE
      CMAKE_MACOSX_RPATH
      PCG32_INCLUDE_DIR TFM_INCLUDE_DIR
      HYPOTHESIS_INCLUDE_DIR GLFW_INCLUDE_DIR GLEW_INCLUDE_DIR
      NANOVG_INCLUDE_DIR NANOGUI_EXTRA_INCS NANOGUI_EXTRA_DEFS
	    NANOGUI_EXTRA_LIBS NANOGUI_INCLUDE_DIR EIGEN_INCLUDE_DIR
      STB_IMAGE_WRITE_INCLUDE_DIR TBB_INCLUDE_DIR
      FILESYSTEM_INCLUDE_DIR PUGIXML_INCLUDE_DIR
      TINYOBJLOADER_INCLUDE_DIR
      TINYEXR_INCLUDE_DIR ZLIB_INCLUDE_DIR ZLIB_INCLUDE_DIR2
)
foreach(CompilerFlag ${CompilerFlags})
  set(${CompilerFlag} "${${CompilerFlag}}" PARENT_SCOPE)
endforeach()
