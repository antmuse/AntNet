# Build steps
file(GLOB_RECURSE APP_TEST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

add_executable(Test ${APP_TEST})

link_directories(${LIBRARY_OUTPUT_PATH})
target_link_libraries(Test Thread)
target_link_libraries(Test AntNet)

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

#运行文件的后缀名
set_target_properties(Test PROPERTIES DEBUG_POSTFIX "-${APP_BTYPE}D")
set_target_properties(Test PROPERTIES RELEASE_POSTFIX "-${APP_BTYPE}")
