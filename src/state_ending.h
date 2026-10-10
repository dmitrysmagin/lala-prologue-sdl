#ifndef STATE_ENDING_H
#define STATE_ENDING_H

/* Ending screen (mirrors showEnding): ENDING.PCX + MC12 music + typewriter
 * text, then wait for key. Returns 0 = key pressed (continue to title),
 * 1 = quit requested (window closed). */
struct Game;
int show_ending(struct Game *g);

#endif
