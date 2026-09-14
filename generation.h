#ifndef GENERATION_H
#define GENERATION_H
#include "constants.h"
#include "trainer.h"

#define COLOR_PAIR_PC 1
#define COLOR_PAIR_AGGRESSIVE 2
#define COLOR_PAIR_OTHER_NPC 3
#define COLOR_PAIR_WATER 4
#define COLOR_PAIR_TREES 5

void placeStartingPoints(char map[MAP_HEIGHT][MAP_WIDTH]);
void growRegions(char map[MAP_HEIGHT][MAP_WIDTH]);
int  isFull(char map[MAP_HEIGHT][MAP_WIDTH]);
void placeBuilding(char map[MAP_HEIGHT][MAP_WIDTH], char building);
void makePathes(char map[MAP_HEIGHT][MAP_WIDTH], int exits[4]);
void placeBuildings(char map[MAP_HEIGHT][MAP_WIDTH]);
void initializeBorders(char map[MAP_HEIGHT][MAP_WIDTH]);
Map* generateMap(int x, int y, Map *north, Map *south, Map *east, Map *west);
void freeMap(Map *m);
void displayMap(Map *m, Trainer *pc);
void invalidateMapDisplay(void);
void fillRest(char map[MAP_HEIGHT][MAP_WIDTH]);
void changeMaps(char dir, Trainer *p, Map *world[401][401], int *wx, int *wy);
char getNPC(int x, int y, Map *m);

#endif
