#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#if defined(__has_include)
#  if __has_include(<ncurses.h>)
#    include <ncurses.h>
#  elif __has_include(<curses.h>)
#    include <curses.h>
#  elif __has_include(<ncurses/curses.h>)
#    include <ncurses/curses.h>
#  else
#    error "No curses header found. Install the ncurses development package."
#  endif
#else
#  include <ncurses.h>
#endif
#include "battle.h"
#include "generation.h"
#include "ui.h"

static const char *trainerTypeName(classType type)
{
    switch (type) {
        case CLASS_HIKER:
            return "Hiker";
        case CLASS_RIVAL:
            return "Rival";
        case CLASS_PACER:
            return "Pacer";
        case CLASS_WANDERER:
            return "Wanderer";
        case CLASS_SENTRY:
            return "Sentry";
        case CLASS_EXPLORER:
            return "Explorer";
        case CLASS_PC:
            return "PC";
        default:
            return "Unknown";
    }
}

int normalizeInput(int input)
{
    if(input == '>'){
        return '>';
    }
    if(input == 'B'){
        return 'B';
    }

    switch (tolower(input)) {
        case 'w':
            return 'q';
        case 'e':
            return 'w';
        case 'r':
            return 'e';
        case 'd':
            return 'd';
        case 'c':
            return 'c';
        case 'x':
            return 's';
        case 'z':
            return 'z';
        case 'a':
            return 'a';
        case ' ':
            return '.';
        case '.':
            return '>';
        case 't':
            return 't';
        case 'f':
            return 'f';
        case 27:
            return 27;
        default:
            return ERR;
    }
}

void displayTrainerList(Map *m, int scrollOffset)
{
    int rowsPerPage = LINES - 2;

    clear();
    mvprintw(0, 0, "Trainer List  Esc: close  Up/PgUp: scroll up  Down/PgDn: scroll down");
    for (int i = 0; i < rowsPerPage && scrollOffset + i < numNPCs; i++) {
        Trainer *t = &m->trainers[scrollOffset + i];
        mvprintw(i + 1, 0, "%-10s Pos:(%2d,%2d) Defeated:%s",
                 trainerTypeName(t->typeC), t->x, t->y, t->defeated ? "Yes" : "No");
    }
    refresh();
}

static const char *genderName(int gender)
{
    return gender ? "Male" : "Female";
}

static void printPokemonName(Pokemon *p)
{
    if(p->shiny && has_colors()){
        attron(COLOR_PAIR(COLOR_PAIR_PC));
        printw("%s", p->name.c_str());
        attroff(COLOR_PAIR(COLOR_PAIR_PC));
    }
    else{
        printw("%s", p->name.c_str());
    }
}

static void printPokemonSummary(Pokemon *p, int row, int number)
{
    mvprintw(row, 0, "%d. ", number);
    printPokemonName(p);
    printw("  Lv:%d  %s", p->level, genderName(p->gender));

    mvprintw(row + 1, 3, "HP:%d  Atk:%d  Def:%d  SpA:%d  SpD:%d  Spe:%d",
             p->hp,
             p->attack,
             p->defense,
             p->specialAttack,
             p->specialDefense,
             p->speed);

    if(p->numMoves == 0){
        mvprintw(row + 2, 3, "Moves: none");
    }
    else if(p->numMoves == 1){
        mvprintw(row + 2, 3, "Moves: %s", p->moves[0].name.c_str());
    }
    else{
        mvprintw(row + 2, 3, "Moves: %s, %s",
                 p->moves[0].name.c_str(),
                 p->moves[1].name.c_str());
    }
}

void promptStarterSelection(Trainer *pcTrainer)
{
    Pokemon starters[3];
    int input = ERR;

    for(int i = 0; i < 3; i++){
        starters[i] = generatePokemonAtLevel(5);
    }

    nodelay(stdscr, FALSE);

    while(input != '1' && input != '2' && input != '3'){
        clear();
        mvprintw(0, 0, "Choose your starter Pokemon");
        mvprintw(1, 0, "Press 1, 2, or 3.");

        for(int i = 0; i < 3; i++){
            printPokemonSummary(&starters[i], 3 + i * 4, i + 1);
        }

        refresh();
        input = getch();
    }

    pcTrainer->party[0] = starters[input - '1'];
    pcTrainer->partySize = 1;

    nodelay(stdscr, TRUE);
    clear();
    refresh();
}

static void clearStatusRows()
{
    for(int row = MAP_HEIGHT; row < MAP_HEIGHT + 3; row++){
        move(row, 0);
        clrtoeol();
    }
}

int runBattlePlaceholder(Map *m, Trainer *pcTrainer)
{
    int result = runTrainerBattle(m, pcTrainer);
    invalidateMapDisplay();
    clearStatusRows();
    refresh();
    return result;
}

int runWildPokemonPlaceholder(Map *m, Trainer *pcTrainer, Pokemon *wildPokemon)
{
    int result = runWildBattle(m, pcTrainer, wildPokemon);
    invalidateMapDisplay();
    clearStatusRows();
    refresh();
    return result;
}

int promptFlyDestination(int *worldX, int *worldY)
{
    char inputBuffer[32] = "";
    int targetX;
    int targetY;

    nodelay(stdscr, FALSE);
    echo();
    curs_set(1);

    mvprintw(MAP_HEIGHT, 0, "Fly to map (x y) in range [-200, 200]. Esc cancels: ");
    clrtoeol();
    move(MAP_HEIGHT + 1, 0);
    clrtoeol();
    refresh();

    int input = getnstr(inputBuffer, static_cast<int>(sizeof(inputBuffer)) - 1);

    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);

    move(MAP_HEIGHT, 0);
    clrtoeol();
    move(MAP_HEIGHT + 1, 0);
    clrtoeol();
    refresh();

    if (input == ERR || strcmp(inputBuffer, "\x1b") == 0) {
        return 0;
    }

    if (sscanf(inputBuffer, "%d %d", &targetX, &targetY) != 2) {
        return 0;
    }

    if (targetX < -200 || targetX > 200 || targetY < -200 || targetY > 200) {
        return 0;
    }

    *worldX = targetX + 200;
    *worldY = targetY + 200;
    return 1;
}
