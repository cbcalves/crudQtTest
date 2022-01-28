cmake_minimum_required(VERSION 3.18)

project(crudQtTest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)

if(MSVC)
  set(COMPILE_WARNS /W4 /WX)
else()
  set(COMPILE_WARNS -Wall -Wextra -pedantic)
endif()

# link_directories might not be /build
include_directories(QtWebApp)
link_directories(QtWebApp/build)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/requesthandler.h
    src/requesthandler.cpp
    src/router.h
    src/router.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    QtWebApp
)
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_WARNS})
