#ifndef POKEMON_H
#define POKEMON_H

#include <string>

#define POKEMON_MAX_MOVES 2
#define TRAINER_MAX_POKEMON 6

class PokemonMove {
public:
    int id;
    std::string name;
    int typeId;
    int power;
    int accuracy;
    int priority;
    int damageClassId;

    PokemonMove() : id(0), name(""), typeId(0), power(0), accuracy(100), priority(0), damageClassId(0) {}
};

class Pokemon {
public:
    int pokemonId;
    int speciesId;
    std::string name;
    int level;

    int currentHp;
    int hp;
    int attack;
    int defense;
    int speed;
    int specialAttack;
    int specialDefense;
    int baseSpeed;

    int hpIv;
    int attackIv;
    int defenseIv;
    int speedIv;
    int specialAttackIv;
    int specialDefenseIv;

    int gender;
    int shiny;
    int typeIds[2];
    int numTypes;

    PokemonMove moves[POKEMON_MAX_MOVES];
    int numMoves;

    Pokemon()
        : pokemonId(0),
          speciesId(0),
          name(""),
          level(0),
          currentHp(0),
          hp(0),
          attack(0),
          defense(0),
          speed(0),
          specialAttack(0),
          specialDefense(0),
          baseSpeed(0),
          hpIv(0),
          attackIv(0),
          defenseIv(0),
          speedIv(0),
          specialAttackIv(0),
          specialDefenseIv(0),
          gender(0),
          shiny(0),
          typeIds{0, 0},
          numTypes(0),
          numMoves(0)
    {
    }
};

int pokemonLevelForMap(int worldX, int worldY);
Pokemon generatePokemonAtLevel(int level);
Pokemon generatePokemonForMap(int worldX, int worldY);

#endif
