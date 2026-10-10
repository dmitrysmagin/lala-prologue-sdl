#ifndef ENGINE_FLIP_H
#define ENGINE_FLIP_H

#include "engine_if.h"

/* Flip-screen engine ("original"): 6x6 grid of 20x12 screens, instant
 * transitions on edge attempt. Fills e with the flip vtable. */
void flip_engine_init(Engine *e);

/* Loop sub-steps, exposed for unit tests. */
void flip_transition(Engine *e);
void flip_collect(Engine *e);

#endif
