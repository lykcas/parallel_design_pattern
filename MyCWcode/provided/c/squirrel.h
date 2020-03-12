//
// Created by YUKE LI on 2020/3/11.
//

#ifndef UNTITLED_SQUIRREL_H
#define UNTITLED_SQUIRREL_H
#include "sauirrel_queue.h"


#define LANDCELL 16
#define SQUIRREL 8
#define BUFFSIZE 500000

struct landcell {
    int id;
    int populationInflux;
    int thisMonthPopulation[4];
    int infectionLevel;
    int thisMonthInfect[4];
};

struct squirrels {
    int id;
    float x;
    float y;
    int step_total;
    int step_50;
    int switch_infect;
    int whetherInfect;
    float population50;
    LinkQueue q;
    QElemType e;
};

void squirrelWorkerCode(struct squirrels *squirrel, int id);
void clockWorkerCode();
void landcellWorkerCode(struct landcell *land, int id);

#endif //UNTITLED_SQUIRREL_H
