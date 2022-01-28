cmake_minimum_required(VERSION 3.18)

project(QtWebApp VERSION 1.8.4 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)

if(MSVC)
  set(COMPILE_WARNS /W4 /WX)
else()
  set(COMPILE_WARNS -Wall -Wextra -pedantic)
endif()

add_subdirectory(httpserver)
add_subdirectory(logging)
add_subdirectory(templateengine)

add_library(${PROJECT_NAME} 
    SHARED
    $<TARGET_OBJECTS:httpserver>
    $<TARGET_OBJECTS:logging>
    $<TARGET_OBJECTS:templateengine>
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Network
)
