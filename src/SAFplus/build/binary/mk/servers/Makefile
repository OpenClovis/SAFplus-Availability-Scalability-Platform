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
# ModuleName  : binary
# File        : Makefile
################################################################################
# Description :
################################################################################

DEP_LIBS	:= -lClTimer -lClBuffer -lClOsal -lClIoc -lClUtils -lClEo -lClMain\
-lClLogClient -lClTxnClient -lClTxnAgent -lClSm -lClCnt -lClIdl -lClRmd \
-lClCorClient -lClAmfClient -lClAmsMgmt -lClAmsXdr -lClDebugClient -lClFaultClient \
-lClEventClient -lClDbal -lClGms -lClCkpt -lClMedClient -lClNameClient \
-lClUtils -lClIdlPtr -lClAlarmClient -lClAlarmUtils -lClAlarmXdr -lClOmClient \
-l3rdpartyutils

LD_LIBS		:= -L$(MODEL_LIB) -lClConfig -L$(ASP_LIBDIR) $(DEP_LIBS) 

ifdef SOLARIS_BUILD
   SYS_LIBS	:= -lpthread -lrt  -L/usr/local/BerkeleyDB.4.2/lib -ldb -lm 
else
    ifeq ($(TARGET_QNX), 1)
        SYS_LIBS := -lm
    else
        SYS_LIBS := -lpthread -lrt -ldb -lm
    endif
endif

ifeq ($(BUILD_TIPC_COMPRESSION), 1)
	SYS_LIBS += -lz
endif

ifeq ($(TARGET_QNX), 1)
    THIRD_PARTY_LIBS := -lezxml -l3rdpartyutils 
else
    THIRD_PARTY_LIBS := -lezxml -lgdbm -l3rdpartyutils -ldl
endif

ifeq ($(HCL_CM),1)
	THIRD_PARTY_LIBS += -L$(TOOLCHAIN_DIR)/lib -lhcl -lopenhpiutils
else
    ifeq ($(CM_BUILD),1)
	THIRD_PARTY_LIBS += -L$(TOOLCHAIN_DIR)/lib -lopenhpi -lopenhpiutils
    else
	THIRD_PARTY_LIBS += -L$(TOOLCHAIN_DIR)/lib
    endif
endif

ASPLIBDIR = $(ASP_LIBDIR)
LDFLAGS += $(EXTRA_LDFLAGS) -L$(ASPLIBDIR)

LDLIBS	+= $(LDFLAGS) $(LD_LIBS) $(SYS_LIBS) $(THIRD_PARTY_LIBS) 

ASP_SERVERS := $(MODEL_BIN)/safplus_txn $(MODEL_BIN)/safplus_alarm $(MODEL_BIN)/safplus_ckpt \
$(MODEL_BIN)/safplus_debug $(MODEL_BIN)/safplus_event $(MODEL_BIN)/safplus_name \
$(MODEL_BIN)/safplus_cor $(MODEL_BIN)/safplus_fault $(MODEL_BIN)/safplus_gms \
$(MODEL_BIN)/safplus_amf $(MODEL_BIN)/safplus_logd $(MODEL_BIN)/safplus_msg 


ifeq ($(CM_BUILD), 1)
        ASP_SERVERS += $(MODEL_BIN)/safplus_cm
        CM_LIB := -lClCm
else
        CM_LIB :=
endif

all: $(ASP_SERVERS)

.PHONY: foo

libs:

$(MODEL_BIN)/safplus_txn: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lCltxnServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClConfig -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_logd: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lCllogdServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_alarm: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lClalarmServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_ckpt: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lClckptServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_cor: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lClcorServer -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_amf: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDFLAGS) $(SYS_LIBS) $(THIRD_PARTY_LIBS) -L$(MODEL_LIB) -L$(ASP_LIBDIR) \
		-lClConfig -lClAmfClient -lClCorClient  \
	  	-lClAmsXdr -lClAmsParser -lClAmsMgmt -lClAmsServer -lClamfServer  \
	  	-lClEo -lClTimer -lClBuffer -lClOsal -lClCnt -lClIoc -lClRmd -lClDbal       \
	  	-lClUtils -lClEventClient -lClDebugClient -lClNameClient -lClIdl -lClIdlPtr \
	  	-lClCkpt -lClLogClient -lClFaultClient -lClGms $(CM_LIB) -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_debug: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lCldebugServer -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_gms: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -laisexec -ltotem_pg -lClgmsServer \
	-laisexec -ltotem_pg -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_event: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lCleventServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_fault: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lClfaultServer -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_name: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lClnameServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround -Wl,--end-group
    
$(MODEL_BIN)/safplus_msg: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -Wl,--start-group,--no-as-needed $(LDLIBS) -lClmsgServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround -Wl,--end-group

$(MODEL_BIN)/safplus_cm: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@  -Wl,--start-group,--no-as-needed $(LDLIBS) -lglib-2.0 -lgthread-2.0 -lltdl  \
	-lClcmServer $(CM_LIB) -lClGroundClient -lClGround -Wl,--end-group

clean:
	rm -rf $(MODEL_BIN)/safplus_alarm \
	$(MODEL_BIN)/safplus_ckpt \
	$(MODEL_BIN)/safplus_cor \
	$(MODEL_BIN)/safplus_amf \
	$(MODEL_BIN)/safplus_debug \
	$(MODEL_BIN)/safplus_event \
	$(MODEL_BIN)/safplus_gms \
	$(MODEL_BIN)/safplus_fault \
	$(MODEL_BIN)/safplus_txn \
	$(MODEL_BIN)/safplus_logd \
	$(MODEL_BIN)/safplus_name \
	$(MODEL_BIN)/safplus_cm \
	$(MODEL_BIN)/safplus_msg

include $(CLOVIS_ROOT)/ASP/mk/make-cross.mk
