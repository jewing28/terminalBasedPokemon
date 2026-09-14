#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct TableInfo {
    std::vector<std::string> headers;
    std::vector<int> isIntColumn;
};

struct PokemonSpeciesData {
    int id;
    std::string identifier;
    int speciesId;
};

struct MoveData {
    int id;
    std::string identifier;
    int typeId;
    int power;
    int accuracy;
    int priority;
    int damageClassId;
};

struct PokemonMoveData {
    int pokemonId;
    int versionGroupId;
    int moveId;
    int pokemonMoveMethodId;
    int level;
};

struct PokemonStatData {
    int pokemonId;
    int statId;
    int baseStat;
};

struct StatData {
    int id;
    std::string identifier;
};

struct PokemonTypeData {
    int pokemonId;
    int typeId;
    int slot;
};

class PokemonDatabase {
public:
    std::vector<PokemonSpeciesData> pokemon;
    std::vector<MoveData> moves;
    std::vector<PokemonMoveData> pokemonMoves;
    std::vector<PokemonStatData> pokemonStats;
    std::vector<StatData> stats;
    std::vector<PokemonTypeData> pokemonTypes;

    int load();
    void clear();
};

extern PokemonDatabase pokemonDatabase;

int handleParserCommand(int argc, char *argv[]);

#endif
