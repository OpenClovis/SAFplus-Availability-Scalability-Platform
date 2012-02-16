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

SRC_FILES	= memLog.c

OBJ_FILES	= $(SRC_FILES:%.c=%.o)

CFLAGS	+= -g -Wall -fomit-frame-pointer

TARGET	= memLog

all: $(TARGET)

memLog: $(OBJ_FILES)
	$(CC) -o $@ $^
	@ cp $@ $(MODEL_BIN)
	@ cp memLog.sh $(MODEL_BIN)
	
lib:

%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ_FILES)
