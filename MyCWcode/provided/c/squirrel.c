
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "mpi.h"
#include "pool.h"
#include "squirrel-functions.h"
#include "squirrel.h"


void initMPI(int argc, char *argv[]) {
    // init MPI environment
    MPI_Init(&argc, &argv);
    // Specify the buffer size to make sure the safety of the message.
    int buffer[BUFFSIZE];
    MPI_Buffer_attach(buffer, BUFFSIZE);
}

void workerLandCode(int id) {
    // Init a struct of landcell
    struct landcell *land;
    land = (struct landcell *) malloc(sizeof(struct landcell));
    // Do as landcell's do
    int i, switch_update = 0, fullflag = 0, myRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    land->id = id;
    land->populationInflux = 0;
    for (i = 0; i < 4; i++) {
        land->thisMonthPopulation[i] = 0;
        land->thisMonthInfect[i] = 0;
    }
    land->infectionLevel = 0;
    int workerStatus = 0;
    int months = 0;
    while (!workerStatus) {
        int count = 0, flag = 0;
        MPI_Status status;
        MPI_Request req;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag) { 
            count = 0;
            MPI_Get_count(&status, MPI_INT, &count);
            // Receive information from clocsk to update the values of populationInflux and infectionLevel
            if (count == 1) { 
                MPI_Recv(&switch_update, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                months++;
                land->populationInflux = 0;
                land->infectionLevel = 0;
                if (fullflag != 3) {
                    land->thisMonthPopulation[fullflag] = land->thisMonthPopulation[3];
                    land->thisMonthInfect[fullflag] = land->thisMonthInfect[3];
                    land->thisMonthPopulation[3] = 0;
                    land->thisMonthInfect[3] = 0;
                    fullflag++;
                } else {
                    for (i = 0; i < 3; i++) {
                        land->thisMonthPopulation[i] = land->thisMonthPopulation[i+1];
                        land->thisMonthInfect[i] = land->thisMonthInfect[i+1];
                    }
                    land->thisMonthPopulation[3] = 0;
                    land->thisMonthInfect[3] = 0;
                }
                for (i = 0; i < 3; i++) {
                    land->populationInflux += land->thisMonthPopulation[i];
                }
                for (i = 0; i < 2; i++) {
                    land->infectionLevel += land->thisMonthInfect[i];
                }
                printf("No. %d land. My population is %d, My infection is %d\n", land->id, land->populationInflux, land->infectionLevel);

            } 
            // Receive a message from the squirrel and send it back to your populationInflux and infection levels
            else if (count == 2) { 
                int squirrelInfo[2] = {0};
                int landInfo[2] = {land->populationInflux, land->infectionLevel};
                MPI_Recv(squirrelInfo, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // Send back own information to squirrel
                MPI_Isend(landInfo, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &req);
                if (squirrelInfo[1] == 1) land->thisMonthInfect[3]++;
                land->thisMonthPopulation[3]++;
            }


        }
        workerStatus = shouldWorkerStop();
    }
}

void workerClockCode(int id) {
    // Do as clock's do
    int months = MONTH, i, switch_update = 1, myRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    int landcount = LANDCELL;
    MPI_Request req;
    while (months) {
        sleep(1);
        printf("------------This is month %d----------\n", MONTH-months+1);
        for (i = 0; i < landcount; i++) {
            MPI_Isend(&switch_update, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, &req);
        }
        months--;
        sleep(1);
    }
    shutdownPool();
}

void workerSquirrelCode(int id) {
    
    // Init a struct of squirrel
    struct squirrels *squirrel;
    squirrel = (struct squirrels *) malloc(sizeof(struct squirrels));
    // Do as squirrel's do
    int myRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    squirrel->id = id;
    long seed = -1 - myRank;
    initialiseRNG(&seed);
    int parentId = getCommandData();
    MPI_Status status;

    float position[2] = {0};
    if (parentId == 0) {
        squirrel->x = 0.0;
        squirrel->y = 0.0;
        squirrel->whetherInfect = 0;
        if ((LANDCELL + 3) < myRank && myRank < (LANDCELL + 8)) {
            squirrel->whetherInfect = 1;
        }
    } else {
        MPI_Recv(position, 2, MPI_FLOAT, parentId, 1, MPI_COMM_WORLD, &status); // 接收父松鼠的位置
        squirrel->x = position[0];
        squirrel->y = position[1];
        squirrel->whetherInfect = 0;
    }
    squirrel->step_total = 0;
    squirrel->step_50 = 0;
    squirrel->switch_infect = 0;
    squirrel->population50 = 0.0;
    InitQueue(&squirrel->q);
    int workerStatus = 0, nosleep = 1;
    int asd = 0;
    while (!workerStatus && nosleep) {
        MPI_Request req;
        float x = squirrel->x, y = squirrel->y;
        squirrelStep(x, y, &squirrel->x, &squirrel->y, &seed);
        position[0] = squirrel->x;
        position[1] = squirrel->y;
        int this_land_Pid = getCellFromPosition(squirrel->x, squirrel->y);
        this_land_Pid++;

        squirrel->step_total += 1;
        squirrel->step_50 += 1;
        int info_squirrel[2] = {0};
        info_squirrel[0] = squirrel->id;
        info_squirrel[1] = squirrel->whetherInfect;
        MPI_Isend(info_squirrel, 2, MPI_INT, this_land_Pid, 0, MPI_COMM_WORLD, &req); 
        // info_lancell[0] is populationInflux，info_lancell[1] infectionLevel
        int info_landcell[2] = {0};
        workerStatus = shouldWorkerStop();
        if (workerStatus == 1) {
            printf("This is squirrel %d. It jumps %d.\n", id, squirrel->step_total);
            break;
        }
        MPI_Recv(info_landcell, 2, MPI_INT, this_land_Pid, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        squirrel->population50 += info_landcell[0]; 
        EnQueue(&squirrel->q, info_landcell[1]);
        // Decide whether or not to have children
        if(squirrel->step_50 == 50) {
            squirrel->population50 = squirrel->population50 / 50.0;
            int giveBirth = 0;
            giveBirth = willGiveBirth(squirrel->population50, &seed);
            if (giveBirth == 1) {
                // Give a birth to
                int childPid = startWorkerProcess();
                int child_info[2] = {3, childPid};
                MPI_Isend(child_info, 2, MPI_INT, childPid, 0, MPI_COMM_WORLD, &req);
                MPI_Isend(position, 2, MPI_FLOAT, childPid, 1, MPI_COMM_WORLD, &req);
            }
            squirrel->step_50 = 0;
            squirrel->population50 = 0;
        }
       // Whether catch disease
        if (squirrel->step_total >=50 && squirrel->switch_infect == 0) {
            float infect50 = QueueCount(squirrel->q) / 50.0;
            int catchDisease = 0;
            catchDisease = willCatchDisease(infect50, &seed);
            if (catchDisease) {
                squirrel->whetherInfect = 1;
                squirrel->switch_infect = 1;
            }
        }
        else if (squirrel->whetherInfect == 1) {
            if (squirrel->switch_infect < 50) {
                squirrel->switch_infect ++;
            } 
            // Whether die
            else { 
                int whetherDie = 0;
                whetherDie = willDie(&seed);
                if (whetherDie) { 
                    nosleep = workerSleep();
                }
            }
        }
        struct timespec ts, ts1;
        ts.tv_nsec = 100000000; // 0.1s
    //        ts.tv_nsec = 10000000; // 0.01s
        ts.tv_sec = 0;
        nanosleep(&ts, &ts1);


    }
}

void workerCode() {
    // 1: landcell; 2: clock; 3:squirrel
    int whoamI[2] = {0}; 
    MPI_Recv(whoamI, 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    void (*fun_ptr_arr[])(int) = {workerLandCode, workerClockCode, workerSquirrelCode};
    (*fun_ptr_arr[whoamI[0]-1])(whoamI[1]);
}

void initWorld() {
    MPI_Request landrequest[LANDCELL], clockrequest, squirrelrequest[SQUIRREL];
    int i, whoyouare[2] = {0};
    // Allocate the specific number of processes to landcell workers
    for (i = 0; i < LANDCELL; i++) {
        int landworkerPid = startWorkerProcess();
        whoyouare[0] = 1;
        whoyouare[1] = landworkerPid;
        MPI_Isend(whoyouare, 2, MPI_INT, landworkerPid, 0, MPI_COMM_WORLD, &landrequest[i]);
    }

    // Allocate one process to clock workers
    int clockworkerPid = startWorkerProcess();
    whoyouare[0] = 2;
    whoyouare[1] = clockworkerPid;
    MPI_Isend(whoyouare, 2, MPI_INT, clockworkerPid, 0, MPI_COMM_WORLD, &clockrequest);

    // Allocate the specific number of processes to squirrel workers
    for (i = 0; i < SQUIRREL; i++) {
        int squirrelworkerPid = startWorkerProcess();
        whoyouare[0] = 3;
        whoyouare[1] = squirrelworkerPid;
        MPI_Isend(whoyouare, 2, MPI_INT, squirrelworkerPid, 0, MPI_COMM_WORLD, &squirrelrequest[i]);
    }

    // Check allocating success
    MPI_Waitall(LANDCELL, landrequest, MPI_STATUS_IGNORE);
    MPI_Wait(&clockrequest, MPI_STATUS_IGNORE);
    MPI_Waitall(SQUIRREL, squirrelrequest, MPI_STATUS_IGNORE);
}

void actorCode() {
    
    // Init the world
    initWorld();

    // Keep working
    int workstatus = masterPoll();
    while (workstatus) {
        workstatus = masterPoll();
    }
}

void runSimulation() {
    //Get the role of this process. 1 means the role is a worker, 2 means the role is a master, 0 means undefined
    int statusCode = processPoolInit();
    if (statusCode == 1) {

        workerCode();

    } else if (statusCode == 2) {

        actorCode();

    }

    processPoolFinalise();
}


int main(int argc, char *argv[]) {
    initMPI(argc, argv);

    runSimulation();

    MPI_Finalize();
    return 0;
}
