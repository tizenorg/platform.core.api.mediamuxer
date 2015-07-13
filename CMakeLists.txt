
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(fw_name "capi-mediamuxer")

PROJECT(${fw_name})

SET(CMAKE_INSTALL_PREFIX /usr)
SET(PREFIX ${CMAKE_INSTALL_PREFIX})

SET(INC_DIR include)
SET(INC_PORT_GST_DIR include/port_gst)
SET(INC_PORT_CUSTOM_DIR include/port_custom)
SET(INC_PORT_FFMPEG_DIR include/port_ffmpeg)
INCLUDE_DIRECTORIES(${INC_DIR} ${INC_PORT_GST_DIR} ${INC_PORT_CUSTOM_DIR} ${INC_PORT_FFMPEG_DIR})

SET(dependents "dlog glib-2.0 mm-common capi-media-tool iniparser gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-app-1.0")
SET(pc_dependents "capi-base-common capi-media-tool")

INCLUDE(FindPkgConfig)
pkg_check_modules(${fw_name} REQUIRED ${dependents})
FOREACH(flag ${${fw_name}_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "-I./include -I./include/headers ${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall -Werror")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

IF("${ARCH}" STREQUAL "arm")
    ADD_DEFINITIONS("-DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DTIZEN_DEBUG")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=${LIB_INSTALL_DIR}")

AUX_SOURCE_DIRECTORY (src MAIN_SRC)
AUX_SOURCE_DIRECTORY (src/port_gst PORT_GST_SRC)
AUX_SOURCE_DIRECTORY (src/port_custom PORT_CUST_SRC)
AUX_SOURCE_DIRECTORY (src/port_ffmpeg PORT_FFMPEG_SRC)

LIST (APPEND SOURCES
     ${MAIN_SRC}
     ${PORT_FFMPEG_SRC}
     ${PORT_CUST_SRC}
     ${PORT_GST_SRC}
     )

ADD_LIBRARY(${fw_name} SHARED ${SOURCES})

TARGET_LINK_LIBRARIES(${fw_name} ${${fw_name}_LDFLAGS})

SET_TARGET_PROPERTIES(${fw_name}
     PROPERTIES
     VERSION ${FULLVER}
     SOVERSION ${MAJORVER}
     CLEAN_DIRECT_OUTPUT 1
)

INSTALL(TARGETS ${fw_name} DESTINATION ${LIB_INSTALL_DIR})
INSTALL(
        DIRECTORY ${INC_DIR}/ DESTINATION include/media
        FILES_MATCHING
        PATTERN "mediamuxer_*.h" EXCLUDE
        PATTERN "${INC_DIR}/*.h"
        )

SET(PC_NAME ${fw_name})
SET(PC_REQUIRED ${pc_dependents})
SET(PC_LDFLAGS -l${fw_name})
SET(PC_CFLAGS -I\${includedir}/media)

CONFIGURE_FILE(
    ${fw_name}.pc.in
    ${CMAKE_CURRENT_SOURCE_DIR}/${fw_name}.pc
    @ONLY
)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${fw_name}.pc DESTINATION lib/pkgconfig)

ADD_SUBDIRECTORY(test)

IF(UNIX)

ADD_CUSTOM_TARGET (distclean @echo cleaning for source distribution)
ADD_CUSTOM_COMMAND(
        DEPENDS clean
        COMMENT "distribution clean"
        COMMAND find
        ARGS    .
        -not -name config.cmake -and \(
        -name tester.c -or
        -name Testing -or
        -name CMakeFiles -or
        -name cmake.depends -or
        -name cmake.check_depends -or
        -name CMakeCache.txt -or
        -name cmake.check_cache -or
        -name *.cmake -or
        -name Makefile -or
        -name core -or
        -name core.* -or
        -name gmon.out -or
        -name install_manifest.txt -or
        -name *.pc -or
        -name *~ \)
        | grep -v TC | xargs rm -rf
        TARGET  distclean
        VERBATIM
)

ENDIF(UNIX)

