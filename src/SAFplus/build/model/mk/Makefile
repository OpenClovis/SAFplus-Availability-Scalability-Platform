# Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
# This file is available  under  a  commercial  license  from  the
# copyright  holder or the GNU General Public License Version 2.0.
#
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# For more  information,  see the  file COPYING provided with this
# material.
################################################################################
# ModuleName  : model
# File        : Makefile
################################################################################
# Description :
################################################################################


DEP_LIBS	:= -lClTimer -lClBuffer -lClOsal -lClIoc -lClUtils -lClEo \
-lClLogClient -lClTxnClient -lClTxnAgent -lClSm -lClCnt  -lClIdl -lClRmd \
-lClCorClient -lClAmfClient -lClAmsMgmt -lClAmsXdr -lClDebugClient -lClFaultClient \
-lClEventClient -lClDbal -lClGms -lClCkpt -lClMedClient \
-lClNameClient -lMetaStruct

LD_LIBS		:= -L$(ASP_LIB) $(DEP_LIBS) -L$(MODEL_LIB) -lClConfig \
-L$(ASP_LIB) $(DEP_LIBS) -L$(MODEL_LIB) -lClConfig

SYS_LIBS	:= -lpthread

ifeq ($(BUILD_TIPC_COMPRESSION), 1)
	SYS_LIBS += -lz
endif

THIRD_PARTY_LIBS := -lezxml -lgdbm -l3rdpartyutils  -ldl

LDLIBS	+= $(LDFLAGS) $(LD_LIBS) $(SYS_LIBS) $(THIRD_PARTY_LIBS) 

ASP_EXE: $(MODEL_BIN)/asp_txn $(MODEL_BIN)/asp_alarm $(MODEL_BIN)/asp_ckpt \
$(MODEL_BIN)/asp_debug $(MODEL_BIN)/asp_event $(MODEL_BIN)/asp_name \
$(MODEL_BIN)/asp_cor $(MODEL_BIN)/asp_fault $(MODEL_BIN)/asp_gms \
$(MODEL_BIN)/asp_amf $(MODEL_BIN)/asp_logd

ifeq ($(CM_BUILD), 1)
    all: $(ASP_EXE) $(MODEL_BIN)/asp_cm
else
    all: $(ASP_EXE) 
endif

libs: all

$(MODEL_BIN)/asp_txn: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -L/opt/clovis/buildtools/ppc-linux-yd26/lib $(LDLIBS) -lCltxnServer \
$(LDLIBS) -lClUtils -lClGround

$(MODEL_BIN)/asp_logd: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lCllogdServer $(LDLIBS) -lClGround

$(MODEL_BIN)/asp_alarm: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClalarmServer $(LDLIBS) -lClGround

$(MODEL_BIN)/asp_ckpt: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClckptServer -lClCkptXdr \
$(LDLIBS) -lClGround

$(MODEL_BIN)/asp_cor: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClcorServer $(LDLIBS) -lClGround

$(MODEL_BIN)/asp_amf: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group  -lClAmsXdr -lClAmsParser \
-lClAmsMgmtStandalone -lClAmsServer -lClCm -lClamfServer  \
-Wl,--end-group $(LDLIBS) -lClGround 

$(MODEL_BIN)/asp_debug: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lCldebugServer $(LDLIBS) -lClGround

$(MODEL_BIN)/asp_gms: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -laisexec -ltotem_pg -lClgmsServer \
    $(LDLIBS) -laisexec -ltotem_pg -lClGround

$(MODEL_BIN)/asp_event: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lCleventServer $(LDLIBS) -lClGround

$(MODEL_BIN)/asp_fault: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClfaultServer \
$(LDLIBS) -lClfaultServer -lClGround

$(MODEL_BIN)/asp_name: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClnameServer $(LDLIBS) -lClGround

$(MODEL_BIN)/asp_cm: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@  -lglib-2.0 -lgthread-2.0 -lltdl -lopenhpi \
-lopenhpiutils $(LDLIBS) -lClcmServer -lClCm $(LDLIBS) -lClGround 

clean:
	rm -rf $(MODEL_BIN)/asp_alarm \
	$(MODEL_BIN)/asp_ckpt \
	$(MODEL_BIN)/asp_cor \
	$(MODEL_BIN)/asp_amf \
	$(MODEL_BIN)/asp_debug \
	$(MODEL_BIN)/asp_event \
	$(MODEL_BIN)/asp_gms \
	$(MODEL_BIN)/asp_fault \
	$(MODEL_BIN)/asp_txn \
	$(MODEL_BIN)/asp_logd \
	$(MODEL_BIN)/asp_name \
	$(MODEL_BIN)/asp_cm

include $(CLOVIS_ROOT)/SAFplus/mk/prefix.mk
