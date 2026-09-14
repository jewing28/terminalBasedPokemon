#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "constants.h"
#include "trainer.h"

void fillQueue(Queue *q, Map *m){
    for(int i = 0; i < numNPCs; i++){
        add(q, &m->trainers[i]);
    }
}

void add(Queue *q, Trainer *t){
    if (q->size >= q->capacity) {
        return;
    }

    int cur = q->size;
    q->heap[cur] = t;
    q->size++;

    while(cur != 0){
        if(q->heap[cur]->nextTurn < q->heap[(cur-1)/2]->nextTurn){
            q->heap[cur] = q->heap[(cur-1)/2];
            q->heap[(cur-1)/2] = t;
            cur = (cur-1)/2;
        }
        else{
            break;
        }
    }
}

Trainer* queueRemove(Queue *q){
    Trainer* temp = q->heap[0];
    q->size--;
    q->heap[0] = q->heap[q->size];
    q->heap[q->size] = nullptr;
    int cur = 0;
    while(cur * 2 + 2 < q->size){
        if(q->heap[cur * 2 + 1]->nextTurn <= q->heap[cur * 2 + 2]->nextTurn && q->heap[cur]->nextTurn > q->heap[cur * 2 + 1]->nextTurn){
            Trainer* swap = q->heap[cur];
            q->heap[cur] = q->heap[cur * 2 + 1];
            q->heap[cur * 2 + 1] = swap;
            cur = cur * 2 + 1;
        }
        else if(q->heap[cur * 2 + 1]->nextTurn > q->heap[cur * 2 + 2]->nextTurn && q->heap[cur]->nextTurn > q->heap[cur * 2 + 2]->nextTurn){
            Trainer* swap = q->heap[cur];
            q->heap[cur] = q->heap[cur * 2 + 2];
            q->heap[cur * 2 + 2] = swap;
            cur = cur * 2 + 2;
        }
        else{
            break;
        }
    }
    if(cur * 2 + 2 == q->size && q->heap[cur]->nextTurn > q->heap[cur * 2 + 1]->nextTurn){
        Trainer* swap = q->heap[cur];
        q->heap[cur] = q->heap[cur * 2 + 1];
        q->heap[cur * 2 + 1] = swap;
        cur = cur * 2 + 1;
    }
    return temp;
}

void emptyQueue(Queue *q){
    while (q->size > 0) {
        q->size--;
        q->heap[q->size] = nullptr;
    }
}
