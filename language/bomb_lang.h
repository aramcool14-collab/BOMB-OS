#ifndef BOMB_LANG_H
#define BOMB_LANG_H

typedef void (*bomb_lang_output_fn)(const char *message);

/* Runs one simple BOMB command. Returns 1 when recognized, 0 otherwise. */
int bomb_lang_run(const char *line, bomb_lang_output_fn output);

#endif
