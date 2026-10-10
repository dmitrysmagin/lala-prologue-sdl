#ifndef STATE_TITLE_H
#define STATE_TITLE_H

/* Title screen (mirrors showTitle): TITLE.PCX + fade-in, G66A music,
 * engine toggle (LEFT/RIGHT), blinking PRESS ENTER. Returns 1 = start
 * game, 0 = quit (ESC / window close). Toggles g_config.scroll_enabled. */
struct Game;
int show_title(struct Game *g);

#endif
