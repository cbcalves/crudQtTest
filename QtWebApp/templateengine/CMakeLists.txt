set(HEADER_FILES
    templateglobal.h
    template.h
    templateloader.h
    templatecache.h
)

set(PROJECT_FILES
    template.cpp
    templateloader.cpp
    templatecache.cpp
)

add_library(templateengine
    OBJECT
    ${HEADER_FILES}
    ${PROJECT_FILES}
)
target_compile_options(templateengine PRIVATE ${COMPILE_WARNS})
target_link_libraries(templateengine PRIVATE Qt${QT_VERSION_MAJOR}::Core)
