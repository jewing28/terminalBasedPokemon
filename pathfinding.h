#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "constants.h"
#include "trainer.h"

int getCost(char tile, trainerType type, int x, int y);
void dijkstra(char map[MAP_HEIGHT][MAP_WIDTH], int startX, int startY, int dist[MAP_HEIGHT][MAP_WIDTH], trainerType type);
void displayDistanceTable(int dist[MAP_HEIGHT][MAP_WIDTH]);

#endif
