#ifndef _CONTINUATION_PLAYER_H_
#define _CONTINUATION_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "continuation.h"

#define CONTINUATIONS_MAX (1024)

extern void continuation_player(void);
extern void mark_continuation(int cont);
extern void unmark_continuation(int cont);

#ifdef __cplusplus
}
#endif

#endif
