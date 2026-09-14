#include <climits>
#include <cstdlib>
#include <vector>
#include "pokemon.h"
#include "parser.h"

using namespace std;

static int clampLevel(int level){
    if(level < 1){
        return 1;
    }
    if(level > 100){
        return 100;
    }
    return level;
}

static int calculatedStat(int base, int iv, int level, int isHp){
    int stat = (((base + iv) * 2) * level) / 100;
    if(isHp){
        return stat + level + 10;
    }

    return stat + 5;
}

static const MoveData *findMove(int moveId){
    for(size_t i = 0; i < pokemonDatabase.moves.size(); i++){
        if(pokemonDatabase.moves[i].id == moveId){
            return &pokemonDatabase.moves[i];
        }
    }

    return nullptr;
}

static void assignMoveMetadata(PokemonMove *pokemonMove, const MoveData *moveData){
    pokemonMove->name = moveData ? moveData->identifier : "";
    pokemonMove->typeId = moveData && moveData->typeId != INT_MAX ? moveData->typeId : 0;
    pokemonMove->power = moveData && moveData->power != INT_MAX ? moveData->power : 0;
    pokemonMove->accuracy = moveData && moveData->accuracy != INT_MAX ? moveData->accuracy : 100;
    pokemonMove->priority = moveData && moveData->priority != INT_MAX ? moveData->priority : 0;
    pokemonMove->damageClassId = moveData && moveData->damageClassId != INT_MAX ? moveData->damageClassId : 0;
}

static int findBaseStat(int pokemonId, int statId){
    for(size_t i = 0; i < pokemonDatabase.pokemonStats.size(); i++){
        const PokemonStatData &stat = pokemonDatabase.pokemonStats[i];
        if(stat.pokemonId == pokemonId && stat.statId == statId){
            return stat.baseStat;
        }
    }

    return 0;
}

static void assignTypes(Pokemon *pokemon){
    pokemon->numTypes = 0;

    for(size_t i = 0; i < pokemonDatabase.pokemonTypes.size(); i++){
        const PokemonTypeData &type = pokemonDatabase.pokemonTypes[i];
        if(type.pokemonId == pokemon->pokemonId && pokemon->numTypes < 2){
            pokemon->typeIds[pokemon->numTypes] = type.typeId;
            pokemon->numTypes++;
        }
    }
}

static int containsMoveId(const vector<int> &moveIds, int moveId){
    for(size_t i = 0; i < moveIds.size(); i++){
        if(moveIds[i] == moveId){
            return 1;
        }
    }

    return 0;
}

static vector<int> findLearnedMoveIds(int pokemonId, int level){
    vector<int> learnedMoveIds;

    for(size_t i = 0; i < pokemonDatabase.pokemonMoves.size(); i++){
        const PokemonMoveData &move = pokemonDatabase.pokemonMoves[i];
        if(move.pokemonId == pokemonId &&
           move.pokemonMoveMethodId == 1 &&
           move.level <= level &&
           !containsMoveId(learnedMoveIds, move.moveId)){
            learnedMoveIds.push_back(move.moveId);
        }
    }

    return learnedMoveIds;
}

static int raiseLevelUntilMoveExists(int pokemonId, int level){
    for(int currentLevel = level; currentLevel <= 100; currentLevel++){
        if(!findLearnedMoveIds(pokemonId, currentLevel).empty()){
            return currentLevel;
        }
    }

    return level;
}

static void assignMoves(Pokemon *pokemon){
    vector<int> learnedMoveIds = findLearnedMoveIds(pokemon->speciesId, pokemon->level);

    if(learnedMoveIds.empty()){
        pokemon->moves[0].id = 165;
        pokemon->moves[0].name = "struggle";
        pokemon->moves[0].typeId = 1;
        pokemon->moves[0].power = 50;
        pokemon->moves[0].accuracy = 100;
        pokemon->moves[0].priority = 0;
        pokemon->moves[0].damageClassId = 2;
        pokemon->numMoves = 1;
        return;
    }

    int firstIndex = rand() % static_cast<int>(learnedMoveIds.size());
    const MoveData *firstMove = findMove(learnedMoveIds[firstIndex]);
    pokemon->moves[0].id = learnedMoveIds[firstIndex];
    assignMoveMetadata(&pokemon->moves[0], firstMove);
    pokemon->numMoves = 1;

    if(learnedMoveIds.size() == 1){
        return;
    }

    int secondIndex = rand() % static_cast<int>(learnedMoveIds.size());
    while(secondIndex == firstIndex){
        secondIndex = rand() % static_cast<int>(learnedMoveIds.size());
    }

    const MoveData *secondMove = findMove(learnedMoveIds[secondIndex]);
    pokemon->moves[1].id = learnedMoveIds[secondIndex];
    assignMoveMetadata(&pokemon->moves[1], secondMove);
    pokemon->numMoves = 2;
}

static void assignStats(Pokemon *pokemon){
    pokemon->hpIv = rand() % 16;
    pokemon->attackIv = rand() % 16;
    pokemon->defenseIv = rand() % 16;
    pokemon->speedIv = rand() % 16;
    pokemon->specialAttackIv = rand() % 16;
    pokemon->specialDefenseIv = rand() % 16;

    pokemon->hp = calculatedStat(findBaseStat(pokemon->pokemonId, 1), pokemon->hpIv, pokemon->level, 1);
    pokemon->attack = calculatedStat(findBaseStat(pokemon->pokemonId, 2), pokemon->attackIv, pokemon->level, 0);
    pokemon->defense = calculatedStat(findBaseStat(pokemon->pokemonId, 3), pokemon->defenseIv, pokemon->level, 0);
    pokemon->specialAttack = calculatedStat(findBaseStat(pokemon->pokemonId, 4), pokemon->specialAttackIv, pokemon->level, 0);
    pokemon->specialDefense = calculatedStat(findBaseStat(pokemon->pokemonId, 5), pokemon->specialDefenseIv, pokemon->level, 0);
    pokemon->baseSpeed = findBaseStat(pokemon->pokemonId, 6);
    pokemon->speed = calculatedStat(pokemon->baseSpeed, pokemon->speedIv, pokemon->level, 0);
    pokemon->currentHp = pokemon->hp;
}

int pokemonLevelForMap(int worldX, int worldY){
    int distance = abs(worldX - 200) + abs(worldY - 200);
    int minLevel;
    int maxLevel;

    if(distance <= 200){
        minLevel = 1;
        maxLevel = distance / 2;
        if(maxLevel < 1){
            maxLevel = 1;
        }
    }
    else{
        minLevel = (distance - 200) / 2;
        maxLevel = 100;
        if(minLevel < 1){
            minLevel = 1;
        }
    }

    return minLevel + rand() % (maxLevel - minLevel + 1);
}

Pokemon generatePokemonAtLevel(int level){
    Pokemon pokemon;

    if(pokemonDatabase.pokemon.empty()){
        return pokemon;
    }

    const PokemonSpeciesData &species = pokemonDatabase.pokemon[rand() % static_cast<int>(pokemonDatabase.pokemon.size())];
    pokemon.pokemonId = species.id;
    pokemon.speciesId = species.speciesId;
    pokemon.name = species.identifier;
    pokemon.level = clampLevel(level);

    pokemon.level = raiseLevelUntilMoveExists(pokemon.speciesId, pokemon.level);
    assignStats(&pokemon);
    assignTypes(&pokemon);
    assignMoves(&pokemon);

    pokemon.gender = rand() % 2;
    pokemon.shiny = rand() % 8192 == 0;

    return pokemon;
}

Pokemon generatePokemonForMap(int worldX, int worldY){
    return generatePokemonAtLevel(pokemonLevelForMap(worldX, worldY));
}
