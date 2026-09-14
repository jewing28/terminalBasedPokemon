#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pathfinding.h"

static int isBorderGate(int x, int y, char tile)
{
    return tile == '#' &&
           (y == 0 || y == MAP_HEIGHT - 1 || x == 0 || x == MAP_WIDTH - 1);
}

static int topLeft(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);
static int topMiddle(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);
static int topRight(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);
static int midLeft(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);
static int midRight(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);
static int botLeft(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);
static int botMiddle(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);
static int botRight(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]);

int getCost(char tile, trainerType typeT, int x, int y){
    if(typeT == TRAINER_PC){
        if(tile == '%' || tile == '^' || tile == '~'){
            return INF;
        }
        else if(tile == ':'){
            return 15;
        }
        else if(tile == '.' || tile == 'M' || tile == 'C' || tile == '#'){
            return 5;
        }
    }
    else if(typeT == TRAINER_HIKER){
        if((tile == '%' && (y == 0 || y == 20 || x == 0 || x == 79)) ||
           isBorderGate(x, y, tile) || tile == '^' || tile == '~'){
            return INF;
        }
        else if(tile == 'M' || tile == 'C'){
            return 50;
        }
        else if(tile == ':' || tile == '%'){
            return 15;
        }
        else if(tile == '.' || tile == '#'){
            return 10;
        }
    }
    else if(typeT == TRAINER_RIVAL){
        if(tile == '%' || isBorderGate(x, y, tile) || tile == '^' || tile == '~'){
            return INF;
        }
        else if(tile == 'M' || tile == 'C'){
            return 50;
        }
        else if(tile == ':'){
            return 20;
        }
        else if(tile == '.' || tile == '#'){
            return 10;
        }
    }
    else if(typeT == TRAINER_SWIMMER){
        if(tile == '~'){
            return 7;
        }
        else{
            return INF;
        }
    }

    return INF;
}

void dijkstra(char map[MAP_HEIGHT][MAP_WIDTH], int startX, int startY, int dist[MAP_HEIGHT][MAP_WIDTH], trainerType typeT){
    int visited[MAP_HEIGHT][MAP_WIDTH];

    for(int i = 0; i < MAP_HEIGHT; i++){
        for(int j = 0; j < MAP_WIDTH; j++){
            dist[i][j] = INF;
            visited[i][j] = 0;
        }
    }

    dist[startY][startX] = 0;

    while(true){
        int x = INF, y = INF, minValid = INF;
        for(int i = 0; i < MAP_HEIGHT; i++){
            for(int j = 0; j < MAP_WIDTH; j++){
                if(!visited[i][j] && dist[i][j] < minValid){
                    y = i;
                    x = j;
                    minValid = dist[i][j];
                }
            }
        }
        if(x == INF && y == INF){
            break;
        }

        if(topLeft(x, y, visited)){
            int cost = getCost(map[y-1][x-1], typeT, x-1, y-1);
            if(cost != INF && dist[y][x] + cost < dist[y-1][x-1]){
                dist[y-1][x-1] = dist[y][x] + cost;
            }
        }
        if(topMiddle(x, y, visited)){
            int cost = getCost(map[y-1][x], typeT, x, y-1);
            if(cost != INF && dist[y][x] + cost < dist[y-1][x]){
                dist[y-1][x] = dist[y][x] + cost;
            }
        }
        if(topRight(x, y, visited)){
            int cost = getCost(map[y-1][x+1], typeT, x+1, y-1);
            if(cost != INF && dist[y][x] + cost < dist[y-1][x+1]){
                dist[y-1][x+1] = dist[y][x] + cost;
            }
        }
        if(midLeft(x, y, visited)){
            int cost = getCost(map[y][x-1], typeT, x-1, y);
            if(cost != INF && dist[y][x] + cost < dist[y][x-1]){
                dist[y][x-1] = dist[y][x] + cost;
            }
        }
        if(midRight(x, y, visited)){
            int cost = getCost(map[y][x+1], typeT, x+1, y);
            if(cost != INF && dist[y][x] + cost < dist[y][x+1]){
                dist[y][x+1] = dist[y][x] + cost;
            }
        }
        if(botLeft(x, y, visited)){
            int cost = getCost(map[y+1][x-1], typeT, x-1, y+1);
            if(cost != INF && dist[y][x] + cost < dist[y+1][x-1]){
                dist[y+1][x-1] = dist[y][x] + cost;
            }
        }
        if(botMiddle(x, y, visited)){
            int cost = getCost(map[y+1][x], typeT, x, y+1);
            if(cost != INF && dist[y][x] + cost < dist[y+1][x]){
                dist[y+1][x] = dist[y][x] + cost;
            }
        }
        if(botRight(x, y, visited)){
            int cost = getCost(map[y+1][x+1], typeT, x+1, y+1);
            if(cost != INF && dist[y][x] + cost < dist[y+1][x+1]){
                dist[y+1][x+1] = dist[y][x] + cost;
            }
        }
        visited[y][x] = 1;
    }
}

static int topLeft(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return x > 0 && y > 0 && !visited[y-1][x-1];
}

static int topMiddle(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return y > 0 && !visited[y-1][x];
}

static int topRight(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return y > 0 && x < 79 && !visited[y-1][x+1];
}

static int midLeft(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return x > 0 && !visited[y][x-1];
}
static int midRight(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return x < 79 && !visited[y][x+1];
}

static int botLeft(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return y < 20 && x > 0 && !visited[y+1][x-1];
}

static int botMiddle(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return y < 20 && !visited[y+1][x];
}

static int botRight(int x, int y, int visited[MAP_HEIGHT][MAP_WIDTH]){
    return y < 20 && x < 79 && !visited[y+1][x+1];
}

void displayDistanceTable(int dist[MAP_HEIGHT][MAP_WIDTH]){
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 80; j++) {
            if(dist[i][j] == INF){
                printf("   ");
            }
            else{
                if(dist[i][j] == 0){
                    printf("@@ "); //to display to location of the player character on the distance maps
                }
                else if(dist[i][j] % 100 < 10){
                    printf("0%d ", dist[i][j] % 100);
                }
                else{
                    printf("%d ", dist[i][j] % 100);
                }
            }
        }
        printf("\n");
    }
}
