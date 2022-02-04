add_library(httpaddons
    OBJECT
    router.h
    router.cpp
    jsondtohandler.h
    jsondtohandler.cpp
)
target_compile_options(httpaddons PRIVATE ${COMPILE_WARNS})
target_link_libraries(httpaddons 
    PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Network
)
