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

// Init the MPI environment
void initMPI(int argc, char *argv[]);
// Implement what a land cell need to do
void workerLandCode(int id);
// Implement what a clock need to do
void workerClockCode(int id);
// Implement what a squirrel need to do
void workerSquirrelCode(int id);
// The wrapper of workerLandCode(), workerClockCode() and workerSquirrelCode()
void workerCode();
// Implement init the squirrel world by Master Actor
void initWorld();
// Implement what a Master Actor need to do
void actorCode();
// Entry to all code
void runSimulation();


#endif //UNTITLED_SQUIRREL_H
