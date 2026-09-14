#ifndef UI_H
#define UI_H

#include "constants.h"
#include "trainer.h"

int normalizeInput(int input);
void promptStarterSelection(Trainer *pcTrainer);
void displayTrainerList(Map *m, int scrollOffset);
int runBattlePlaceholder(Map *m, Trainer *pcTrainer);
int runWildPokemonPlaceholder(Map *m, Trainer *pcTrainer, Pokemon *wildPokemon);
int promptFlyDestination(int *worldX, int *worldY);

#endif
