include_directories(/usr/include/libmongoc-1.0)
include_directories(/usr/include/libbson-1.0)
add_library(mongo
    OBJECT
    mongo.h
    mongo.cpp
    mongoclient.h
    mongoclient.cpp
)
target_compile_options(mongo PRIVATE ${COMPILE_WARNS})
target_link_libraries(mongo 
    PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core
    mongoc-1.0
    bson-1.0
)
