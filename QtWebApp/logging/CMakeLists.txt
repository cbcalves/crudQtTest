set(HEADER_FILES
    logglobal.h
    logmessage.h
    logger.h
    filelogger.h
    dualfilelogger.h
)

set(PROJECT_FILES
    logmessage.cpp
    logger.cpp
    filelogger.cpp
    dualfilelogger.cpp
)

add_library(logging
    OBJECT
    ${HEADER_FILES}
    ${PROJECT_FILES}
)
target_compile_options(logging PRIVATE ${COMPILE_WARNS})
target_link_libraries(logging PRIVATE Qt${QT_VERSION_MAJOR}::Core)
