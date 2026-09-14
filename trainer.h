#ifndef TRAINER_H
#define TRAINER_H
#include "constants.h"
#include "pokemon.h"

// This is used for pathfinding
enum trainerType {
    TRAINER_PC,
    TRAINER_HIKER,
    TRAINER_RIVAL,
    TRAINER_SWIMMER,
    TRAINER_OTHER
};

//This is used for actual movement
enum classType {
    CLASS_PC,
    CLASS_HIKER,
    CLASS_RIVAL,
    CLASS_PACER,
    CLASS_WANDERER,
    CLASS_SENTRY,
    CLASS_EXPLORER
};

class Trainer {
public:
    int nextTurn;
    int x;
    int y;
    int defeated;
    trainerType typeT;
    classType typeC;
    int curDir; // Only used by Pacers, Wanderers, and Explorers [):Up, 1:Down, 2:Left, 3:Right]
    Pokemon party[TRAINER_MAX_POKEMON];
    int partySize;
    int potions;
    int revives;
    int pokeballs;
};

extern Trainer pc;

void randomValidPosition(Trainer *t, Map *m);
int checkNPCs(int x, int y, Map *m);
int checkNPCsPlayer(int x, int y, Map *m);
int movePC(char dir, Trainer *p, Map *m, char *exitDir);
int moveNPC(Trainer *t, Map *m, int dist[MAP_HEIGHT][MAP_WIDTH]);
int validSpace(Trainer *t, Map *m, int x, int y);

#endif
