/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : cor
 * File        : clCorStats.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * COR statistics module
 *
 *
 *****************************************************************************/

#ifndef _COR_STATS_H_
#define _COR_STATS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <clCommon.h>

struct corStats {
	ClUint32T	maxTrans;
	ClUint32T	maxChannel;
	ClUint32T  corMode;
    ClUint32T  inputEventCount;
    ClUint32T  upTime;
    ClUint32T  maxChannels;
    ClUint32T  nullMsgRxd;
    ClUint32T  nullPayload;
    ClUint32T  invalidChan;
    ClUint32T  validFSMEvents;
    ClUint32T  invalidFSMEvent;
    ClUint32T  helloAckRx;
    ClUint32T  helloAckTx;
    ClUint32T  helloTx;
};

typedef struct corStats CORStat_t;
typedef struct corStats *CORStat_h;


          /* ------------- proto types ----------------- */

extern ClRcT clCorStatisticsInitialize(void);
extern void   clCorStatisticsShow(char** params);
extern void clCorStatisticsFinalize(void);


#ifdef __cplusplus
}
#endif
#endif  /* _COR_STATS_H_ */
