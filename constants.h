#ifndef CONSTANTS_H
#define CONSTANTS_H

class Trainer;

// Map Constants and struct
#define MAP_HEIGHT 21
#define MAP_WIDTH 80

extern int numNPCs;

class Map {
public:
    int gates[4];
    char map[MAP_HEIGHT][MAP_WIDTH];
    Trainer *trainers;
    int mapClock;
};

// Path Finding Constants
#define INF 1000000000

#endif
