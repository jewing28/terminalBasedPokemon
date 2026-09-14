#include <stdlib.h>
#include "trainer.h"
#include "pathfinding.h"

void randomValidPosition(Trainer *t, Map *m)
{
    int x = (rand() % (MAP_WIDTH - 2)) + 1;
    int y = (rand() % (MAP_HEIGHT - 2)) + 1;

    while (getCost(m->map[y][x], t->typeT, x, y) == INF || !checkNPCs(x, y, m)) {
        x = (rand() % (MAP_WIDTH - 2)) + 1;
        y = (rand() % (MAP_HEIGHT - 2)) + 1;
    }

    t->x = x;
    t->y = y;
}

int checkNPCs(int x, int y, Map *m)
{
    if (x == pc.x && y == pc.y) {
        return 0;
    }

    for (int i = 0; i < numNPCs; i++) {
        if (x == m->trainers[i].x && y == m->trainers[i].y) {
            return 0;
        }
    }

    return 1;
}

int checkNPCsPlayer(int x, int y, Map *m)
{
    for (int i = 0; i < numNPCs; i++) {
        if (m->trainers[i].defeated) {
            continue;
        }
        if (x == m->trainers[i].x && y == m->trainers[i].y) {
            return 0;
        }
    }

    return 1;
}

int movePC(char dir, Trainer *p, Map *m, char *exitDir)
{
    int x = (dir == 'a' || dir == 'q' || dir == 'z') ? p->x - 1 :
            (dir == 'd' || dir == 'e' || dir == 'c') ? p->x + 1 : p->x;
    int y = (dir == 'w' || dir == 'q' || dir == 'e') ? p->y - 1 :
            (dir == 's' || dir == 'z' || dir == 'c') ? p->y + 1 : p->y;

    if (exitDir) {
        *exitDir = '\0';
    }

    if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH) {
        if (exitDir) {
            if (y < 0) {
                *exitDir = 'w';
            } else if (y >= MAP_HEIGHT) {
                *exitDir = 's';
            } else if (x < 0) {
                *exitDir = 'a';
            } else if (x >= MAP_WIDTH) {
                *exitDir = 'd';
            }
        }
        return 0;
    }

    if (((m->map[y][x] != '.') && (m->map[y][x] != ':') && (m->map[y][x] != '#') &&
         (m->map[y][x] != 'C') && (m->map[y][x] != 'M')) || !checkNPCsPlayer(x, y, m)) {
        return 1;
    }

    p->x = x;
    p->y = y;

    if (m->map[y][x] == '#') {
        if (y == 0) {
            if (exitDir) {
                *exitDir = 'w';
            }
            return 0;
        }
        if (y == MAP_HEIGHT - 1) {
            if (exitDir) {
                *exitDir = 's';
            }
            return 0;
        }
        if (x == 0) {
            if (exitDir) {
                *exitDir = 'a';
            }
            return 0;
        }
        if (x == MAP_WIDTH - 1) {
            if (exitDir) {
                *exitDir = 'd';
            }
            return 0;
        }
    }

    return m->map[p->y][p->x] == ':' ? 20 : 10;
}

int moveNPC(Trainer *t, Map *m, int dist[MAP_HEIGHT][MAP_WIDTH])
{
    int target[2];

    if (dist) {
        int bestX = t->x;
        int bestY = t->y;
        int best = dist[t->y][t->x];

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) {
                    continue;
                }

                int nx = t->x + dx;
                int ny = t->y + dy;

                if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) {
                    continue;
                }
                if (!checkNPCs(nx, ny, m)) {
                    continue;
                }
                if (getCost(m->map[ny][nx], t->typeT, nx, ny) != INF && dist[ny][nx] < best) {
                    best = dist[ny][nx];
                    bestX = nx;
                    bestY = ny;
                }
            }
        }

        t->x = bestX;
        t->y = bestY;
        return getCost(m->map[t->y][t->x], t->typeT, t->x, t->y);
    }

    if (t->typeC == CLASS_PACER) {
        target[0] = t->x;
        target[1] = t->y;
        target[0] += t->curDir == 2 ? -1 : t->curDir == 3 ? 1 : 0;
        target[1] += t->curDir == 0 ? -1 : t->curDir == 1 ? 1 : 0;

        if (validSpace(t, m, target[0], target[1])) {
            t->x = target[0];
            t->y = target[1];
            return getCost(m->map[t->y][t->x], t->typeT, t->x, t->y);
        }

        t->curDir = ((t->curDir & 2) == 0) ? t->curDir + 1 : t->curDir - 1;
        return 1;
    }

    if (t->typeC == CLASS_WANDERER) {
        char curTerrain = m->map[t->y][t->x];

        target[0] = t->x;
        target[1] = t->y;
        target[0] += t->curDir == 2 ? -1 : t->curDir == 3 ? 1 : 0;
        target[1] += t->curDir == 0 ? -1 : t->curDir == 1 ? 1 : 0;

        if (validSpace(t, m, target[0], target[1]) && m->map[target[1]][target[0]] == curTerrain) {
            t->x = target[0];
            t->y = target[1];
            return getCost(m->map[t->y][t->x], t->typeT, t->x, t->y);
        }

        t->curDir = rand() % 4;
        return 1;
    }

    if (t->typeC == CLASS_EXPLORER) {
        target[0] = t->x;
        target[1] = t->y;
        target[0] += t->curDir == 2 ? -1 : t->curDir == 3 ? 1 : 0;
        target[1] += t->curDir == 0 ? -1 : t->curDir == 1 ? 1 : 0;

        if (target[1] >= 0 && target[1] < MAP_HEIGHT && target[0] >= 0 && target[0] < MAP_WIDTH &&
            checkNPCs(target[0], target[1], m) &&
            getCost(m->map[target[1]][target[0]], t->typeT, target[0], target[1]) != INF) {
            t->x = target[0];
            t->y = target[1];
            return getCost(m->map[t->y][t->x], t->typeT, t->x, t->y);
        }

        t->curDir = rand() % 4;
        return 1;
    }

    return 1;
}

int validSpace(Trainer *t, Map *m, int x, int y)
{
    if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH) {
        return 0;
    }
    if (!checkNPCs(x, y, m)) {
        return 0;
    }

    return getCost(m->map[y][x], t->typeT, x, y) != INF;
}
