#include <stdlib.h>
#include <stdio.h>
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

static void clearBattleRows(void);
static void printTruncated(int row, const char *message);
static void waitForEscape(void);

static Trainer *findAdjacentUndefeatedTrainer(Map *m, Trainer *pcTrainer)
{
    for(int i = 0; i < numNPCs; i++){
        Trainer *trainer = &m->trainers[i];
        if(trainer->defeated){
            continue;
        }
        if(abs(trainer->x - pcTrainer->x) <= 1 && abs(trainer->y - pcTrainer->y) <= 1){
            return trainer;
        }
    }

    return nullptr;
}

static int firstUsablePokemon(Trainer *trainer)
{
    for(int i = 0; i < trainer->partySize; i++){
        if(trainer->party[i].currentHp > 0){
            return i;
        }
    }

    return -1;
}

static int hasUsablePokemon(Trainer *trainer)
{
    return firstUsablePokemon(trainer) >= 0;
}

static void healParty(Trainer *trainer)
{
    for(int i = 0; i < trainer->partySize; i++){
        trainer->party[i].currentHp = trainer->party[i].hp;
    }
}

void restockTrainerBag(Trainer *trainer)
{
    trainer->potions = 5;
    trainer->revives = 2;
    trainer->pokeballs = 5;
}

static int choosePartyPokemon(Trainer *trainer, int allowFainted, int excludedIndex)
{
    int input = ERR;
    char line[160];

    while(1){
        clearBattleRows();
        printTruncated(MAP_HEIGHT, "Choose Pokemon  Esc: cancel");
        for(int i = 0; i < trainer->partySize && i < TRAINER_MAX_POKEMON; i++){
            snprintf(line, sizeof(line), "%d.%s %d/%d%s",
                     i + 1,
                     trainer->party[i].name.c_str(),
                     trainer->party[i].currentHp,
                     trainer->party[i].hp,
                     i == excludedIndex ? " ACT" : trainer->party[i].currentHp <= 0 ? " FNT" : "");
            mvprintw(MAP_HEIGHT + 1 + (i / 3), (i % 3) * 26, "%.*s", 25, line);
        }
        refresh();

        input = getch();
        if(input == 27){
            return -1;
        }
        if(input >= '1' && input < '1' + trainer->partySize){
            int index = input - '1';
            if(index != excludedIndex && (allowFainted || trainer->party[index].currentHp > 0)){
                return index;
            }
        }
    }
}

static int usePotion(Trainer *trainer, char *message, size_t messageSize)
{
    if(trainer->potions <= 0){
        snprintf(message, messageSize, "No potions left.");
        return 0;
    }

    int index = choosePartyPokemon(trainer, 0, -1);
    if(index < 0){
        snprintf(message, messageSize, "Canceled.");
        return 0;
    }

    Pokemon *pokemon = &trainer->party[index];
    if(pokemon->currentHp >= pokemon->hp){
        snprintf(message, messageSize, "%s is already full HP.", pokemon->name.c_str());
        return 0;
    }

    pokemon->currentHp += 20;
    if(pokemon->currentHp > pokemon->hp){
        pokemon->currentHp = pokemon->hp;
    }
    trainer->potions--;
    snprintf(message, messageSize, "Used potion on %s.", pokemon->name.c_str());
    return 1;
}

static int useRevive(Trainer *trainer, char *message, size_t messageSize)
{
    if(trainer->revives <= 0){
        snprintf(message, messageSize, "No revives left.");
        return 0;
    }

    int index = choosePartyPokemon(trainer, 1, -1);
    if(index < 0){
        snprintf(message, messageSize, "Canceled.");
        return 0;
    }

    Pokemon *pokemon = &trainer->party[index];
    if(pokemon->currentHp > 0){
        snprintf(message, messageSize, "%s has not fainted.", pokemon->name.c_str());
        return 0;
    }

    pokemon->currentHp = pokemon->hp / 2;
    if(pokemon->currentHp < 1){
        pokemon->currentHp = 1;
    }
    trainer->revives--;
    snprintf(message, messageSize, "Used revive on %s.", pokemon->name.c_str());
    return 1;
}

static int openBagAction(Trainer *trainer, Pokemon *wildPokemon, int wildBattle, char *message, size_t messageSize)
{
    int input = ERR;
    char line[160];

    while(1){
        clearBattleRows();
        snprintf(line, sizeof(line), "Bag: Potions %d  Revives %d  Pokeballs %d", trainer->potions, trainer->revives, trainer->pokeballs);
        printTruncated(MAP_HEIGHT, line);
        printTruncated(MAP_HEIGHT + 1, "1. Potion  2. Revive");
        printTruncated(MAP_HEIGHT + 2, wildBattle ? "3. Pokeball  Esc: cancel" : "Esc: cancel");
        refresh();

        input = getch();
        if(input == 27){
            return 0;
        }
        if(input == '1'){
            return usePotion(trainer, message, messageSize);
        }
        if(input == '2'){
            return useRevive(trainer, message, messageSize);
        }
        if(input == '3' && wildBattle){
            if(trainer->pokeballs <= 0){
                snprintf(message, messageSize, "No pokeballs left.");
                return 0;
            }
            trainer->pokeballs--;
            if(trainer->partySize < TRAINER_MAX_POKEMON){
                trainer->party[trainer->partySize] = *wildPokemon;
                trainer->party[trainer->partySize].currentHp = trainer->party[trainer->partySize].hp;
                trainer->partySize++;
                wildPokemon->currentHp = 0;
                snprintf(message, messageSize, "%s was caught.", wildPokemon->name.c_str());
            }
            else{
                int replaceIndex = -1;
                while(replaceIndex < 0){
                    replaceIndex = choosePartyPokemon(trainer, 1, -1);
                }

                snprintf(message, messageSize, "%s replaced %s.",
                         wildPokemon->name.c_str(),
                         trainer->party[replaceIndex].name.c_str());
                trainer->party[replaceIndex] = *wildPokemon;
                trainer->party[replaceIndex].currentHp = trainer->party[replaceIndex].hp;
                wildPokemon->currentHp = 0;
            }
            return 1;
        }
    }
}

static int switchPokemon(Trainer *trainer, int currentIndex, char *message, size_t messageSize)
{
    int newIndex = choosePartyPokemon(trainer, 0, currentIndex);

    if(newIndex < 0){
        return currentIndex;
    }

    snprintf(message, messageSize, "Come back %s. Go %s.",
             trainer->party[currentIndex].name.c_str(),
             trainer->party[newIndex].name.c_str());
    return newIndex;
}

static int forcePokemonSelection(Trainer *trainer)
{
    int selected = -1;

    while(selected < 0){
        selected = choosePartyPokemon(trainer, 0, -1);
    }

    return selected;
}

void openBagMenu(Trainer *trainer)
{
    char message[160] = "";

    nodelay(stdscr, FALSE);
    openBagAction(trainer, nullptr, 0, message, sizeof(message));
    if(!message[0]){
        nodelay(stdscr, TRUE);
        return;
    }
    clearBattleRows();
    printTruncated(MAP_HEIGHT, message);
    waitForEscape();
    nodelay(stdscr, TRUE);
}

static int moveGetsStab(Pokemon *attacker, PokemonMove *move)
{
    for(int i = 0; i < attacker->numTypes; i++){
        if(attacker->typeIds[i] == move->typeId){
            return 1;
        }
    }

    return 0;
}

static int criticalHit(Pokemon *attacker)
{
    return (rand() % 256) < (attacker->baseSpeed / 2);
}

static double singleTypeEffectiveness(int attackType, int defendType)
{
    switch(attackType){
        case 1:  return (defendType == 6 || defendType == 9) ? 0.5 : defendType == 8 ? 0.0 : 1.0;
        case 2:
            if(defendType == 1 || defendType == 6 || defendType == 9 || defendType == 15 || defendType == 17) return 2.0;
            if(defendType == 3 || defendType == 4 || defendType == 7 || defendType == 14 || defendType == 18) return 0.5;
            return defendType == 8 ? 0.0 : 1.0;
        case 3:
            if(defendType == 2 || defendType == 7 || defendType == 12) return 2.0;
            return (defendType == 6 || defendType == 9 || defendType == 13) ? 0.5 : 1.0;
        case 4:
            if(defendType == 12 || defendType == 18) return 2.0;
            if(defendType == 4 || defendType == 5 || defendType == 6 || defendType == 8) return 0.5;
            return defendType == 9 ? 0.0 : 1.0;
        case 5:
            if(defendType == 4 || defendType == 6 || defendType == 9 || defendType == 10 || defendType == 13) return 2.0;
            if(defendType == 7 || defendType == 12) return 0.5;
            return defendType == 3 ? 0.0 : 1.0;
        case 6:
            if(defendType == 3 || defendType == 7 || defendType == 10 || defendType == 15) return 2.0;
            return (defendType == 2 || defendType == 5 || defendType == 9) ? 0.5 : 1.0;
        case 7:
            if(defendType == 12 || defendType == 14 || defendType == 17) return 2.0;
            if(defendType == 2 || defendType == 3 || defendType == 4 || defendType == 8 || defendType == 9 || defendType == 10 || defendType == 18) return 0.5;
            return 1.0;
        case 8:
            if(defendType == 8 || defendType == 14) return 2.0;
            return defendType == 17 ? 0.5 : defendType == 1 ? 0.0 : 1.0;
        case 9:
            if(defendType == 6 || defendType == 15 || defendType == 18) return 2.0;
            return (defendType == 9 || defendType == 10 || defendType == 11 || defendType == 13) ? 0.5 : 1.0;
        case 10:
            if(defendType == 7 || defendType == 9 || defendType == 12 || defendType == 15) return 2.0;
            if(defendType == 6 || defendType == 10 || defendType == 11 || defendType == 16) return 0.5;
            return 1.0;
        case 11:
            if(defendType == 5 || defendType == 6 || defendType == 10) return 2.0;
            return (defendType == 11 || defendType == 12 || defendType == 16) ? 0.5 : 1.0;
        case 12:
            if(defendType == 5 || defendType == 6 || defendType == 11) return 2.0;
            if(defendType == 3 || defendType == 4 || defendType == 7 || defendType == 9 || defendType == 10 || defendType == 12 || defendType == 16) return 0.5;
            return 1.0;
        case 13:
            if(defendType == 3 || defendType == 11) return 2.0;
            if(defendType == 12 || defendType == 13 || defendType == 16) return 0.5;
            return defendType == 5 ? 0.0 : 1.0;
        case 14:
            if(defendType == 2 || defendType == 4) return 2.0;
            return (defendType == 9 || defendType == 14) ? 0.5 : defendType == 17 ? 0.0 : 1.0;
        case 15:
            if(defendType == 3 || defendType == 5 || defendType == 12 || defendType == 16) return 2.0;
            return (defendType == 9 || defendType == 10 || defendType == 11 || defendType == 15) ? 0.5 : 1.0;
        case 16:
            return defendType == 16 ? 2.0 : defendType == 9 ? 0.5 : defendType == 18 ? 0.0 : 1.0;
        case 17:
            if(defendType == 8 || defendType == 14) return 2.0;
            return (defendType == 2 || defendType == 17 || defendType == 18) ? 0.5 : 1.0;
        case 18:
            if(defendType == 2 || defendType == 16 || defendType == 17) return 2.0;
            return (defendType == 4 || defendType == 9 || defendType == 10) ? 0.5 : 1.0;
        default:
            return 1.0;
    }
}

static double typeEffectiveness(PokemonMove *move, Pokemon *defender)
{
    double multiplier = 1.0;

    for(int i = 0; i < defender->numTypes; i++){
        multiplier *= singleTypeEffectiveness(move->typeId, defender->typeIds[i]);
    }

    return multiplier;
}

static int calculateDamage(Pokemon *attacker, Pokemon *defender, PokemonMove *move)
{
    if(move->damageClassId == 1 || move->power <= 0){
        return 0;
    }

    int power = move->power > 0 ? move->power : 1;
    int attack = move->damageClassId == 3 ? attacker->specialAttack : attacker->attack;
    int defense = move->damageClassId == 3 ? defender->specialDefense : defender->defense;
    attack = attack > 0 ? attack : 1;
    defense = defense > 0 ? defense : 1;
    double damage = (((((2.0 * attacker->level) / 5.0) + 2.0) * power * attack / defense) / 50.0) + 2.0;

    if(criticalHit(attacker)){
        damage *= 1.5;
    }
    damage *= (85 + rand() % 16) / 100.0;
    if(moveGetsStab(attacker, move)){
        damage *= 1.5;
    }
    double typeMultiplier = typeEffectiveness(move, defender);
    damage *= typeMultiplier;

    int finalDamage = static_cast<int>(damage);

    if(finalDamage < 1 && typeMultiplier > 0.0){
        finalDamage = 1;
    }

    return finalDamage;
}

static void clearBattleRows(void)
{
    for(int row = MAP_HEIGHT; row < MAP_HEIGHT + 3; row++){
        move(row, 0);
        clrtoeol();
    }
}

static void printTruncated(int row, const char *message)
{
    mvprintw(row, 0, "%.*s", MAP_WIDTH - 1, message);
}

static void printBattleSummary(Pokemon *pcPokemon, Pokemon *opponentPokemon, const char *opponentLabel)
{
    char line[160];

    snprintf(line, sizeof(line), "%s %s Lv:%d HP:%d/%d | Your %s Lv:%d HP:%d/%d",
             opponentLabel,
             opponentPokemon->name.c_str(),
             opponentPokemon->level,
             opponentPokemon->currentHp,
             opponentPokemon->hp,
             pcPokemon->name.c_str(),
             pcPokemon->level,
             pcPokemon->currentHp,
             pcPokemon->hp);
    printTruncated(MAP_HEIGHT, line);
}

static int choosePcMove(Pokemon *pcPokemon, Pokemon *opponentPokemon, const char *opponentLabel)
{
    int input = ERR;
    char line[160];

    while(1){
        clearBattleRows();
        printBattleSummary(pcPokemon, opponentPokemon, opponentLabel);

        if(pcPokemon->numMoves > 0){
            snprintf(line, sizeof(line), "1. %s", pcPokemon->moves[0].name.c_str());
            printTruncated(MAP_HEIGHT + 1, line);
        }
        if(pcPokemon->numMoves > 1){
            snprintf(line, sizeof(line), "2. %s", pcPokemon->moves[1].name.c_str());
            printTruncated(MAP_HEIGHT + 2, line);
        }
        else{
            printTruncated(MAP_HEIGHT + 2, "Esc: cancel");
        }
        refresh();

        input = getch();
        if(input == 27){
            return -1;
        }
        if(input >= '1' && input < '1' + pcPokemon->numMoves){
            return input - '1';
        }
    }
}

static void drawBattle(Pokemon *pcPokemon, Pokemon *opponentPokemon, const char *opponentLabel)
{
    clearBattleRows();
    printBattleSummary(pcPokemon, opponentPokemon, opponentLabel);
    printTruncated(MAP_HEIGHT + 1, "1. Fight  2. Bag");
    printTruncated(MAP_HEIGHT + 2, "3. Pokemon  4. Run");
    refresh();
}

static int moveHits(PokemonMove *move)
{
    int accuracy = move->accuracy > 0 ? move->accuracy : 100;

    return (rand() % 100) <= accuracy;
}

static void applyAttack(Pokemon *attacker, Pokemon *defender, PokemonMove *move, char *message, size_t messageSize)
{
    if(!moveHits(move)){
        snprintf(message, messageSize, "%s used %s, but it missed.",
                 attacker->name.c_str(),
                 move->name.c_str());
        return;
    }

    if(move->damageClassId == 1 || move->power <= 0){
        snprintf(message, messageSize, "%s used %s, but it did no damage.",
                 attacker->name.c_str(),
                 move->name.c_str());
        return;
    }

    if(typeEffectiveness(move, defender) == 0.0){
        snprintf(message, messageSize, "%s used %s, but it had no effect.",
                 attacker->name.c_str(),
                 move->name.c_str());
        return;
    }

    int damage = calculateDamage(attacker, defender, move);
    if(damage > defender->currentHp){
        damage = defender->currentHp;
    }

    defender->currentHp -= damage;
    if(defender->currentHp < 0){
        defender->currentHp = 0;
    }

    snprintf(message, messageSize, "%s used %s for %d damage.",
             attacker->name.c_str(),
             move->name.c_str(),
             damage);
}

static void waitForEscape(void)
{
    int input = ERR;

    printTruncated(MAP_HEIGHT + 2, "Esc: continue");
    refresh();
    while(input != 27){
        input = getch();
    }
}

static void showTurnMessages(Pokemon *pcPokemon, Pokemon *opponentPokemon, const char *opponentLabel, const char *firstMessage, const char *secondMessage)
{
    clearBattleRows();
    printBattleSummary(pcPokemon, opponentPokemon, opponentLabel);
    printTruncated(MAP_HEIGHT + 1, firstMessage);
    if(secondMessage && secondMessage[0]){
        printTruncated(MAP_HEIGHT + 2, secondMessage);
    }
    refresh();
    napms(2000);
}

static void showBattleEnd(Pokemon *pcPokemon, Pokemon *opponentPokemon, const char *opponentLabel)
{
    clearBattleRows();
    printBattleSummary(pcPokemon, opponentPokemon, opponentLabel);
    printTruncated(MAP_HEIGHT + 1, "Battle ended.");
    waitForEscape();
}

static void handleWhiteOut(Map *m, Trainer *pcTrainer)
{
    (void)m;
    healParty(pcTrainer);
    clearBattleRows();
    printTruncated(MAP_HEIGHT, "You blacked out. Your Pokemon were healed.");
    waitForEscape();
}

static void showSendOutMessage(Pokemon *nextPokemon)
{
    char message[160];

    clearBattleRows();
    snprintf(message, sizeof(message), "%s is about to send out %s.",
             "Opponent",
             nextPokemon->name.c_str());
    printTruncated(MAP_HEIGHT, message);
    refresh();
    napms(2000);
}

static void showPlayerSendOutMessage(Pokemon *nextPokemon)
{
    char message[160];

    clearBattleRows();
    snprintf(message, sizeof(message), "Go %s.", nextPokemon->name.c_str());
    printTruncated(MAP_HEIGHT, message);
    refresh();
    napms(2000);
}

static void runOneOnOneBattle(Trainer *pcTrainer, Pokemon *opponentPokemon, const char *opponentLabel, int wildBattle)
{
    int pcIndex = firstUsablePokemon(pcTrainer);
    char firstMessage[160] = "";
    char secondMessage[160] = "";

    if(pcIndex < 0 || !opponentPokemon){
        return;
    }

    nodelay(stdscr, FALSE);

    while(hasUsablePokemon(pcTrainer) && opponentPokemon->currentHp > 0){
        if(pcTrainer->party[pcIndex].currentHp <= 0){
            pcIndex = forcePokemonSelection(pcTrainer);
            showPlayerSendOutMessage(&pcTrainer->party[pcIndex]);
        }

        Pokemon *pcPokemon = &pcTrainer->party[pcIndex];
        drawBattle(pcPokemon, opponentPokemon, opponentLabel);

        int input = getch();
        if(input != '1' && input != '2' && input != '3' && input != '4'){
            continue;
        }

        int opponentMove = opponentPokemon->numMoves > 0 ? rand() % opponentPokemon->numMoves : 0;
        PokemonMove *opponentSelectedMove = &opponentPokemon->moves[opponentMove];
        PokemonMove *pcSelectedMove = nullptr;
        int pcFirst = 1;

        firstMessage[0] = '\0';
        secondMessage[0] = '\0';

        if(input == '1'){
            int pcMove = choosePcMove(pcPokemon, opponentPokemon, opponentLabel);
            if(pcMove < 0){
                continue;
            }

            pcSelectedMove = &pcPokemon->moves[pcMove];

            if(opponentSelectedMove->priority > pcSelectedMove->priority){
                pcFirst = 0;
            }
            else if(opponentSelectedMove->priority == pcSelectedMove->priority){
                if(opponentPokemon->speed > pcPokemon->speed){
                    pcFirst = 0;
                }
                else if(opponentPokemon->speed == pcPokemon->speed){
                    pcFirst = rand() % 2;
                }
            }
        }
        else if(input == '2'){
            int usedItem = openBagAction(pcTrainer, wildBattle ? opponentPokemon : nullptr, wildBattle, firstMessage, sizeof(firstMessage));
            if(!usedItem){
                if(firstMessage[0]){
                    showTurnMessages(pcPokemon, opponentPokemon, opponentLabel, firstMessage, "");
                }
                continue;
            }
            pcFirst = 1;
            if(wildBattle && opponentPokemon->currentHp <= 0){
                showTurnMessages(pcPokemon, opponentPokemon, opponentLabel, firstMessage, "");
                break;
            }
        }
        else{
            if(input == '4'){
                if(wildBattle){
                    firstMessage[0] = '\0';
                    snprintf(firstMessage, sizeof(firstMessage), "You got away safely.");
                    showTurnMessages(pcPokemon, opponentPokemon, opponentLabel, firstMessage, "");
                    break;
                }

                snprintf(firstMessage, sizeof(firstMessage), "You cannot run from a trainer battle.");
                showTurnMessages(pcPokemon, opponentPokemon, opponentLabel, firstMessage, "");
                continue;
            }

            int newPcIndex = switchPokemon(pcTrainer, pcIndex, firstMessage, sizeof(firstMessage));
            if(newPcIndex == pcIndex){
                continue;
            }
            pcIndex = newPcIndex;
            pcPokemon = &pcTrainer->party[pcIndex];
            pcFirst = 1;
        }

        if(pcFirst){
            if(pcSelectedMove){
                applyAttack(pcPokemon, opponentPokemon, pcSelectedMove, firstMessage, sizeof(firstMessage));
            }
            if(opponentPokemon->currentHp > 0){
                applyAttack(opponentPokemon, pcPokemon, opponentSelectedMove, secondMessage, sizeof(secondMessage));
            }
            else if(pcSelectedMove){
                snprintf(secondMessage, sizeof(secondMessage), "%s fainted.", opponentPokemon->name.c_str());
            }
        }
        else{
            applyAttack(opponentPokemon, pcPokemon, opponentSelectedMove, firstMessage, sizeof(firstMessage));
            if(pcPokemon->currentHp > 0 && pcSelectedMove){
                applyAttack(pcPokemon, opponentPokemon, pcSelectedMove, secondMessage, sizeof(secondMessage));
            }
            else{
                snprintf(secondMessage, sizeof(secondMessage), "%s fainted.", pcPokemon->name.c_str());
            }
        }

        showTurnMessages(pcPokemon, opponentPokemon, opponentLabel, firstMessage, secondMessage);
    }

    nodelay(stdscr, TRUE);
}

int runTrainerBattle(Map *m, Trainer *pcTrainer)
{
    Trainer *opponent = findAdjacentUndefeatedTrainer(m, pcTrainer);

    if(!opponent || opponent->partySize <= 0){
        return 0;
    }

    while(hasUsablePokemon(pcTrainer) && hasUsablePokemon(opponent)){
        int opponentIndex = firstUsablePokemon(opponent);
        runOneOnOneBattle(pcTrainer, &opponent->party[opponentIndex], "Opponent", 0);

        if(hasUsablePokemon(pcTrainer) && hasUsablePokemon(opponent)){
            int nextOpponentIndex = firstUsablePokemon(opponent);
            showSendOutMessage(&opponent->party[nextOpponentIndex]);
        }
    }

    if(!hasUsablePokemon(opponent)){
        opponent->defeated = 1;
        int pcIndex = firstUsablePokemon(pcTrainer);
        showBattleEnd(&pcTrainer->party[pcIndex >= 0 ? pcIndex : 0], &opponent->party[0], "Opponent");
        return 1;
    }

    if(!hasUsablePokemon(pcTrainer)){
        handleWhiteOut(m, pcTrainer);
        return -1;
    }

    if(pcTrainer->partySize > 0 && hasUsablePokemon(opponent)){
        int opponentIndex = firstUsablePokemon(opponent);
        showBattleEnd(&pcTrainer->party[0], &opponent->party[opponentIndex], "Opponent");
    }
    return 0;
}

int runWildBattle(Map *m, Trainer *pcTrainer, Pokemon *wildPokemon)
{
    runOneOnOneBattle(pcTrainer, wildPokemon, "Wild", 1);
    if(!hasUsablePokemon(pcTrainer)){
        handleWhiteOut(m, pcTrainer);
        return -1;
    }
    if(pcTrainer->partySize > 0){
        showBattleEnd(&pcTrainer->party[0], wildPokemon, "Wild");
    }
    return 0;
}
