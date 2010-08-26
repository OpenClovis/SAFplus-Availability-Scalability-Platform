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
* ModuleName  : hpiEventFilter
* File        : clSysComp0eventFilter.c
*******************************************************************************/

/*******************************************************************************
* Description :
* This file hold the HPI Event filters .
*
*******************************************************************************/
#include <clHpiEventFilter.h>
#include "clSysComp0eventHandler.h"


ClHpiSensorEventFilterT sensor_filter[]= {
                                { -1, SAHPI_TEMPERATURE, SAHPI_EC_THRESHOLD, NULL}, 
                                { -1, SAHPI_TEMPERATURE, SAHPI_EC_THRESHOLD, "Temp_In Left"},
                                { -1, SAHPI_TEMPERATURE, SAHPI_EC_THRESHOLD, "Temp_In Center"},
                                { -1, SAHPI_TEMPERATURE, SAHPI_EC_THRESHOLD, "Temp_In Right"},
                                { -1, SAHPI_PROCESSOR, SAHPI_EC_THRESHOLD, NULL},
                                {-1, SAHPI_VOLTAGE, SAHPI_EC_THRESHOLD, NULL},
                                {-1, SAHPI_CURRENT, SAHPI_EC_THRESHOLD, NULL}
};



ClHpiHotswapEventFilterT hotswap_filter[]={
		{SAHPI_HS_STATE_EXTRACTION_PENDING,SAHPI_HS_STATE_ACTIVE},
		{SAHPI_HS_STATE_NOT_PRESENT,SAHPI_HS_STATE_ACTIVE},
		{SAHPI_HS_STATE_ACTIVE,SAHPI_HS_STATE_EXTRACTION_PENDING},
		{SAHPI_HS_STATE_INACTIVE,SAHPI_HS_STATE_EXTRACTION_PENDING},
		{SAHPI_HS_STATE_NOT_PRESENT,SAHPI_HS_STATE_EXTRACTION_PENDING},
		{SAHPI_HS_STATE_NOT_PRESENT,SAHPI_HS_STATE_INACTIVE},
		{SAHPI_HS_STATE_INSERTION_PENDING,SAHPI_HS_STATE_INACTIVE},
		{SAHPI_HS_STATE_INACTIVE,SAHPI_HS_STATE_INSERTION_PENDING},
		{SAHPI_HS_STATE_INSERTION_PENDING,SAHPI_HS_STATE_NOT_PRESENT},
		{SAHPI_HS_STATE_NOT_PRESENT,SAHPI_HS_STATE_INSERTION_PENDING},
		{SAHPI_HS_STATE_ACTIVE,SAHPI_HS_STATE_INSERTION_PENDING}
};
		
		
                                    
ClHpiEventFilterT test_filter[] = {
										
{SAHPI_ENT_COOLING_DEVICE, {{{SAHPI_ENT_UNSPECIFIED, 0}}}, 
		PM_HPI_SENSOR_EVENT_FILTER, (void *)&sensor_filter[1], -1, fantray_callback },
{SAHPI_ENT_COOLING_DEVICE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_SENSOR_EVENT_FILTER, (void *)&sensor_filter[2], -1, fantray_callback },
{SAHPI_ENT_COOLING_DEVICE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_SENSOR_EVENT_FILTER, (void *)&sensor_filter[3], -1, fantray_callback },
{SAHPI_ENT_COOLING_UNIT, {{{SAHPI_ENT_UNSPECIFIED, 0}}}, 
		PM_HPI_SENSOR_EVENT_FILTER, (void *)&sensor_filter[1], -1, fantray_callback },
{SAHPI_ENT_COOLING_UNIT, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_SENSOR_EVENT_FILTER, (void *)&sensor_filter[2], -1, fantray_callback },
{SAHPI_ENT_COOLING_UNIT, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_SENSOR_EVENT_FILTER, (void *)&sensor_filter[3], -1, fantray_callback },
/* HotSwap Event Filter */
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
		PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[0], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[1], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[2], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[3], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[4], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[5], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
		PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[6], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[7], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[8], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[9], -1, hotswap_callback },
{SAHPI_ENT_SBC_BLADE, {{{SAHPI_ENT_UNSPECIFIED, 0}}},
	   	PM_HPI_HOTSWAP_EVENT_FILTER, (void *)&hotswap_filter[10], -1, hotswap_callback }
};



ClUint32T getNumberOfFilters(void)
{
		return(sizeof(test_filter)/sizeof(ClHpiEventFilterT));

}
