/*
 * Copyright (C) 2006 Red Hat, Inc.
 * Copyright (c) 2006 Sun Microsystems, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@mvista.com)
 *
 * This software licensed under BSD license, the text of which follows:
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AIS_MAR_GEN_H_DEFINED
#define AIS_MAR_GEN_H_DEFINED

#ifndef OPENAIS_SOLARIS
#include <stdint.h>
#else
#include <sys/types.h>
#endif
#include <string.h>

#include <saAis.h>
#include <swab.h>

typedef int8_t mar_int8_t;
typedef int16_t mar_int16_t;
typedef int32_t mar_int32_t;
typedef int64_t mar_int64_t;

typedef uint8_t mar_uint8_t;
typedef uint16_t mar_uint16_t;
typedef uint32_t mar_uint32_t;
typedef uint64_t mar_uint64_t;

static inline void swab_mar_int8_t (mar_int8_t *to_swab)
{
	return;
}

static inline void swab_mar_int16_t (mar_int16_t *to_swab)
{
	*to_swab = swab16 (*to_swab);
}

static inline void swab_mar_int32_t (mar_int32_t *to_swab)
{
	*to_swab = swab32 (*to_swab);
}

static inline void swab_mar_int64_t (mar_int64_t *to_swab)
{
	*to_swab = swab64 (*to_swab);
}

static inline void swab_mar_uint8_t (mar_uint8_t *to_swab)
{
	return;
}

static inline void swab_mar_uint16_t (mar_uint16_t *to_swab)
{
	*to_swab = swab16 (*to_swab);
}

static inline void swab_mar_uint32_t (mar_uint32_t *to_swab)
{
	*to_swab = swab32 (*to_swab);
}

static inline void swab_mar_uint64_t (mar_uint64_t *to_swab)
{
	*to_swab = swab64 (*to_swab);
}

typedef struct {
	mar_uint16_t length __attribute__((aligned(8)));
	mar_uint8_t value[SA_MAX_NAME_LENGTH] __attribute__((aligned(8)));
} mar_name_t;

static inline void swab_mar_name_t (mar_name_t *to_swab)
{
	swab_mar_uint16_t (&to_swab->length);
}

static inline void marshall_from_mar_name_t (
	SaNameT *dest,
	mar_name_t *src)
{
	dest->length = src->length;
	memcpy (dest->value, src->value, SA_MAX_NAME_LENGTH);
}

static inline void marshall_to_mar_name_t (
	mar_name_t *dest,
	SaNameT *src)
{
	dest->length = src->length;
	memcpy (dest->value, src->value, SA_MAX_NAME_LENGTH);
}

typedef enum {
	MAR_FALSE = 0,
	MAR_TRUE = 1
} mar_bool_t;

typedef mar_uint64_t mar_time_t;

static inline void swab_mar_time_t (mar_time_t *to_swab)
{
	swab_mar_uint64_t (to_swab);
}

#define MAR_TIME_END ((SaTimeT)0x7fffffffffffffffull)
#define MAR_TIME_BEGIN            0x0ULL
#define MAR_TIME_UNKNOWN          0x8000000000000000ULL

#define MAR_TIME_ONE_MICROSECOND 1000ULL
#define MAR_TIME_ONE_MILLISECOND 1000000ULL
#define MAR_TIME_ONE_SECOND      1000000000ULL
#define MAR_TIME_ONE_MINUTE      60000000000ULL
#define MAR_TIME_ONE_HOUR        3600000000000ULL
#define MAR_TIME_ONE_DAY         86400000000000ULL
#define MAR_TIME_MAX             SA_TIME_END

#define MAR_TRACK_CURRENT 0x01
#define MAR_TRACK_CHANGES 0x02
#define MAR_TRACK_CHANGES_ONLY 0x04

typedef mar_uint64_t mar_invocation_t;

static inline void swab_mar_invocation_t (mar_invocation_t *to_swab)
{
	swab_mar_uint64_t (to_swab);
}

typedef mar_uint64_t mar_size_t;

static inline void swab_mar_size_t (mar_size_t *to_swab)
{
	swab_mar_uint64_t (to_swab);
}
#endif /* AIS_MAR_GEN_TYPES_H_DEFINED */
