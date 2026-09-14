#ifndef QUEUE_H
#define QUEUE_H

class Trainer;
class Map;

struct Queue {
    Trainer **heap;
    int size;
    int capacity;
};

Trainer* queueRemove(Queue *q);
void add(Queue *q, Trainer *t);
void fillQueue(Queue *q, Map *m);
void emptyQueue(Queue *q);

#endif
