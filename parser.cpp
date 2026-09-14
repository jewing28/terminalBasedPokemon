#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "parser.h"

using namespace std;

PokemonDatabase pokemonDatabase;

static string normalizeTableName(string table){
    if(table.size() > 4 && table.substr(table.size() - 4) == ".csv"){
        table = table.substr(0, table.size() - 4);
    }

    return table;
}

static vector<string> splitCsvLine(const string &line){
    vector<string> cells;
    string cell;
    int inQuotes = 0;

    for(size_t i = 0; i < line.size(); i++){
        if(line[i] == '"'){
            if(inQuotes && i + 1 < line.size() && line[i + 1] == '"'){
                cell += '"';
                i++;
            }
            else{
                inQuotes = !inQuotes;
            }
        }
        else if(line[i] == ',' && !inQuotes){
            cells.push_back(cell);
            cell = "";
        }
        else{
            cell += line[i];
        }
    }

    cells.push_back(cell);
    return cells;
}

static vector<string> splitHeaderList(const string &line){
    vector<string> headers;
    string header;

    for(size_t i = 0; i < line.size(); i++){
        if(line[i] == ','){
            headers.push_back(header);
            header = "";
        }
        else{
            header += line[i];
        }
    }

    headers.push_back(header);
    return headers;
}

static int getTableInfo(const string &table, TableInfo &info){
    if(table == "pokemon"){
        info.headers = splitHeaderList("id,identifier,species_id,height,weight,base_experience,order,is_default");
        info.isIntColumn = {1, 0, 1, 1, 1, 1, 1, 1};
    }
    else if(table == "moves"){
        info.headers = splitHeaderList("id,identifier,generation_id,type_id,power,pp,accuracy,priority,target_id,damage_class_id,effect_id,effect_chance,contest_type_id,contest_effect_id,super_contest_effect_id");
        info.isIntColumn = {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    }
    else if(table == "pokemon_moves"){
        info.headers = splitHeaderList("pokemon_id,version_group_id,move_id,pokemon_move_method_id,level,order");
        info.isIntColumn = {1, 1, 1, 1, 1, 1};
    }
    else if(table == "pokemon_species"){
        info.headers = splitHeaderList("id,identifier,generation_id,evolves_from_species_id,evolution_chain_id,color_id,shape_id,habitat_id,gender_rate,capture_rate,base_happiness,is_baby,hatch_counter,has_gender_differences,growth_rate_id,forms_switchable,is_legendary,is_mythical,order,conquest_order");
        info.isIntColumn = {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    }
    else if(table == "experience"){
        info.headers = splitHeaderList("growth_rate_id,level,experience");
        info.isIntColumn = {1, 1, 1};
    }
    else if(table == "type_names"){
        info.headers = splitHeaderList("type_id,local_language_id,name");
        info.isIntColumn = {1, 1, 0};
    }
    else if(table == "pokemon_stats"){
        info.headers = splitHeaderList("pokemon_id,stat_id,base_stat,effort");
        info.isIntColumn = {1, 1, 1, 1};
    }
    else if(table == "stats"){
        info.headers = splitHeaderList("id,damage_class_id,identifier,is_battle_only,game_index");
        info.isIntColumn = {1, 1, 0, 1, 1};
    }
    else if(table == "pokemon_types"){
        info.headers = splitHeaderList("pokemon_id,type_id,slot");
        info.isIntColumn = {1, 1, 1};
    }
    else{
        return 0;
    }

    return 1;
}

static string csvDirectory(const string &base){
    return base + "/pokedex/pokedex/data/csv";
}

static string findCsvPath(const string &table){
    string path = csvDirectory("/share/cs327") + "/" + table + ".csv";
    ifstream file(path.c_str());
    if(file.good()){
        return path;
    }

    const char *home = getenv("HOME");
    if(home){
        path = csvDirectory(string(home) + "/.poke327") + "/" + table + ".csv";
        ifstream homeFile(path.c_str());
        if(homeFile.good()){
            return path;
        }
    }

    return "";
}

static int csvInt(const vector<string> &cells, size_t index){
    if(index >= cells.size() || cells[index].empty()){
        return INT_MAX;
    }

    return atoi(cells[index].c_str());
}

static string csvString(const vector<string> &cells, size_t index){
    if(index >= cells.size()){
        return "";
    }

    return cells[index];
}

static int openCsvTable(const string &table, ifstream &file){
    string path = findCsvPath(table);
    if(path.empty()){
        cerr << "Could not find " << table << ".csv in /share/cs327 or $HOME/.poke327" << endl;
        return 1;
    }

    file.open(path.c_str());
    if(!file){
        cerr << "Could not open " << path << endl;
        return 1;
    }

    return 0;
}

static int loadPokemonTable(vector<PokemonSpeciesData> &pokemon){
    ifstream file;
    if(openCsvTable("pokemon", file)){
        return 1;
    }

    string line;
    getline(file, line);
    while(getline(file, line)){
        if(!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        }

        vector<string> cells = splitCsvLine(line);
        PokemonSpeciesData p;
        p.id = csvInt(cells, 0);
        p.identifier = csvString(cells, 1);
        p.speciesId = csvInt(cells, 2);
        pokemon.push_back(p);
    }

    return 0;
}

static int loadMovesTable(vector<MoveData> &moves){
    ifstream file;
    if(openCsvTable("moves", file)){
        return 1;
    }

    string line;
    getline(file, line);
    while(getline(file, line)){
        if(!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        }

        vector<string> cells = splitCsvLine(line);
        MoveData m;
        m.id = csvInt(cells, 0);
        m.identifier = csvString(cells, 1);
        m.typeId = csvInt(cells, 3);
        m.power = csvInt(cells, 4);
        m.accuracy = csvInt(cells, 6);
        m.priority = csvInt(cells, 7);
        m.damageClassId = csvInt(cells, 9);
        moves.push_back(m);
    }

    return 0;
}

static int loadPokemonMovesTable(vector<PokemonMoveData> &pokemonMoves){
    ifstream file;
    if(openCsvTable("pokemon_moves", file)){
        return 1;
    }

    string line;
    getline(file, line);
    while(getline(file, line)){
        if(!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        }

        vector<string> cells = splitCsvLine(line);
        PokemonMoveData pm;
        pm.pokemonId = csvInt(cells, 0);
        pm.versionGroupId = csvInt(cells, 1);
        pm.moveId = csvInt(cells, 2);
        pm.pokemonMoveMethodId = csvInt(cells, 3);
        pm.level = csvInt(cells, 4);
        pokemonMoves.push_back(pm);
    }

    return 0;
}

static int loadPokemonStatsTable(vector<PokemonStatData> &pokemonStats){
    ifstream file;
    if(openCsvTable("pokemon_stats", file)){
        return 1;
    }

    string line;
    getline(file, line);
    while(getline(file, line)){
        if(!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        }

        vector<string> cells = splitCsvLine(line);
        PokemonStatData ps;
        ps.pokemonId = csvInt(cells, 0);
        ps.statId = csvInt(cells, 1);
        ps.baseStat = csvInt(cells, 2);
        pokemonStats.push_back(ps);
    }

    return 0;
}

static int loadStatsTable(vector<StatData> &stats){
    ifstream file;
    if(openCsvTable("stats", file)){
        return 1;
    }

    string line;
    getline(file, line);
    while(getline(file, line)){
        if(!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        }

        vector<string> cells = splitCsvLine(line);
        StatData s;
        s.id = csvInt(cells, 0);
        s.identifier = csvString(cells, 2);
        stats.push_back(s);
    }

    return 0;
}

static int loadPokemonTypesTable(vector<PokemonTypeData> &pokemonTypes){
    ifstream file;
    if(openCsvTable("pokemon_types", file)){
        return 1;
    }

    string line;
    getline(file, line);
    while(getline(file, line)){
        if(!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        }

        vector<string> cells = splitCsvLine(line);
        PokemonTypeData pt;
        pt.pokemonId = csvInt(cells, 0);
        pt.typeId = csvInt(cells, 1);
        pt.slot = csvInt(cells, 2);
        pokemonTypes.push_back(pt);
    }

    return 0;
}

void PokemonDatabase::clear(){
    pokemon.clear();
    moves.clear();
    pokemonMoves.clear();
    pokemonStats.clear();
    stats.clear();
    pokemonTypes.clear();
}

int PokemonDatabase::load(){
    clear();

    if(loadPokemonTable(pokemon) ||
       loadMovesTable(moves) ||
       loadPokemonMovesTable(pokemonMoves) ||
       loadPokemonStatsTable(pokemonStats) ||
       loadStatsTable(stats) ||
       loadPokemonTypesTable(pokemonTypes)){
        clear();
        return 1;
    }

    return 0;
}

static void printHeaders(const vector<string> &headers){
    for(size_t i = 0; i < headers.size(); i++){
        if(i){
            cout << ",";
        }
        cout << headers[i];
    }
    cout << endl;
}

static void printRow(const vector<string> &cells, const vector<int> &isIntColumn){
    for(size_t i = 0; i < isIntColumn.size(); i++){
        if(i){
            cout << ",";
        }

        string cell = i < cells.size() ? cells[i] : "";
        if(isIntColumn[i]){
            int value = cell.empty() ? INT_MAX : atoi(cell.c_str());
            if(value != INT_MAX){
                cout << value;
            }
        }
        else{
            cout << cell;
        }
    }

    cout << endl;
}

int handleParserCommand(int argc, char *argv[]){
    if(argc != 2){
        return -1;
    }

    string table = normalizeTableName(argv[1]);
    TableInfo info;

    if(!getTableInfo(table, info)){
        return -1;
    }

    string path = findCsvPath(table);
    if(path.empty()){
        cerr << "Could not find " << table << ".csv in /share/cs327 or $HOME/.poke327" << endl;
        return 1;
    }

    ifstream file(path.c_str());
    if(!file){
        cerr << "Could not open " << path << endl;
        return 1;
    }

    string line;
    getline(file, line);

    printHeaders(info.headers);
    while(getline(file, line)){
        if(!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        }
        printRow(splitCsvLine(line), info.isIntColumn);
    }

    return 0;
}
