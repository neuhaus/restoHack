#
# Build Hack's object-name table for a cross build.
#
# hack.onames.h is produced by makedefs, a host utility, from def.objects.h.
# That is fine in a native build - restoHack's own CMakeLists compiles makedefs
# and runs it. It is not fine here: this is a cross build, so an makedefs built
# by the toolchain is an ARM binary that will not run on the machine building
# it. So this script compiles makedefs with the *host* compiler and runs that.
#
# Invoked through `cmake -P`, which runs on the build host regardless of the
# toolchain in play:
#
#   cmake -DHOST_C_COMPILER=cc \
#         -DMAKEDEFS_SRC=<src>/makedefs.c \
#         -DDEF_OBJECTS=<src>/def.objects.h \
#         -DMAKEDEFS_BIN=<build>/makedefs-host \
#         -DOUT_FILE=<build>/generated/hack.onames.h \
#         -P gen-onames.cmake
#

execute_process(
    COMMAND ${HOST_C_COMPILER} -O1 -o ${MAKEDEFS_BIN} ${MAKEDEFS_SRC}
    RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "could not compile host makedefs (${rc})")
endif()

execute_process(
    COMMAND ${MAKEDEFS_BIN} ${DEF_OBJECTS}
    OUTPUT_FILE ${OUT_FILE}
    RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "makedefs ${DEF_OBJECTS} failed (${rc})")
endif()
