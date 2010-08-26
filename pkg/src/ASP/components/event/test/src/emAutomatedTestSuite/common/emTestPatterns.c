/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/common/emTestPatterns.c $
 * $Author: bkpavan $
 * $Date: 2006/09/13 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <clEventApi.h>
#include <stdint.h>
#include "emTestPatterns.h"

/*** Filters & Patterns ***/

/*
 ** Use the following Macros with caution. The default version expects 
 ** an initialized array as an argument. One can have array of size 1 
 ** to use this version. E.g "Strings", intarray[1], longarray[4], etc.
 */
#define CL_EVT_PATTERN_SET(PATTERN)         { 0, sizeof(PATTERN), (ClUint8T*)PATTERN }
#define CL_EVT_FILTER_SET(FLAG, FILTER)     { FLAG,  CL_EVT_PATTERN_SET(FILTER) }

/*
 ** For other types u can use _VAR version. Note that it uses '&'. 
 ** Do not pass conflicting types. This won't accept constant arguments.
 */
#define CL_EVT_VAR_PATTERN_SET(PATTERN)     { 0, sizeof(PATTERN), (ClUint8T*)&PATTERN }
#define CL_EVT_VAR_FILTER_SET(FLAG, FILTER) { FLAG,  CL_EVT_VAR_PATTERN_SET(FILTER) }

#define CL_EVT_PATTERN_ARRAY_SET(PATTERNS)  { 0, sizeof(PATTERNS)/sizeof(ClEventPatternT), PATTERNS }
#define CL_EVT_FILTER_ARRAY_SET(FILTERS)    { sizeof(FILTERS)/sizeof(ClEventFilterT), FILTERS }

#define gPatt1      "Filter pattern 11"
#define gPatt2      "Filter pattern 22"
#define gPatt3      "Filter pattern 33"
#define gPatt4      "Filter pattern 44"
#define gPatt5      "Filter pattern 55"

#define gStrPatt0   ""
#define gStrPatt1   "1"
#define gStrPatt2   "12"
#define gStrPatt3   "123"
#define gStrPatt4   "1234"
#define gStrPatt5   "12345"
#define gStrPatt6   "123456"
#define gStrPatt7   "1234567"
#define gStrPatt8   "12345678"
#define gStrPatt9   "123456789"

/*** Patterns ***/

static ClEventPatternT gDefaultPatterns[] = {
    CL_EVT_PATTERN_SET(gPatt1),
    CL_EVT_PATTERN_SET(gPatt2),
    CL_EVT_PATTERN_SET(gPatt3),
    CL_EVT_PATTERN_SET(gPatt4)
};

static ClEventPatternArrayT gDefaultPattern =
CL_EVT_PATTERN_ARRAY_SET(gDefaultPatterns);

static ClEventPatternT gDefaultPassAllPatterns[] = {
    CL_EVT_PATTERN_SET(gPatt1),
    CL_EVT_PATTERN_SET(gPatt2),
    CL_EVT_PATTERN_SET(gPatt3),
    CL_EVT_PATTERN_SET(gPatt5)
};

static ClEventPatternArrayT gDefaultPassAllPattern =
CL_EVT_PATTERN_ARRAY_SET(gDefaultPassAllPatterns);

static ClEventPatternT gStrPatterns[] = {
    CL_EVT_PATTERN_SET(gStrPatt1),
    CL_EVT_PATTERN_SET(gStrPatt2),
    CL_EVT_PATTERN_SET(gStrPatt3),
    CL_EVT_PATTERN_SET(gStrPatt4),
    CL_EVT_PATTERN_SET(gStrPatt5),
    CL_EVT_PATTERN_SET(gStrPatt6),
    CL_EVT_PATTERN_SET(gStrPatt7),
    CL_EVT_PATTERN_SET(gStrPatt8),
    CL_EVT_PATTERN_SET(gStrPatt9),
    CL_EVT_PATTERN_SET(gStrPatt0),
};

static ClEventPatternArrayT gStrPattern =
CL_EVT_PATTERN_ARRAY_SET(gStrPatterns);



typedef struct structPattern
{
    char c;
    uint32_t i;
    char str7[7];
    double d;
    long l;
    char str5[5];
    uint64_t ll;
    char str6[6];
    float f;
    uint16_t sh;
} ClEvtStructPatternT;

static ClEvtStructPatternT gStructPatternList[] = {
    {'Z', 0xAABBCCDD, "123456", 3.14, 0x12345678, "1234", 0x1122334455667788L,
     "12345", 2.178F, 0xEEFF},
    {' ', 0x11223344, "ABCDEF", 11.2, 0x0A0B0C0D, "ABCD", 0x0102030405060708L,
     "ABCDE", 5.678F, 0x2288},
};

static ClEventPatternT gStructPatterns[] = {
    CL_EVT_VAR_PATTERN_SET(gStructPatternList[0]),
    CL_EVT_VAR_PATTERN_SET(gStructPatternList[1])
};

static ClEventPatternArrayT gStructPattern =
CL_EVT_PATTERN_ARRAY_SET(gStructPatterns);


/*
 * Mixed Pattern 
 */

uint32_t gIntPatt = 0xAABBCCDD; /* 4 bytes */
uint16_t gShortPatt = 0x1234;   /* 2 bytes */
uint64_t gLongPatt = 0x1122334455667788;    /* 8 bytes */

float gFloatPatt = 3.14;
double gDoublePatt = 2.178;

static ClEventPatternT gMixedPatterns[] = {
    CL_EVT_VAR_PATTERN_SET(gShortPatt),
    CL_EVT_VAR_PATTERN_SET(gIntPatt),
    CL_EVT_VAR_PATTERN_SET(gLongPatt),
    CL_EVT_VAR_PATTERN_SET(gFloatPatt),
    CL_EVT_VAR_PATTERN_SET(gDoublePatt),
};

static ClEventPatternArrayT gMixedPattern =
CL_EVT_PATTERN_ARRAY_SET(gMixedPatterns);

static ClEventPatternT gNormalPatterns[] = {
    CL_EVT_PATTERN_SET(gStrPatt9),
    CL_EVT_PATTERN_SET(gStrPatt6),
    CL_EVT_PATTERN_SET(gStrPatt3),
    CL_EVT_PATTERN_SET(gStrPatt8),
    CL_EVT_PATTERN_SET(gStrPatt5),
    CL_EVT_PATTERN_SET(gStrPatt4),
    CL_EVT_PATTERN_SET(gStrPatt7),
};

static ClEventPatternArrayT gNormalPattern =
CL_EVT_PATTERN_ARRAY_SET(gNormalPatterns);

static ClEventPatternT gSupersetPatterns[] = {
    CL_EVT_PATTERN_SET(gStrPatt9),
    CL_EVT_PATTERN_SET(gStrPatt6),
    CL_EVT_PATTERN_SET(gStrPatt3),
    CL_EVT_PATTERN_SET(gStrPatt8),
    CL_EVT_PATTERN_SET(gStrPatt5),
    CL_EVT_PATTERN_SET(gStrPatt4),
    CL_EVT_PATTERN_SET(gStrPatt7),
    CL_EVT_PATTERN_SET(gStrPatt0),
    CL_EVT_PATTERN_SET(gStrPatt1),
};

static ClEventPatternArrayT gSupersetPattern =
CL_EVT_PATTERN_ARRAY_SET(gSupersetPatterns);

static ClEventPatternT gSubsetPatterns[] = {
    CL_EVT_PATTERN_SET(gStrPatt9),
    CL_EVT_PATTERN_SET(gStrPatt6),
    CL_EVT_PATTERN_SET(gStrPatt3),
    CL_EVT_PATTERN_SET(gStrPatt8),
};

static ClEventPatternArrayT gSubsetPattern =
CL_EVT_PATTERN_ARRAY_SET(gSubsetPatterns);

/*
 * Make an array of patterns for easier reference 
 */

ClPtrT gEmTestAppPatternDb[] = {
    &gDefaultPattern,   /* CL_EVT_DEFAULT_PATTERN */
    &gStrPattern,   /* CL_EVT_STR_PATTERN */
    &gStructPattern,    /* CL_EVT_STRUCT_PATTERN */
    &gMixedPattern, /* CL_EVT_MIXED_PATTERN */
    &gNormalPattern,    /* CL_EVT_NORMAL_PATTERN */
    &gSupersetPattern,  /* CL_EVT_SUPERSET_PATTERN */
    &gSubsetPattern,    /* CL_EVT_SUBSET_PATTERN */
    &gDefaultPassAllPattern,    /* CL_EVT_DEFAULT_PASS_ALL_PATTERN */
    &gNormalPattern,    /* CL_EVT_NORMAL_PATTERN */
    &gSubsetPattern,    /* CL_EVT_SUBSET_PATTERN */
};

ClSizeT gEmTestAppPatternDbSize =
    sizeof(gEmTestAppPatternDb) / sizeof(ClHandleT);

/*** Filters ***/

static ClEventFilterT gDefaultFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gPatt1),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gPatt2),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gPatt4),
};

static ClEventFilterT gDefaultPassAllFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gPatt1),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gPatt2),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gPatt4),
};

static ClEventFilterArrayT gDefaultFilter =
CL_EVT_FILTER_ARRAY_SET(gDefaultFilters);
static ClEventFilterArrayT gDefaultPassAllFilter =
CL_EVT_FILTER_ARRAY_SET(gDefaultPassAllFilters);

static ClEventFilterT gStrFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt1),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt2),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt4),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt5),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt6),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt7),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt8),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt9),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt0),
};

static ClEventFilterArrayT gStrFilter = CL_EVT_FILTER_ARRAY_SET(gStrFilters);

static ClEventFilterT gStructFilters[] = {
    CL_EVT_VAR_FILTER_SET(CL_EVENT_EXACT_FILTER, gStructPatternList[0]),
    CL_EVT_VAR_FILTER_SET(CL_EVENT_EXACT_FILTER, gStructPatternList[1]),
};

static ClEventFilterArrayT gStructFilter =
CL_EVT_FILTER_ARRAY_SET(gStructFilters);

static ClEventFilterT gMixedFilters[] = {
    CL_EVT_VAR_FILTER_SET(CL_EVENT_EXACT_FILTER, gShortPatt),
    CL_EVT_VAR_FILTER_SET(CL_EVENT_EXACT_FILTER, gIntPatt),
    CL_EVT_VAR_FILTER_SET(CL_EVENT_EXACT_FILTER, gLongPatt),
    CL_EVT_VAR_FILTER_SET(CL_EVENT_EXACT_FILTER, gFloatPatt),
    CL_EVT_VAR_FILTER_SET(CL_EVENT_EXACT_FILTER, gDoublePatt),
};

static ClEventFilterArrayT gMixedFilter =
CL_EVT_FILTER_ARRAY_SET(gMixedFilters);

static ClEventFilterT gNormalFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt9),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt6),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt8),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt5),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt4),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt7),
};

static ClEventFilterArrayT gNormalFilter =
CL_EVT_FILTER_ARRAY_SET(gNormalFilters);

static ClEventFilterT gNormalPassAllFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt9),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt6),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt8),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt5),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt4),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gStrPatt7),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gStrPatt4),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gStrPatt7),
};

static ClEventFilterArrayT gNormalPassAllFilter =
CL_EVT_FILTER_ARRAY_SET(gNormalPassAllFilters);

static ClEventFilterT gSupersetFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt9),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt6),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt8),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt5),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt4),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt7),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt0),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt1),
};

static ClEventFilterArrayT gSupersetFilter =
CL_EVT_FILTER_ARRAY_SET(gSupersetFilters);

static ClEventFilterT gSubsetFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt9),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt6),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt8),
};

static ClEventFilterArrayT gSubsetFilter =
CL_EVT_FILTER_ARRAY_SET(gSubsetFilters);

static ClEventFilterT gSubsetPassAllFilters[] = {
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt9),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt6),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_EXACT_FILTER, gStrPatt8),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gStrPatt6),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gStrPatt3),
    CL_EVT_FILTER_SET(CL_EVENT_PASS_ALL_FILTER, gStrPatt8),
};

static ClEventFilterArrayT gSubsetPassAllFilter =
CL_EVT_FILTER_ARRAY_SET(gSubsetPassAllFilters);

ClPtrT gEmTestAppFilterDb[] = {
    &gDefaultFilter,    /* CL_EVT_DEFAULT_PATTERN */
    &gStrFilter,    /* CL_EVT_STR_PATTERN */
    &gStructFilter, /* CL_EVT_STRUCT_PATTERN */
    &gMixedFilter,  /* CL_EVT_MIXED_PATTERN */
    &gNormalFilter, /* CL_EVT_NORMAL_PATTERN */
    &gSupersetFilter,   /* CL_EVT_SUPERSET_PATTERN */
    &gSubsetFilter, /* CL_EVT_SUBSET_PATTERN */
    &gDefaultPassAllFilter, /* CL_EVT_DEFAULT_PASS_ALL_PATTERN */
    &gNormalPassAllFilter,  /* CL_EVT_NORMAL_PASS_ALL_PATTERN */
    &gSubsetPassAllFilter,  /* CL_EVT_SUBSET_PASS_ALL_PATTERN */
};

ClSizeT gEmTestAppFilterDbSize = sizeof(gEmTestAppFilterDb) / sizeof(ClUint32T);
