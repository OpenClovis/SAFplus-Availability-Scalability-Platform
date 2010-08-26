################################################################################
#
#   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
# 
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is  free software; you can redistribute it and / or
# modify  it under  the  terms  of  the GNU General Public License
# version 2 as published by the Free Software Foundation.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You  should  have  received  a  copy of  the  GNU General Public
# License along  with  this program. If  not,  write  to  the 
# Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
################################################################################
#
# Build: 4.2.0
#
################################################################################
# ModuleName  : binary
# File        : Makefile
################################################################################
# Description :
################################################################################

#ifdef (CROSS_BUILD)
#LDFLAGS:=/opt/clovis/buildtools/ppc-linux-yd26/lib
#endif

DEP_LIBS	:= -lClTimer -lClBuffer -lClOsal -lClIoc -lClUtils -lClEo -lClMain\
-lClLogClient -lClTxnClient -lClTxnAgent -lClSm -lClCnt -lClIdl -lClRmd \
-lClCorClient -lClAmfClient -lClAmsMgmt -lClAmsXdr -lClDebugClient -lClFaultClient \
-lClEventClient -lClDbal -lClGms -lClCkpt -lClMedClient -lClNameClient \
-lClUtils -lClIdlPtr -lClAlarmClient -lClAlarmUtils -lClAlarmXdr -lClOmClient \
-lClMsgClient -l3rdpartyutils

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
LDFLAGS += -L$(ASPLIBDIR)

LDLIBS	+= $(LDFLAGS) $(LD_LIBS) $(SYS_LIBS) $(THIRD_PARTY_LIBS) 

ASP_SERVERS := $(MODEL_BIN)/asp_txn $(MODEL_BIN)/asp_alarm $(MODEL_BIN)/asp_ckpt \
$(MODEL_BIN)/asp_debug $(MODEL_BIN)/asp_event $(MODEL_BIN)/asp_name \
$(MODEL_BIN)/asp_cor $(MODEL_BIN)/asp_fault $(MODEL_BIN)/asp_gms \
$(MODEL_BIN)/asp_amf $(MODEL_BIN)/asp_logd $(MODEL_BIN)/asp_msg 


ifeq ($(CM_BUILD), 1)
        ASP_SERVERS += $(MODEL_BIN)/asp_cm
endif

all: $(ASP_SERVERS)

.PHONY: foo

libs:

$(MODEL_BIN)/asp_txn: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lCltxnServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClConfig -lClGroundClient -lClGround

$(MODEL_BIN)/asp_logd: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lCllogdServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround

$(MODEL_BIN)/asp_alarm: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClalarmServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround

$(MODEL_BIN)/asp_ckpt: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClckptServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround

$(MODEL_BIN)/asp_cor: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClcorServer $(LDLIBS) -lClGroundClient -lClGround

$(MODEL_BIN)/asp_amf: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ -L$(MODEL_LIB) -lClConfig -L$(ASPLIBDIR) -lClAmfClient -lClCorClient  \
              -lClAmsXdr -lClAmsParser -lClAmsMgmt -lClAmsServer -lClamfServer  \
              -lClEo -lClTimer -lClBuffer -lClOsal -lClCnt -lClIoc -lClRmd -lClDbal       \
              -lClUtils -lClEventClient -lClDebugClient -lClNameClient -lClIdl -lClIdlPtr \
              -lClCkpt -lClLogClient -lClFaultClient -lClGms -lClCm -L$(MODEL_LIB) -lClConfig \
              -L $(ASPLIBDIR) $(THIRD_PARTY_LIBS) $(SYS_LIBS) \
              -lClGroundClient -lClGround

$(MODEL_BIN)/asp_debug: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lCldebugServer $(LDLIBS) -lClGroundClient -lClGround

$(MODEL_BIN)/asp_gms: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -laisexec -ltotem_pg -lClgmsServer \
    $(LDLIBS) -laisexec -ltotem_pg -lClGroundClient -lClGround

$(MODEL_BIN)/asp_event: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lCleventServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround

$(MODEL_BIN)/asp_fault: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClfaultServer \
$(LDLIBS) -lClfaultServer -lClGroundClient -lClGround

$(MODEL_BIN)/asp_name: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClnameServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround
    
$(MODEL_BIN)/asp_msg: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@ $(LDLIBS) -lClmsgServer -lClConfig $(DEP_LIBS) $(DEP_LIBS) -lClGroundClient -lClGround

$(MODEL_BIN)/asp_cm: $(MODEL_LIB)/libClConfig.a
	$(CC) -o $@  -lglib-2.0 -lgthread-2.0 -lltdl  \
 $(LDLIBS) -lClcmServer -lClCm $(LDLIBS) -lClGroundClient -lClGround 

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
	$(MODEL_BIN)/asp_cm \
	$(MODEL_BIN)/asp_msg

include $(CLOVIS_ROOT)/ASP/mk/make-cross.mk
