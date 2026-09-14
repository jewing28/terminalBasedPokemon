#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <new>
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
#include "generation.h"
#include "trainer.h"

static void assignTrainerParty(Trainer *trainer, int worldX, int worldY){
    trainer->partySize = 0;

    do{
        trainer->party[trainer->partySize] = generatePokemonForMap(worldX, worldY);
        trainer->partySize++;
    } while(trainer->partySize < TRAINER_MAX_POKEMON && rand() % 100 < 60);
}

static int displayInitialized = 0;
static chtype previousDisplay[MAP_HEIGHT][MAP_WIDTH];

static chtype tileAppearance(Map *m, Trainer *pc, int row, int col){
    if(pc->y == row && pc->x == col){
        return has_colors() ? ('@' | COLOR_PAIR(COLOR_PAIR_PC) | A_BOLD) : '@';
    }

    char npc = getNPC(col, row, m);
    if (npc == 'h' || npc == 'r') {
        return has_colors() ? (npc | COLOR_PAIR(COLOR_PAIR_AGGRESSIVE) | A_BOLD) : static_cast<chtype>(npc);
    }
    if (npc) {
        return has_colors() ? (npc | COLOR_PAIR(COLOR_PAIR_OTHER_NPC) | A_BOLD) : static_cast<chtype>(npc);
    }
    if (m->map[row][col] == '~' && has_colors()) {
        return m->map[row][col] | COLOR_PAIR(COLOR_PAIR_WATER) | A_BOLD;
    }
    if (m->map[row][col] == '^' && has_colors()) {
        return m->map[row][col] | COLOR_PAIR(COLOR_PAIR_TREES);
    }
    return m->map[row][col];
}

void invalidateMapDisplay(void){
    displayInitialized = 0;
}

/*
Make a randomly generated map with the following parameters
- Width: 80
- Height: 21 (leaving 3 rows for other mechanics)
- Characters: %(bolders/border), ^(trees), #(roads), :(tall grass), .(clearing), ~(water)
- 2 Paths one North-South and one East-West
- Borders should be made of % characters
- At least Two Tall Grass Regions, At least Two Clearing Regions, At least One Water Region, At least One Bolder Region, and At least One Tree Region
*/

Map* generateMap(int x, int y, Map *north, Map *south, Map *east, Map *west){
    Map *m = new (std::nothrow) Map;
    if (!m) {
        return nullptr;
    }
    
    m->gates[0] = north ? north->gates[1] : 0;
    m->gates[1] = south ? south->gates[0] : 0;
    m->gates[2] = east ? east->gates[3] : 0;
    m->gates[3] = west ? west->gates[2] : 0;
    m->mapClock = 0;

    initializeBorders(m->map);

    makePathes(m->map, m->gates);
    
    // 1000 / D is the odds of each building spawning
    if(x == y && x == 200){
        placeBuilding(m->map, 'M');
    }
    else if(1000.0 / (abs(x - 200) + abs(y - 200)) > (double)(rand() % 10000) / 100){
        placeBuilding(m->map, 'M');
    }
        
    if(x == y && x == 200){
        placeBuilding(m->map, 'C');
    }
    else if(1000.0 / (abs(x - 200) + abs(y - 200)) > (double)(rand() % 10000) / 100){
        placeBuilding(m->map, 'C');
    }

    placeStartingPoints(m->map);
    
    int i = 0;
    while (!isFull(m->map) && i < 50) {
        growRegions(m->map);
        i++;
    }

    fillRest(m->map);
    if(y == 0){
        m->map[0][m->gates[0]] = '%';
    }
    if(y == 400){
        m->map[20][m->gates[1]] = '%';
    }
    if(x == 400){
        m->map[m->gates[2]][79] = '%';
    }
    if(x == 0){
        m->map[m->gates[3]][0] = '%';
    }

    m->trainers = new (std::nothrow) Trainer[numNPCs];
    if (!m->trainers) {
        delete m;
        return nullptr;
    }
    m->trainers[0].typeT = TRAINER_HIKER;
    m->trainers[0].typeC = CLASS_HIKER;
    m->trainers[0].nextTurn = 0;
    m->trainers[0].defeated = 0;
    m->trainers[0].x = 0;
    m->trainers[0].y = 0;
    m->trainers[1].curDir = 0;
    m->trainers[1].typeT = TRAINER_RIVAL;
    m->trainers[1].typeC = CLASS_RIVAL;
    m->trainers[1].nextTurn = 0;
    m->trainers[1].defeated = 0;
    m->trainers[1].x = 0;
    m->trainers[1].y = 0;
    m->trainers[0].curDir = 0;
    for(int i = 2; i < numNPCs; i++){
        m->trainers[i].typeC = static_cast<classType>(rand() % 6 + 1);
        m->trainers[i].typeT = (m->trainers[i].typeC <= 2)
            ? static_cast<trainerType>(m->trainers[i].typeC)
            : static_cast<trainerType>(rand() % 2 + 1);
        m->trainers[i].nextTurn = 0;
        m->trainers[i].defeated = 0;
        m->trainers[i].x = 0;
        m->trainers[i].y = 0;
        m->trainers[i].curDir = rand() % 4;
    }

    for(int i = 0; i < numNPCs; i++){
        assignTrainerParty(&m->trainers[i], x, y);
        m->trainers[i].potions = 0;
        m->trainers[i].revives = 0;
        m->trainers[i].pokeballs = 0;
    }

    for(int i = 0; i < numNPCs; i++){
        randomValidPosition(&m->trainers[i], m);
    }

    return m;
}

void freeMap(Map *m) {
    if (m) {
        delete[] m->trainers;
        delete m;
    }
}

void fillRest(char map[21][80]){
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 80; j++) {
            if(map[i][j] == '*') {
                map[i][j] = '^';
            }
        }
    }
}

void displayMap(Map *m, Trainer *pc){
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 80; j++) {
            chtype current = tileAppearance(m, pc, i, j);
            if(!displayInitialized || previousDisplay[i][j] != current){
                mvaddch(i, j, current);
                previousDisplay[i][j] = current;
            }
        }
    }
    displayInitialized = 1;
    refresh();
}

char getNPC(int x, int y, Map *m){
    char output[] = {'\0', 'h', 'r', 'p', 'w', 's', 'e'};
    for(int i = 0; i < numNPCs; i++){
        if(m->trainers[i].x == x && m->trainers[i].y == y){
            return output[m->trainers[i].typeC];
        }
    }
    return '\0';
}

void placeStartingPoints(char map[21][80]){
    int x_coord = 0;
    int y_coord = 0;

    // Place starting points
    for (int i = 0; i < 4; i++) {
        do {
            x_coord = rand() % 78 + 1;
            y_coord = rand() % 19 + 1;
        } while (map[y_coord][x_coord] != '*');
        map[y_coord][x_coord] = '.'; // Clearing
    }
    for (int i = 4; i < 6; i++) {
        do {
            x_coord = rand() % 78 + 1;
            y_coord = rand() % 19 + 1;
        } while (map[y_coord][x_coord] != '*');
        map[y_coord][x_coord] = ':'; // Tall Grass
    }
    for (int i = 6; i < 7; i++) {
        do {
            x_coord = rand() % 78 + 1;
            y_coord = rand() % 19 + 1;
        } while (map[y_coord][x_coord] != '*');
        map[y_coord][x_coord] = '~'; // Water
    }
    for (int i = 7; i < 8; i++) {
        do {
            x_coord = rand() % 78 + 1;
            y_coord = rand() % 19 + 1;
        } while (map[y_coord][x_coord] != '*');
        map[y_coord][x_coord] = '%'; // Bolder
    }
    for (int i = 8; i < 11; i++) {
        do {
            x_coord = rand() % 78 + 1;
            y_coord = rand() % 19 + 1;
        } while (map[y_coord][x_coord] != '*');
        map[y_coord][x_coord] = '^'; // Trees
    }
}

// Expands regions from their starting points flood style
// Priority: 1 Clearing > 2 Tall Grass > 3 Water > 4 Trees > 5 Boulders
void growRegions(char map[21][80]){
    // Starts at (1,1), if empty, check all 4 adjacent neighbors (excluding the hard boundary) for region type to grow into it
    for (int i = 1; i < 20; i++) {
        for (int j = 1; j < 79; j++) {
            if (map[i][j] == '*') {
                // Check neighbors
                char neighbors[4] = {
                    map[i - 1][j], map[i][j - 1], map[i][j + 1], map[i + 1][j]
                };

                if (i == 1) { // Top row
                    neighbors[0] = '*';
                }
                if (i == 19) { // Bottom row
                    neighbors[3] = '*';
                }
                if (j == 1) { // Left column
                    neighbors[1] = '*';
                }
                if (j == 78) { // Right column
                    neighbors[2] = '*';
                }

                int clearing = 0;
                int tall_grass = 0;
                int water = 0;
                int trees = 0;
                int boulders = 0;

                for (int k = 0; k < 4; k++) {
                    if (neighbors[k] == '.') {
                        clearing++;
                    } else if (neighbors[k] == ':') {
                        tall_grass++;
                    } else if (neighbors[k] == '~') {
                        water++;
                    } else if (neighbors[k] == '^') {
                        trees++;
                    } else if (neighbors[k] == '%') {
                        boulders++;
                    }
                }

                // Prioritized growth, becomes whatever region has the highest count, if tied use the priority order from above
                if(clearing + tall_grass + water + trees + boulders > 0) {
                    if(clearing >= tall_grass && clearing >= water && clearing >= trees && clearing >= boulders) {
                        map[i][j] = '1';
                    }
                    else if(tall_grass >= water && tall_grass >= trees && tall_grass >= boulders) {
                        map[i][j] = '2';
                    }
                    else if(water >= trees && water >= boulders) {
                        map[i][j] = '3';
                    }
                    else if(trees >= boulders) {
                        map[i][j] = '4';
                    }
                    else {
                        map[i][j] = '5';
                    }
                }
            }
        }
    }

    for (int i = 1; i < 20; i++) {
        for (int j = 1; j < 79; j++) {
            if (map[i][j] == '1') {
                map[i][j] = '.';
            } else if (map[i][j] == '2') {
                map[i][j] = ':';
            } else if (map[i][j] == '3') {
                map[i][j] = '~';
            } else if (map[i][j] == '4') {
                map[i][j] = '^';
            } else if (map[i][j] == '5') {
                map[i][j] = '%';
            }
        }
    }
}

// Checks for empty spaces on the map
int isFull(char map[21][80]) {
    for (int i = 1; i < 20; i++) {
        for (int j = 1; j < 79; j++) {
            if (map[i][j] == '*') {
                return 0;
            }
        }
    }
    return 1;
}

// Place building {Pokemart (M) and Pokemon Center (C)} with the constaint that they must be 2x2 and adjacent to the path (#)
void placeBuilding(char map[21][80], char building) {
    int placed = 0;
    int iteration = 0;
    while (!placed) {
        int x = rand() % 74 + 3;
        int y = rand() % 15 + 3;

        // if path found is horizontal to the right
        if(map[y][x] == '#' && (map[y][x + 1] == '#')) {
            // if both above and below work, randomly choose one
            if((map[y - 1][x] == '*' && map[y - 1][x + 1] == '*' && map[y - 2][x] == '*' && map[y - 2][x + 1] == '*') && (y < 18 && map[y + 1][x] == '*' && map[y + 1][x + 1] == '*' && map[y + 2][x] == '*' && map[y + 2][x + 1] == '*')) {
                if(rand() % 2 == 0) {
                    map[y - 1][x] = building;
                    map[y - 1][x + 1] = building;
                    map[y - 2][x] = building;
                    map[y - 2][x + 1] = building;
                } else {
                    map[y + 1][x] = building;
                    map[y + 1][x + 1] = building;
                    map[y + 2][x] = building;
                    map[y + 2][x + 1] = building;
                }
                placed = 1;
            }
            // if only above
            else if(map[y - 1][x] == '*' && map[y - 1][x + 1] == '*' && map[y - 2][x] == '*' && map[y - 2][x + 1] == '*') {
                map[y - 1][x] = building;
                map[y - 1][x + 1] = building;
                map[y - 2][x] = building;
                map[y - 2][x + 1] = building;
                placed = 1;
            // if only below
            } else if(map[y + 1][x] == '*' && map[y + 1][x + 1] == '*' && map[y + 2][x] == '*' && map[y + 2][x + 1] == '*') {
                map[y + 1][x] = building;
                map[y + 1][x + 1] = building;
                map[y + 2][x] = building;
                map[y + 2][x + 1] = building;
                placed = 1;
            }
        }

        // if path found is horizontal to the left
        else if(map[y][x] == '#' && (map[y][x - 1] == '#')) {
            // if both above and below work, randomly choose one
            if((map[y - 1][x] == '*' && map[y - 1][x - 1] == '*' && map[y - 2][x] == '*' && map[y - 2][x - 1] == '*') && (y < 18 && map[y + 1][x] == '*' && map[y + 1][x - 1] == '*' && map[y + 2][x] == '*' && map[y + 2][x - 1] == '*')) {
                if(rand() % 2 == 0) {
                    map[y - 1][x] = building;
                    map[y - 1][x - 1] = building;
                    map[y - 2][x] = building;
                    map[y - 2][x - 1] = building;
                } else {
                    map[y + 1][x] = building;
                    map[y + 1][x - 1] = building;
                    map[y + 2][x] = building;
                    map[y + 2][x - 1] = building;
                }
                placed = 1;
            }
            // if only above
            else if(map[y - 1][x] == '*' && map[y - 1][x - 1] == '*' && map[y - 2][x] == '*' && map[y - 2][x - 1] == '*') {
                map[y - 1][x] = building;
                map[y - 1][x - 1] = building;
                map[y - 2][x] = building;
                map[y - 2][x - 1] = building;
                placed = 1;
            // if only below
            } else if(map[y + 1][x] == '*' && map[y + 1][x - 1] == '*' && map[y + 2][x] == '*' && map[y + 2][x - 1] == '*') {
                map[y + 1][x] = building;
                map[y + 1][x - 1] = building;
                map[y + 2][x] = building;
                map[y + 2][x - 1] = building;
                placed = 1;
            }
        }

        // if path is vertical to the top
        else if(map[y][x] == '#' && (map[y - 1][x] == '#')) {
            // if both left and right work, randomly choose one
            if((map[y][x - 1] == '*' && map[y - 1][x - 1] == '*' && map[y][x - 2] == '*' && map[y - 1][x - 2] == '*') && (x < 77 && map[y][x + 1] == '*' && map[y - 1][x + 1] == '*' && map[y][x + 2] == '*' && map[y - 1][x + 2] == '*')) {
                if(rand() % 2 == 0) {
                    map[y][x - 1] = building;
                    map[y - 1][x - 1] = building;
                    map[y][x - 2] = building;
                    map[y - 1][x - 2] = building;
                } else {
                    map[y][x + 1] = building;
                    map[y - 1][x + 1] = building;
                    map[y][x + 2] = building;
                    map[y - 1][x + 2] = building;
                }
                placed = 1;
            }
            // if only left
            else if(map[y][x - 1] == '*' && map[y - 1][x - 1] == '*' && map[y][x - 2] == '*' && map[y - 1][x - 2] == '*') {
                map[y][x - 1] = building;
                map[y - 1][x - 1] = building;
                map[y][x - 2] = building;
                map[y - 1][x - 2] = building;
                placed = 1;
            // if only right
            } else if(map[y][x + 1] == '*' && map[y - 1][x + 1] == '*' && map[y][x + 2] == '*' && map[y - 1][x + 2] == '*') {
                map[y][x + 1] = building;
                map[y - 1][x + 1] = building;
                map[y][x + 2] = building;
                map[y - 1][x + 2] = building;
                placed = 1;
            }
        }

        // if path is vertical to the bottom
        else if(map[y][x] == '#' && (map[y + 1][x] == '#')) {
            // if both left and right work, randomly choose one
            if((map[y][x - 1] == '*' && map[y + 1][x - 1] == '*' && map[y][x - 2] == '*' && map[y + 1][x - 2] == '*') && (x < 77 && map[y][x + 1] == '*' && map[y + 1][x + 1] == '*' && map[y][x + 2] == '*' && map[y + 1][x + 2] == '*')) {
                if(rand() % 2 == 0) {
                    map[y][x - 1] = building;
                    map[y + 1][x - 1] = building;
                    map[y][x - 2] = building;
                    map[y + 1][x - 2] = building;
                } else {
                    map[y][x + 1] = building;
                    map[y + 1][x + 1] = building;
                    map[y][x + 2] = building;
                    map[y + 1][x + 2] = building;
                }
                placed = 1;
            }
            // if only left
            else if(map[y][x - 1] == '*' && map[y + 1][x - 1] == '*' && map[y][x - 2] == '*' && map[y + 1][x - 2] == '*') {
                map[y][x - 1] = building;
                map[y + 1][x - 1] = building;
                map[y][x - 2] = building;
                map[y + 1][x - 2] = building;
                placed = 1;
            // if only right
            } else if(map[y][x + 1] == '*' && map[y + 1][x + 1] == '*' && map[y][x + 2] == '*' && map[y + 1][x + 2] == '*') {
                map[y][x + 1] = building;
                map[y + 1][x + 1] = building;
                map[y][x + 2] = building;
                map[y + 1][x + 2] = building;
                placed = 1;
            }
        }

        if(iteration++ > 1000) {
            break; // Prevent infinite loop in case of impossible placement
        }
    }
}

void makePathes(char map[21][80], int exits[4]) {
    // Create North-South path
    // If north gate not fixed, randomize it
    if (!exits[0]) {
        exits[0] = rand() % 78 + 1;
    }

    int current_row = 1;
    int current_col = exits[0];

    // Set north gate
    map[0][exits[0]] = '#';

    // Determine target exit
    int target_exit;
    if (exits[1]) {
        target_exit = exits[1];  // fixed from south neighbor
    } else {
        target_exit = rand() % 78 + 1;  // free generation
    }

    int run = 0;
    int max_run = 7;

    while (!(current_row == 19 && current_col == target_exit)) {

        map[current_row][current_col] = '#';

        int dx = abs(current_col - target_exit);
        int dy = 19 - current_row;

        int d_row = 0;
        int d_col = 0;

        // If already aligned horizontally → move south
        if (current_col == target_exit) {
            d_row = 1;
            run = 0;
        }
        else {

            // Base south bias
            int south_bias = 70;

            // Gradually reduce south bias as we go down
            south_bias -= current_row * 2;
            if (south_bias < 30)
                south_bias = 30;

            // If horizontal gap is too risky, increase horizontal correction
            if (dx > dy + 2) {
                south_bias = 20;
            }

            if (run >= max_run || rand() % 100 < south_bias) {
                d_row = 1;
                run = 0;
            } else {
                d_row = 0;
                d_col = (current_col < target_exit) ? 1 : -1;
                run++;
            }
        }

        current_row += d_row;
        current_col += d_col;

        // Clamp horizontal bounds
        current_col = current_col < 1 ? 1 : current_col > 78 ? 78 : current_col;
        current_row = current_row > 19 ? 19 : current_row;
    }

    // Final bottom placement
    if (!exits[1]) {
        exits[1] = current_col;
    }

    map[19][exits[1]] = '#';
    map[20][exits[1]] = '#';


    // Create West-East path
    if (!exits[3]) {
        exits[3] = rand() % 19 + 1;
    }

    current_row = exits[3];
    current_col = 1;

    // Set west gate
    map[exits[3]][0] = '#';

    // Determine target exit
    if (exits[2]) {
        target_exit = exits[2];  // fixed from east neighbor
    } else {
        target_exit = rand() % 19 + 1;  // free generation
    }

    run = 0;
    max_run = 7;

    while (!(current_col == 78 && current_row == target_exit)) {
        map[current_row][current_col] = '#';
        int dy = abs(current_row - target_exit);
        int dx = 78 - current_col;

        int d_row = 0;
        int d_col = 0;

        // If already aligned vertically → move east
        if (current_row == target_exit) {
            if (rand() % 100 < 80) {
                d_col = 1;        // mostly east
                d_row = 0;
                run = 0;
            } else {
                d_col = 0;
                d_row = (rand() % 2 == 0) ? 1 : -1;  // small vertical wobble
                run = (rand() % 4) - 7;
            }
        }
        else {
            // Base east bias
            int east_bias = 70;
            // Gradually reduce east bias as we go right
            east_bias -= (current_col * 40) / 78;
            if (east_bias < 30)
                east_bias = 30;
            // If vertical gap is too risky, increase vertical correction
            if (dy > dx + 2) {
                east_bias = 20;
            }

            if(run < 0){
                d_col = 1;
                run++;
            }
            else if (run >= max_run || rand() % 100 < east_bias) {
                d_col = 1;
                run = 0;
            } else {
                d_col = 0;
                d_row = (current_row < target_exit) ? 1 : -1;
                run++;
            }
        }

        current_row += d_row;
        current_col += d_col;

        // Clamp vertical bounds
        current_row = current_row < 1 ? 1 : current_row > 19 ? 19 : current_row;
        current_col = current_col > 78 ? 78 : current_col;
    }

    // Final east placement
    if (!exits[2]) {
        exits[2] = current_row;
    }

    map[exits[2]][78] = '#';
    map[exits[2]][79] = '#';
}

// Sets the borders to % and the interior to *
void initializeBorders(char map[21][80]) {
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 80; j++) {
            if (i == 0 || i == 20 || j == 0 || j == 79) {
                map[i][j] = '%';
            } else {
                map[i][j] = '*';
            }
        }
    }
}

void changeMaps(char dir, Trainer *p, Map *world[401][401], int *wx, int *wy){
    if(dir == 'w'){
        (*wy)--;
        p->y = MAP_HEIGHT - 2;
    }
    if(dir == 's'){
        p->y = 1;
        (*wy)++;
    }
    if(dir == 'a'){
        p->x = MAP_WIDTH - 2;
        (*wx)--;
    }
    if(dir == 'd'){
        p->x = 1;
        (*wx)++;
    }

    if(!world[*wy][*wx]){
        world[*wy][*wx] = generateMap((*wx), (*wy), (*wy) == 0 ? nullptr : world[(*wy)-1][(*wx)], (*wy) == 400 ? nullptr : world[(*wy)+1][(*wx)], (*wx) == 400 ? nullptr : world[(*wy)][(*wx)+1], (*wx) == 0 ? nullptr : world[(*wy)][(*wx)-1]);
    }
}
