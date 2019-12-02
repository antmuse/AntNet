# Build steps

file(GLOB_RECURSE THREAD_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    *.cpp)

aux_source_directory(. DIR_SRCS)

add_library(Thread STATIC ${THREAD_SRC})

include_directories(
    ${PROJECT_SOURCE_DIR}/Include
    ${PROJECT_SOURCE_DIR}/Include/Core
    ${PROJECT_SOURCE_DIR}/Include/Thread
    )

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include
    DESTINATION .
    COMPONENT headers
    )
