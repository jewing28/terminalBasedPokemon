#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <new>
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
#include "generation.h"
#include "pathfinding.h"
#include "trainer.h"
#include "queue.h"
#include "ui.h"
#include "parser.h"
#include "battle.h"

int numNPCs = 20;
Trainer pc;

static void clearStatusRows()
{
    for(int row = MAP_HEIGHT; row < MAP_HEIGHT + 3; row++){
        move(row, 0);
        clrtoeol();
    }
}

static void healParty(Trainer *trainer)
{
    for(int i = 0; i < trainer->partySize; i++){
        trainer->party[i].currentHp = trainer->party[i].hp;
    }
}

static void showMessageUntilEscape(const char *message)
{
    int input = ERR;

    clearStatusRows();
    mvprintw(MAP_HEIGHT, 0, "%s", message);
    refresh();

    while(input != 27){
        input = getch();
        napms(50);
    }
}

static void runPokemonCenter(Trainer *pcTrainer)
{
    int input = ERR;

    clearStatusRows();
    mvprintw(MAP_HEIGHT, 0, "Welcome to the Pokemon Center. Press 1 to heal, or 2 to cancel.");
    refresh();

    while(input != '1' && input != '2'){
        input = getch();
        napms(50);
    }

    if(input == '1'){
        healParty(pcTrainer);
        showMessageUntilEscape("Your Pokemon are fully healed. Press Escape to leave.");
    }
    else{
        showMessageUntilEscape("Come back anytime. Press Escape to leave.");
    }
}

static void placeTrainerOnPokemonCenter(Map *m, Trainer *trainer)
{
    if(!m){
        return;
    }

    for(int y = 0; y < MAP_HEIGHT; y++){
        for(int x = 0; x < MAP_WIDTH; x++){
            if(m->map[y][x] == 'C'){
                trainer->x = x;
                trainer->y = y;
                return;
            }
        }
    }
}

static void returnToStartingPokemonCenter(Map *world[401][401], int *worldX, int *worldY, Trainer *pcTrainer, Queue *queue, int *gameClock, int hikerDist[MAP_HEIGHT][MAP_WIDTH], int rivalDist[MAP_HEIGHT][MAP_WIDTH])
{
    if(world[*worldY][*worldX]){
        world[*worldY][*worldX]->mapClock = *gameClock;
    }
    *worldX = 200;
    *worldY = 200;

    if(!world[*worldY][*worldX]){
        world[*worldY][*worldX] = generateMap(*worldX, *worldY,
            world[(*worldY) - 1][*worldX],
            world[(*worldY) + 1][*worldX],
            world[*worldY][(*worldX) + 1],
            world[*worldY][(*worldX) - 1]);
    }

    placeTrainerOnPokemonCenter(world[*worldY][*worldX], pcTrainer);
    emptyQueue(queue);
    *gameClock = world[*worldY][*worldX]->mapClock;
    pcTrainer->nextTurn = *gameClock;
    add(queue, pcTrainer);
    fillQueue(queue, world[*worldY][*worldX]);
    invalidateMapDisplay();
    dijkstra(world[*worldY][*worldX]->map, pcTrainer->x, pcTrainer->y, hikerDist, TRAINER_HIKER);
    dijkstra(world[*worldY][*worldX]->map, pcTrainer->x, pcTrainer->y, rivalDist, TRAINER_RIVAL);
}

int main(int argc, char *argv[]){
    int parserStatus = handleParserCommand(argc, argv);
    if (parserStatus >= 0) {
        return parserStatus;
    }

    if (pokemonDatabase.load()) {
        fprintf(stderr, "Failed to load Pokemon data.\n");
        return 1;
    }

    srand(time(NULL));
    setlocale(LC_ALL, "");

    Map *world[401][401] = { nullptr };
    int worldX = 200;
    int worldY = 200;
    int gameClock = 0;
    int quitGame = 0;
    int input;
    int pendingInput = ERR;
    char statusMessage[128] = "";
    int hikerDist[MAP_HEIGHT][MAP_WIDTH];
    int rivalDist[MAP_HEIGHT][MAP_WIDTH];
    pc.typeT = TRAINER_PC;
    pc.typeC = CLASS_PC;
    pc.x = 40;
    pc.y = 10;
    pc.nextTurn = 0;
    pc.defeated = 0;
    pc.partySize = 0;
    restockTrainerBag(&pc);
    Queue queue;
    for(int i = 1; i < argc - 1; i++){
        if(strcmp(argv[i], "--numtrainers") == 0){
            numNPCs = atoi(argv[i+1]) > 2 ? atoi(argv[i+1]) : 2;
            break;
        }
    }
    queue.heap = new (std::nothrow) Trainer*[numNPCs + 1]();
    if (!queue.heap) {
        return 1;
    }
    queue.capacity = numNPCs + 1;
    queue.size = 0;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) {
        start_color();
        init_pair(COLOR_PAIR_PC, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_PAIR_AGGRESSIVE, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_PAIR_OTHER_NPC, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_PAIR_WATER, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_PAIR_TREES, COLOR_GREEN, COLOR_BLACK);
    }

    promptStarterSelection(&pc);
    
    world[worldY][worldX] = generateMap(worldX, worldY, world[worldY-1][worldX],world[worldY+1][worldX],world[worldY][worldX-1],world[worldY][worldX+1]);
    randomValidPosition(&pc, world[worldY][worldX]);
    gameClock = world[worldY][worldX]->mapClock;
    pc.nextTurn = gameClock;
    add(&queue, &pc);
    fillQueue(&queue, world[worldY][worldX]);
    displayMap(world[worldY][worldX], &pc);
    mvprintw(MAP_HEIGHT, 0, "%s", statusMessage);
    clrtoeol();
    refresh();
    dijkstra(world[worldY][worldX]->map, pc.x, pc.y, hikerDist, TRAINER_HIKER);
    dijkstra(world[worldY][worldX]->map, pc.x, pc.y, rivalDist, TRAINER_RIVAL);
    
    while(!quitGame){
        int polledInput = getch();
        while (polledInput != ERR) {
            if (tolower(polledInput) == 'q') {
                quitGame = 1;
                break;
            }
            polledInput = normalizeInput(polledInput);
            if (polledInput != ERR) {
                pendingInput = polledInput;
            }
            polledInput = getch();
        }

        if (quitGame) {
            break;
        }

        if (queue.size == 0) {
            continue;
        }

        if (queue.heap[0]->nextTurn > gameClock) {
            //mvprintw(MAP_HEIGHT, 0, "PLAYER INPUT: ");
            clrtoeol();
            refresh();
            gameClock++;
            napms(10);
            continue;
        }

        while (queue.size > 0 && queue.heap[0]->nextTurn <= gameClock) {
            Trainer *current = queueRemove(&queue);
            if(current->typeT == TRAINER_PC){
                //mvprintw(MAP_HEIGHT, 0, "PLAYER INPUT: ");
                clrtoeol();
                refresh();
                input = pendingInput;
                pendingInput = ERR;

                if(input == 'q' || input == 'w' || input == 'e' ||
                   input == 'a' || input == 'd' ||
                   input == 'z' || input == 's' || input == 'c'){
                    char exitDir = '\0';
                    int nm = movePC(input, &pc, world[worldY][worldX], &exitDir);
                    if(nm == 0){
                        world[worldY][worldX]->mapClock = gameClock;
                        changeMaps(exitDir, &pc, world, &worldX, &worldY);
                        emptyQueue(&queue);
                        gameClock = world[worldY][worldX]->mapClock;
                        pc.nextTurn = gameClock;
                        add(&queue, &pc);
                        fillQueue(&queue, world[worldY][worldX]);
                        input = 0;
                        statusMessage[0] = '\0';
                        invalidateMapDisplay();
                        dijkstra(world[worldY][worldX]->map, pc.x, pc.y, hikerDist, TRAINER_HIKER);
                        dijkstra(world[worldY][worldX]->map, pc.x, pc.y, rivalDist, TRAINER_RIVAL);
                        break;
                    }
                    else{
                        pc.nextTurn += nm;
                        statusMessage[0] = '\0';
                        if(world[worldY][worldX]->map[pc.y][pc.x] == ':' && rand() % 100 < 10){
                            Pokemon wildPokemon = generatePokemonForMap(worldX, worldY);
                            if(runWildPokemonPlaceholder(world[worldY][worldX], &pc, &wildPokemon) == -1){
                                returnToStartingPokemonCenter(world, &worldX, &worldY, &pc, &queue, &gameClock, hikerDist, rivalDist);
                            }
                        }
                    }
                    dijkstra(world[worldY][worldX]->map, pc.x, pc.y, hikerDist, TRAINER_HIKER);
                    dijkstra(world[worldY][worldX]->map, pc.x, pc.y, rivalDist, TRAINER_RIVAL);
                }
                else if(input == '>'){
                    char currentTile = world[worldY][worldX]->map[pc.y][pc.x];
                    if(currentTile == 'M'){
                        restockTrainerBag(&pc);
                        snprintf(statusMessage, sizeof(statusMessage), "PokeMart restocked your bag. Press Escape to leave.");
                    }
                    if(currentTile == 'M'){
                        int buildingInput = ERR;
                        displayMap(world[worldY][worldX], &pc);
                        mvprintw(MAP_HEIGHT, 0, "%s", statusMessage);
                        clrtoeol();
                        refresh();
                        while(buildingInput != 27){
                            buildingInput = getch();
                            napms(50);
                        }
                        statusMessage[0] = '\0';
                        pc.nextTurn += 10;
                    }
                    else if(currentTile == 'C'){
                        displayMap(world[worldY][worldX], &pc);
                        runPokemonCenter(&pc);
                        statusMessage[0] = '\0';
                        pc.nextTurn += 10;
                    }
                    else{
                        statusMessage[0] = '\0';
                        pc.nextTurn += 1;
                    }
                }
                else if(input == 't'){
                    int listInput = ERR;
                    int scrollOffset = 0;
                    int rowsPerPage = LINES - 2;
                    int maxOffset = numNPCs > rowsPerPage ? numNPCs - rowsPerPage : 0;

                    displayTrainerList(world[worldY][worldX], scrollOffset);
                    while(listInput != 27){
                        listInput = getch();
                        if (listInput == KEY_UP) {
                            scrollOffset = scrollOffset > 0 ? scrollOffset - 1 : 0;
                            displayTrainerList(world[worldY][worldX], scrollOffset);
                        }
                        else if (listInput == KEY_DOWN) {
                            scrollOffset = scrollOffset < maxOffset ? scrollOffset + 1 : maxOffset;
                            displayTrainerList(world[worldY][worldX], scrollOffset);
                        }
                        else if (listInput == KEY_PPAGE) {
                            scrollOffset -= rowsPerPage;
                            if (scrollOffset < 0) {
                                scrollOffset = 0;
                            }
                            displayTrainerList(world[worldY][worldX], scrollOffset);
                        }
                        else if (listInput == KEY_NPAGE) {
                            scrollOffset += rowsPerPage;
                            if (scrollOffset > maxOffset) {
                                scrollOffset = maxOffset;
                            }
                            displayTrainerList(world[worldY][worldX], scrollOffset);
                        }
                        napms(50);
                    }
                    invalidateMapDisplay();
                    statusMessage[0] = '\0';
                    pc.nextTurn += 1;
                }
                else if(input == 'f'){
                    int targetWorldX = worldX;
                    int targetWorldY = worldY;

                    if (promptFlyDestination(&targetWorldX, &targetWorldY)) {
                        world[worldY][worldX]->mapClock = gameClock;
                        worldX = targetWorldX;
                        worldY = targetWorldY;
                        if (!world[worldY][worldX]) {
                            world[worldY][worldX] = generateMap(worldX, worldY,
                                worldY == 0 ? nullptr : world[worldY - 1][worldX],
                                worldY == 400 ? nullptr : world[worldY + 1][worldX],
                                worldX == 400 ? nullptr : world[worldY][worldX + 1],
                                worldX == 0 ? nullptr : world[worldY][worldX - 1]);
                        }
                        randomValidPosition(&pc, world[worldY][worldX]);
                        emptyQueue(&queue);
                        gameClock = world[worldY][worldX]->mapClock;
                        pc.nextTurn = gameClock;
                        add(&queue, &pc);
                        fillQueue(&queue, world[worldY][worldX]);
                        invalidateMapDisplay();
                        statusMessage[0] = '\0';
                        dijkstra(world[worldY][worldX]->map, pc.x, pc.y, hikerDist, TRAINER_HIKER);
                        dijkstra(world[worldY][worldX]->map, pc.x, pc.y, rivalDist, TRAINER_RIVAL);
                        input = 0;
                        break;
                    }

                    snprintf(statusMessage, sizeof(statusMessage), "Fly canceled or invalid coordinates.");
                    pc.nextTurn += 1;
                }
                else if(input == 'B'){
                    displayMap(world[worldY][worldX], &pc);
                    openBagMenu(&pc);
                    statusMessage[0] = '\0';
                    pc.nextTurn += 10;
                }
                else{
                    statusMessage[0] = '\0';
                    pc.nextTurn += 1;
                }

                input = 0;
                add(&queue, current);
            }
            else{
                if (!current->defeated) {
                    current->nextTurn += moveNPC(current, world[worldY][worldX], current->typeC == CLASS_HIKER ? hikerDist : current->typeC == CLASS_RIVAL ? rivalDist : nullptr);
                    add(&queue, current);
                }
            }
        }
        displayMap(world[worldY][worldX], &pc);
        refresh();
        if(runBattlePlaceholder(world[worldY][worldX], &pc) == -1){
            returnToStartingPokemonCenter(world, &worldX, &worldY, &pc, &queue, &gameClock, hikerDist, rivalDist);
        }
        displayMap(world[worldY][worldX], &pc);
        mvprintw(MAP_HEIGHT, 0, "%s", statusMessage);
        clrtoeol();
        refresh();
        napms(50);
        /*displayMap(world[worldY][worldX], &pc);
        printf("Type: %d, Class: %d, Char: %c, Pos: (%d,%d), NextTurn: %d\n",
               current->typeT, current->typeC, getNPC(current->x, current->y, world[worldY][worldX]),
               current->x, current->y, current->nextTurn);*/
        //usleep(2500000); //================== uncomment before submit
    }

    int row = 0;
    int col = 0;
    for(row = 0; row < 401; row++){
        for(col = 0; col < 401; col++){
            if(world[row][col]){
                freeMap(world[row][col]);
                world[row][col] = nullptr;
            }
        }
    }

    delete[] queue.heap;
    endwin();

    return 0;
}

int terrainCost(Trainer *t, Map *m){
    if(!t || !m){
        return INF;
    }
    return getCost(m->map[t->y][t->x], t->typeT, t->x, t->y);
}
