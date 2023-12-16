#ifndef HEY_RAND_H
#define HEY_RAND_H

#include <stdint.h>
#define RND_U32 uint32_t
#define RND_U64 uint64_t
#include "rnd.h"
#define HEY_RAND_OUT_TYPE uint32_t
#define HEY_RAND_STATE_TYPE rnd_pcg_t
#define HEY_RAND_INIT(STATE, SEED) rnd_pcg_seed(STATE, SEED)
#define HEY_RAND_NEXT(STATE) rnd_pcg_next(STATE, SEED)
#define HEY_RAND_MAX UINT32_MAX

#endif

#ifdef HEY_IMPLEMENTATION

#define RND_IMPLEMENTATION
#include "rnd.h"

#endif
