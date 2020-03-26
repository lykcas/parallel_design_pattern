//
// Created by YUKE LI on 2020/3/11.
//

#ifndef UNTITLED_SQUIRREL_H
#define UNTITLED_SQUIRREL_H
#include "sauirrel_queue.h"


#define LANDCELL 16
#define SQUIRREL 34
#define BUFFSIZE 5000
#define MONTH 24

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

void initMPI(int argc, char *argv[]);
void workerLandCode(int id);
void workerClockCode(int id);
void workerSquirrelCode(int id);
void workerCode();
void initWorld();
void actorCode();
//void workers(void (*fn_pointer)(int), int id);
void runSimulation();
//void squirrelWorkerCode(struct squirrels *squirrel, int id);
//void clockWorkerCode();
//void landcellWorkerCode(struct landcell *land, int id);


#endif //UNTITLED_SQUIRREL_H
