#include <ncurses.h>
#include "ipc/IpcClient.h"

/*
 *  PID | UnitName | Status | Description
 */
struct TableDefinition
{
    const char *header;
    const int size;
};

static const TableDefinition table_defs[] = {
        { "PID", 6 },
        { "Name", 15 },
        { "Status", 10 }, // RUNNING | STOPPED
        { "Description", 20 }
};

static int SCREEN_WIDTH;
static int SCREEN_HEIGHT;

void create_help()
{
    mvaddstr(SCREEN_HEIGHT - 1, 0, "(h)elp, (l)ogs, (r)eload nysd, (k)ill unit, (s)tart unit, (i)nfo, (q)uit");
    refresh();
}

void create_table_headers()
{
    // Create table headers
    int current_x = 0;
    for(int i = 0; i < sizeof(table_defs); i++)
    {
        mvaddstr(0, current_x, table_defs[i].header);
        current_x += table_defs[i].size;
    }

    refresh();
}

void interactive()
{
    clear();

    create_help();
    create_table_headers();

    IpcClient client;
    client.Connect("/Users/nathan/.nys/nys.sock");

    char ch;
    while(true)
    {
        ch = getch();

        if(ch == 'q') break;
        if(ch == 'r')
        {
            client.SendMessage({
                .type = MESSAGE_RELOAD
            });
        }
    }

    clear();
}

void init_curses()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    SCREEN_WIDTH = COLS;
    SCREEN_HEIGHT = LINES;
}

int main()
{
    // Start
    init_curses();
    interactive();
    return 0;
}