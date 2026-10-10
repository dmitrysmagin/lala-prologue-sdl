#ifndef ENGINE_SCROLL_H
#define ENGINE_SCROLL_H

#include "engine_if.h"

/* Smooth-scrolling engine: full-map world buffer + camera follow.
 * e->scroll must point at a ScrollState (world.h). Fills the scroll vtable. */
void scroll_engine_init(Engine *e);

/* Loop sub-step, exposed for unit tests. */
void scroll_collect(Engine *e);

#endif
