CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(fw_test "${fw_name}-test")
SET(exe_name "mediamuxer_test")

INCLUDE_DIRECTORIES(../include)
INCLUDE_DIRECTORIES(../include/headers)
link_directories(${CMAKE_SOURCE_DIR}/../)

INCLUDE(FindPkgConfig)
FOREACH(flag ${${fw_test}_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

aux_source_directory(. sources)

FOREACH(src ${sources})
    GET_FILENAME_COMPONENT(src_name ${src} NAME_WE)
    LIST(APPEND source_files "${src_name}")
    MESSAGE("${src_name}")
ENDFOREACH()
ADD_EXECUTABLE(${exe_name} ${source_files})
TARGET_LINK_LIBRARIES(${exe_name} capi-mediamuxer ${${fw_test}_LDFLAGS})
