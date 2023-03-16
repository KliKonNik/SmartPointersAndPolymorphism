cmake_minimum_required (VERSION 3.21)

set (PROJECT SmartPointersAndPolymorphism)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_CCACHE        "Enable the usage of Ccache."           ON)
option(ENABLE_WARNINGS      "Enable to add warnings to a target."   ON)

set (TARGET SmartPointers)
set (OUTPUT_NAME SmartPointers)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (SOURCE_FILES
        SmartPointersAndPolymorphism.cpp
)

add_executable(${TARGET} ${SOURCE_FILES})
