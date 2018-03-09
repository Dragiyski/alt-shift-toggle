#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc-7
CCC=g++-7
CXX=g++-7
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GCC-7-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/KeyScanner.o \
	${OBJECTDIR}/SubscriberList.o \
	${OBJECTDIR}/UDevContext.o \
	${OBJECTDIR}/UDevDevice.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/process-tag.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=c++17
CXXFLAGS=-std=c++17

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lstdc++fs -pthread `pkg-config --libs libudev`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alt-shift-notify

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alt-shift-notify: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alt-shift-notify ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/KeyScanner.o: KeyScanner.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libudev`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/KeyScanner.o KeyScanner.cpp

${OBJECTDIR}/SubscriberList.o: SubscriberList.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libudev`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SubscriberList.o SubscriberList.cpp

${OBJECTDIR}/UDevContext.o: UDevContext.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libudev`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/UDevContext.o UDevContext.cpp

${OBJECTDIR}/UDevDevice.o: UDevDevice.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libudev`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/UDevDevice.o UDevDevice.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libudev`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/process-tag.o: process-tag.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libudev`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/process-tag.o process-tag.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
