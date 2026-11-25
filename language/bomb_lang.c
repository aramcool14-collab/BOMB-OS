#include "bomb_lang.h"

static int starts_with(const char *text, const char *prefix)
{
    while (*prefix) {
        if (*text++ != *prefix++) return 0;
    }
    return 1;
}

static void send(bomb_lang_output_fn output, const char *message)
{
    if (output) output(message);
}

int bomb_lang_run(const char *line, bomb_lang_output_fn output)
{
    if (!line) return 0;

    if (starts_with(line, "say ")) {
        const char *message = line + 4;
        if (*message == '"') {
            ++message;
            send(output, message);
        } else {
            send(output, message);
        }
        return 1;
    }

    if (starts_with(line, "open ")) {
        if (starts_with(line + 5, "calculator"))
            send(output, "OPEN:CALCULATOR");
        else if (starts_with(line + 5, "games"))
            send(output, "OPEN:GAMES");
        else if (starts_with(line + 5, "settings"))
            send(output, "OPEN:SETTINGS");
        else
            send(output, "I don't know that app yet. Try: calculator, games, settings.");
        return 1;
    }

    if (starts_with(line, "game ")) {
        if (starts_with(line + 5, "start"))
            send(output, "GAME:START");
        else if (starts_with(line + 5, "stop"))
            send(output, "GAME:STOP");
        else
            send(output, "Try: game start or game stop.");
        return 1;
    }

    if (starts_with(line, "help")) {
        send(output, "BOMB commands: say, open, game start, game stop, help.");
        return 1;
    }

    if (starts_with(line, "clear"))
    {
        send(output, "SCREEN:CLEAR");
        return 1;
    }

    send(output, "Unknown command. Type help.");
    return 0;
}
