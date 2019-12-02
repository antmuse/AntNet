# Build steps

set(PRO_NAME "AntNet-${APP_BTYPE}")
set(PRO_VERSION "0.1.0.${BUILD_NUMBER}")
set(PRO_BRIEF "${PRO_NAME} is a toy project to illustrate a full featured C++ CMake project.")

#file(GLOB_RECURSE SRC_NET RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/../Core" SRC_CORE)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_NET)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/HTTP" SRC_HTTP)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/RawSocket" SRC_RAWSOCKET)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/Mail" SRC_MAIL)
add_library(AntNet STATIC 
    ${SRC_NET}
    ${SRC_MAIL}
    ${SRC_RAWSOCKET}
    ${SRC_HTTP}
    ${SRC_CORE}
    )
link_directories(${LIBRARY_OUTPUT_PATH})
target_link_libraries(AntNet Thread)

include_directories(
    ${PROJECT_SOURCE_DIR}/Include
    ${PROJECT_SOURCE_DIR}/Include/Core
    ${PROJECT_SOURCE_DIR}/Include/Thread
    ${PROJECT_SOURCE_DIR}/Include/Net
    ${PROJECT_SOURCE_DIR}/Include/Net/HTTP
    ${PROJECT_SOURCE_DIR}/Include/Net/Mail
    ${PROJECT_SOURCE_DIR}/Include/Net/RawSocket
    )


install(DIRECTORY ${PROJECT_SOURCE_DIR}/include
    DESTINATION .
    COMPONENT headers
    )
