#ifndef BATTLE_H
#define BATTLE_H

#include "constants.h"
#include "trainer.h"

int runTrainerBattle(Map *m, Trainer *pcTrainer);
int runWildBattle(Map *m, Trainer *pcTrainer, Pokemon *wildPokemon);
void restockTrainerBag(Trainer *trainer);
void openBagMenu(Trainer *trainer);

#endif
