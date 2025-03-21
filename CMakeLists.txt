cmake_minimum_required(VERSION 3.23)
project(GrabCutEdgeDetection)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the build type to Release
set(CMAKE_BUILD_TYPE Release)

# Add the NDEBUG flag for Release builds
add_definitions(-DNDEBUG)

# Set the OpenCV static flag
set(OpenCV_STATIC ON)

# Handle OpenCV on different platforms
if(WIN32)
    set(OpenCV_DIR "D:/opencv-install")
    find_package(OpenCV REQUIRED PATHS "D:/opencv-install" NO_DEFAULT_PATH)
elseif(APPLE)
    # If on macOS, Homebrew might be used to install OpenCV
    find_package(OpenCV REQUIRED PATHS /usr/local/opt/opencv4 /opt/homebrew/opt/opencv4)
else()
    # Default OpenCV search if the platform is neither macOS nor Windows
    find_package(OpenCV REQUIRED)
endif()

# Ensure that OpenCV is found
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Please install OpenCV and make sure it's in the CMake search path.")
endif()

# Set MSVC runtime library (for Windows)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Add the executable
add_executable(grabcut_edge_detection grabcut_edge_detection.cpp)

# Link OpenCV libraries
target_link_libraries(grabcut_edge_detection ${OpenCV_LIBS})

# Optional: Add include directories for OpenCV headers
include_directories(${OpenCV_INCLUDE_DIRS})

# Set the output directory for the built executable
set_target_properties(grabcut_edge_detection PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Suppress library conflicts for MSVC
if(MSVC)
    target_link_options(grabcut_edge_detection PRIVATE "/NODEFAULTLIB:library")
endif()
