# Copyright (c) 2002-2006 MontaVista Software, Inc.
# Copyright (c) 2006-2007 Red Hat, Inc,
# 
# All rights reserved.
# 
# This software licensed under BSD license, the text of which follows:
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# - Neither the name of the MontaVista Software, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

# Include configuration
#
include ../Makefile.inc

include $(CLOVIS_ROOT)/ASP/mk/make-cross.mk
include $(CLOVIS_ROOT)/ASP/mk/make-path.mk

override CFLAGS += -I../include \
                -I$(BUILDTOOLS_PATH)/include \
                -I$(BUILDTOOLS_PATH)/$(TARGET)/include
ifeq ($(BUILD_SHARED), 1)
 override CFLAGS += -fPIC
endif
LDFLAGS += -L./
override CFLAGS += -DUSE_CLOCK_GETTIME
ifeq (${BUILD_DYNAMIC}, 1) 
	override LDFLAGS += ${DYFLAGS}
endif

ifeq (${OPENAIS_COMPAT}, LINUX) 
	override LDFLAGS += -ldl
endif


# Clovis include files 
CLOVIS_FLAGS  += -g -Wall -I$(CLOVIS_ROOT)/ASP/components/include/ \
                 -I$(CLOVIS_ROOT)/ASP/components/gms/include \
                 -I$(CLOVIS_ROOT)/ASP/components/debug/include \
                 -I$(CLOVIS_ROOT)/ASP/components/eo/include \
                 -I$(CLOVIS_ROOT)/ASP/components/rmd/include \
                 -I$(CLOVIS_ROOT)/ASP/components/gms/common \
                 -I$(CLOVIS_ROOT)/ASP/components/gms/server/ \
                 -I$(CLOVIS_ROOT)/ASP/components/ioc/include  \
                 -I$(CLOVIS_ROOT)/ASP/components/buffer/include \
                 -I$(CLOVIS_ROOT)/ASP/components/utils/include \
                 -I$(CLOVIS_ROOT)/ASP/components/cnt/include \
                 -I$(CLOVIS_ROOT)/ASP/components/osal/include  \
                 -I$(CLOVIS_ROOT)/ASP/components/timer/include \
                 -I$(CLOVIS_ROOT)/ASP/components/include/ipi \
                 -I$(CLOVIS_ROOT)/ASP/components/amf/include \
                 -I$(CLOVIS_ROOT)/ASP/components/ckpt/include 

ifndef CL_NOLOG                 
CLOVIS_FLAGS += -I$(CLOVIS_ROOT)/ASP/components/log/include 
endif



# Totem objects
TOTEM_SRC = aispoll.c totemip.c totemnet.c totemrrp.c totemsrp.c totemmrp.c totempg.c crypto.c wthread.c
TOTEM_OBJS = $(OBJ_DIR)/aispoll.o $(OBJ_DIR)/totemip.o $(OBJ_DIR)/totemnet.o $(OBJ_DIR)/totemrrp.o $(OBJ_DIR)/totemsrp.o $(OBJ_DIR)/totemmrp.o $(OBJ_DIR)/totempg.o $(OBJ_DIR)/crypto.o $(OBJ_DIR)/wthread.o

ifeq ($(TARGET_QNX),1)
TOTEM_SRC += getpeereid.c
TOTEM_OBJS += $(OBJ_DIR)/getpeereid.o
else
 ifeq ($(TARGET_VXWORKS), 1)
   TOTEM_SRC += getpeereid.c
   TOTEM_OBJS += $(OBJ_DIR)/getpeereid.o
 endif
endif

# main executive objects
MAIN_SRC = main.c print.c mempool.c util.c sync.c service.c ipc.c flow.c timer.c \
	       totemconfig.c mainconfig.c
MAIN_OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/print.o $(OBJ_DIR)/mempool.o $(OBJ_DIR)/util.o $(OBJ_DIR)/sync.o $(OBJ_DIR)/service.o $(OBJ_DIR)/ipc.o $(OBJ_DIR)/flow.o $(OBJ_DIR)/timer.o $(OBJ_DIR)/totemconfig.o $(OBJ_DIR)/mainconfig.o
    
OTHER_SRC = objdb.c aisparser.c ysf_ykd.o ../lcr/lcr_ifact.c
OTHER_OBJS = $(OBJ_DIR)/objdb.o $(OBJ_DIR)/aisparser.o $(OBJ_DIR)/vsf_ykd.o $(OBJ_DIR)/lcr_ifact.o

CLOVIS_SRC = clovis_gms_wrapper.c clovis_wrapper_xdr_marshall.c clovis_wrapper_xdr_unmarshall.c
CLOVIS_OBJ = $(OBJ_DIR)/clovis_gms_wrapper.o $(OBJ_DIR)/clovis_wrapper_xdr_marshall.o $(OBJ_DIR)/clovis_wrapper_xdr_unmarshall.o

ifeq ($(BUILD_SHARED), 1)
lib: $(LIB_DIR)/libtotem_pg.a $(LIB_DIR)/libaisexec.a \
	$(LIB_DIR)/libaisexec.so
else
lib: $(LIB_DIR)/libtotem_pg.a $(LIB_DIR)/libaisexec.a
endif

all: $(LIB_DIR)/libaisexec.a


$(LIB_DIR)/libaisexec.a: $(MAIN_OBJS) $(OTHER_OBJS) $(CLOVIS_OBJ) $(LIB_DIR)/libtotem_pg.a 
	$(AR) -rc $(LIB_DIR)/libaisexec.a $(MAIN_OBJS) $(OTHER_OBJS) $(CLOVIS_OBJ) $(LIB_DIR)/libtotem_pg.a

$(LIB_DIR)/libtotem_pg.a: $(TOTEM_OBJS)
	$(AR) -rc $(LIB_DIR)/libtotem_pg.a $(TOTEM_OBJS)

ifeq ($(BUILD_SHARED), 1)
$(LIB_DIR)/libaisexec.so: $(MAIN_OBJS) $(OTHER_OBJS) $(CLOVIS_OBJ) $(TOTEM_OBJS)
	$(CC) $(LDFLAGS) -fPIC -shared  $(TARGET_ARCH) $^ -o $@
endif

clean:
	rm -f $(LIB_DIR)/libaisexec.* $(OBJ_DIR)/*.o $(LIB_DIR)/libtotem_pg.a *.da *.bb *.bbg

depend:
	makedepend -Y -- $(CFLAGS) $(CPPFLAGS) $(EXEC_SRC) $(TOTEM_SRC) $(LCR_SRC) > /dev/null 2>&1

$(MAIN_OBJS): $(MAIN_SRC)
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR))
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c

$(OTHER_OBJS): $(MAIN_SRC)
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR))
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c

$(TOTEM_OBJS): $(MAIN_SRC)
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR))
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c

# - fPIC rules required for service handler shared objects
$(OBJ_DIR)/lcr_ifact.o: ../lcr/lcr_ifact.c
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR))
	$(CC) $(CFLAGS) $(CPPFLAGS) -I../lcr -c -o $@ ../lcr/lcr_ifact.c

#
#$(OBJ_DIR)/aisparser.o: aisparser.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/vsf_ykd.o: vsf_ykd.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
$(OBJ_DIR)/clovis_gms_wrapper.o: clovis_gms_wrapper.c
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR))
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CLOVIS_FLAGS) -c -o $@ $(*F).c

$(OBJ_DIR)/clovis_wrapper_xdr_marshall.o: clovis_wrapper_xdr_marshall.c
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR))
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CLOVIS_FLAGS) -c -o $@ $(*F).c

$(OBJ_DIR)/clovis_wrapper_xdr_unmarshall.o: clovis_wrapper_xdr_unmarshall.c
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR))
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CLOVIS_FLAGS) -c -o $@ $(*F).c

#$(OBJ_DIR)/objdb.o: objdb.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
## -fPIC rules required for lib totem
#
#$(OBJ_DIR)/aispoll.o: aispoll.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/totempg.o: totempg.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/totemsrp.o: totemsrp.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/totemrrp.o: totemrrp.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/totemip.o: totemip.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/totemnet.o: totemnet.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/wthread.o: wthread.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/crypto.o: crypto.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/totemmrp.o: totemmrp.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c
#
#$(OBJ_DIR)/totemconfig.o: totemconfig.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(*F).c

# DO NOT DELETE

$(OBJ_DIR)/aispoll.o: aispoll.h ../include/list.h ../include/hdb.h tlist.h
$(OBJ_DIR)/totemip.o: totemip.h ../include/swab.h
$(OBJ_DIR)/totemnet.o: aispoll.h totemnet.h totem.h totemip.h wthread.h
$(OBJ_DIR)/totemnet.o: ../include/queue.h ../include/sq.h ../include/list.h
$(OBJ_DIR)/totemnet.o: ../include/hdb.h ../include/swab.h crypto.h
$(OBJ_DIR)/totemrrp.o: ../include/queue.h ../include/sq.h ../include/list.h
$(OBJ_DIR)/totemrrp.o: ../include/hdb.h ../include/swab.h aispoll.h totemnet.h totem.h totemip.h
$(OBJ_DIR)/totemrrp.o: totemrrp.h
$(OBJ_DIR)/totemsrp.o: aispoll.h totemsrp.h totem.h totemip.h totemrrp.h wthread.h
$(OBJ_DIR)/totemsrp.o: ../include/queue.h ../include/sq.h ../include/list.h
$(OBJ_DIR)/totemsrp.o: ../include/hdb.h ../include/swab.h crypto.h
$(OBJ_DIR)/totemmrp.o: totem.h totemip.h totemsrp.h aispoll.h
$(OBJ_DIR)/totempg.o: ../include/hdb.h totempg.h aispoll.h totemsrp.h totem.h totemip.h
$(OBJ_DIR)/totempg.o: totemmrp.h ../include/swab.h
$(OBJ_DIR)/tlist.o: ../include/list.h tlist.h
$(OBJ_DIR)/crypto.o: crypto.h
$(OBJ_DIR)/wthread.o: wthread.h ../include/queue.h
$(OBJ_DIR)/aisparser.o: ../lcr/lcr_comp.h objdb.h config.h mempool.h ../include/list.h
$(OBJ_DIR)/aisparser.o: util.h ../include/saAis.h
$(OBJ_DIR)/vsf_ykd.o: main.h ../include/saAis.h ../include/saClm.h ../include/saAis.h
$(OBJ_DIR)/vsf_ykd.o: ../include/queue.h ../include/ipc_gen.h ../exec/totemip.h
$(OBJ_DIR)/vsf_ykd.o: mainconfig.h ../include/list.h aispoll.h totemsrp.h totem.h
$(OBJ_DIR)/vsf_ykd.o: totemip.h totempg.h objdb.h print.h ../include/swab.h vsf.h ../lcr/lcr_comp.h
$(OBJ_DIR)/clovis_gms_wrapper.o: totem.h totemip.h ../include/saAis.h ../include/saClm.h
$(OBJ_DIR)/clovis_gms_wrapper.o: ../include/saAis.h ../include/ipc_gen.h ../exec/totemip.h
$(OBJ_DIR)/clovis_gms_wrapper.o: ../include/ipc_clm.h ../include/saClm.h ../include/ipc_gen.h
$(OBJ_DIR)/clovis_gms_wrapper.o: ../include/mar_clm.h ../include/mar_gen.h ../include/mar_gen.h
$(OBJ_DIR)/clovis_gms_wrapper.o: ../include/mar_clm.h ../include/list.h ../include/queue.h
$(OBJ_DIR)/clovis_gms_wrapper.o: ../lcr/lcr_comp.h aispoll.h totempg.h totemsrp.h main.h mainconfig.h
$(OBJ_DIR)/clovis_gms_wrapper.o: objdb.h mempool.h service.h ../include/swab.h print.h
