# Component name (using all lowercase):

COMPNAME	:= amf

# List local source files needed for the component server:

SRC_FILES	:= $(wildcard *.c)

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer cnt debug eo rmd ioc amf

SRC_FILES_cpmStatusNodeGet := $(SRC_FILES)
PURIFY_SHARED_LIB_NAMES := cpmStatusNodeGet

#ifdef SOLARIS_BUILD
#EXTRA_SHARED_LDFLAGS=-L$(ASP_LIB) -lmw $(LIB_BERKELEY_DB) -L$(MODEL_LIB) -L$(COMP_ROOT)/ground/client -lClGround
#else
#EXTRA_SHARED_LDFLAGS=-Wl,--start-group -L$(ASP_LIB) -lmw -L$(MODEL_LIB) -Wl,--end-group -L$(COMP_ROOT)/ground/client -lClGround
#endif

#CPPFLAGS += -I ../../include -I ../../../utils/include

SYS_LIBS  := -ldl
BUILD_WITHOUT_C99 := 1
BUILD_CPLUSPLUS :=1

# If there are more than one client libraries to be built here, you
# need to list the source files that will make up each library:
#
# Example:
#     SRC_FILES_libClTmpcomp_basic.a: file1.c file2.c
#     SRC_FILES_libClTmpcomp_advanced.a: file1.c file2.c file3.c file4.c
#
# If your component builds one library only, than this list will automatically
# obtained from the SRC_FILES variable above, so you don't need to create it.
#
include $(CLOVIS_ROOT)/ASP/mk/make-client.mk

