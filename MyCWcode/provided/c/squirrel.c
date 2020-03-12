//
// Created by YUKE LI on 2020/3/11.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mpi.h"
#include "pool.h"
#include "squirrel.h"




int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int buffer[BUFFSIZE];
    MPI_Buffer_attach( buffer, BUFFSIZE);
    MPI_Status status;

    int statusCode = processPoolInit();
    if (statusCode == 1) {
        //worker
        // 1: landcell; 2: clock; 3:squirrel
        int whoamI[2] = {0};
        MPI_Recv(whoamI, 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (whoamI[0] == 1) {
//            printf("I am %d! I am %d landcell\n", whoamI[0], whoamI[1]);
            struct landcell *land;
            land = (struct landcell *)malloc(sizeof(struct landcell));
            landcellWorkerCode(land, whoamI[1]);
        } else if (whoamI[0] == 2) {
//            printf("I am %d! I am %d clock\n", whoamI[0], whoamI[1]);
            clockWorkerCode();
        } else if (whoamI[0] == 3) {
            struct squirrels *squirrel;
            squirrel = (struct squirrels *)malloc(sizeof(struct squirrels));
            if (20 < whoamI[1] && whoamI[1] < 25) {
                squirrel->whetherInfect = 1;
                squirrelWorkerCode(squirrel, whoamI[1]);
            } else {
                squirrel->whetherInfect = 0;
                squirrelWorkerCode(squirrel, whoamI[1]);
            }
//            printf("I am %d! I am %d squirrel\n", whoamI[0], whoamI[1]);
        } else {
            printf("?????\n");
        }

    } else if (statusCode ==2) {
        // master
        int i, numberOfSquirrels = 0;
//        int whoyouare = 0; // 1: landcell; 2: clock; 3:squirrel
        int whoyouare[2] = {0};
        // init landcell
        MPI_Request landrequest[LANDCELL];
        for (i = 0; i < LANDCELL; i++) {
            int landworkerPid = startWorkerProcess();
            whoyouare[0] = 1;
            whoyouare[1] = landworkerPid;
            MPI_Isend(whoyouare, 2, MPI_INT, landworkerPid, 0, MPI_COMM_WORLD, &landrequest[i]);
        }

        // init clock
        int clockworkerPid = startWorkerProcess();
        whoyouare[0] = 2;
        whoyouare[1] = clockworkerPid;
        MPI_Request clockrequest;
        MPI_Isend(whoyouare, 2, MPI_INT, clockworkerPid, 0, MPI_COMM_WORLD, &clockrequest);

        // Init squirrel
        MPI_Request squirrelrequest[SQUIRREL];
        for (i = 0; i < SQUIRREL; i++) {
            int squirrelworkerPid = startWorkerProcess();
            whoyouare[0] = 3;
            whoyouare[1] = squirrelworkerPid;
            MPI_Isend(whoyouare, 2, MPI_INT, squirrelworkerPid, 0, MPI_COMM_WORLD, &squirrelrequest[i]);
            numberOfSquirrels++;
        }

        // Check init success
        MPI_Waitall(LANDCELL, landrequest, MPI_STATUS_IGNORE);
        MPI_Wait(&clockrequest, MPI_STATUS_IGNORE);
        MPI_Waitall(SQUIRREL, squirrelrequest, MPI_STATUS_IGNORE);
        printf("Init landcell success!\n Init clock success!\n Init squirrel success!\n");

        //
//        while (numberOfSquirrels) {
//            int event = 0; // 1: start new squirrel; 2: die a squirrel
//            MPI_Recv(&event, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_ANY_SOURCE, MPI_STATUS_IGNORE);
//            if (event == 1) {
//                // start new squirrel
//                squirrelworkerPid = startWorkerProcess();
//                numberOfSquirrels++;
//            } else if (event == 2){
//                // die a squirrel
//                numberOfSquirrels--;
//            }
//        }
        int workstatus = masterPoll();
        while (workstatus) {
            workstatus = masterPoll();
        }
    }
    processPoolFinalise();
    MPI_Finalize();
    return 0;
}

void landcellWorkerCode(struct landcell *land, int id) {
    int flag = 0, switch_update = 0;
    MPI_Status status;
    land->id = id;
    land->populationInflux = 0;
    land->infectionLevel = 0;
    printf("I am a land, my id = %d\n", land->id);
    int shouldStop = shouldWorkerStop();
    while (!shouldStop){
        MPI_Iprobe(LANDCELL+1, 0, MPI_COMM_WORLD, &flag, &status);
        if (flag) { // means need to update!
            // Update
            MPI_Recv(&switch_update, 1, MPI_INT, LANDCELL+1, 0, MPI_COMM_WORLD, &status);
            printf("No. %d land receive update command %d.\n", land->id, switch_update);
        } else {
            //
        }
        shouldStop = shouldWorkerStop();
    }
}

void clockWorkerCode() {
    int months = 5, i, switch_update = 1;
    int landcount = 16;
    MPI_Request req;
    while (months) {
        sleep(1);
        for (i = 0; i < landcount; i++) {
            MPI_Isend(&switch_update, 1, MPI_INT, i+1, 0, MPI_COMM_WORLD, &req);
        }
        months--;
        sleep(1);
    }
    shutdownPool();
}

void squirrelWorkerCode(struct squirrels *squirrel, int id) {
    // 初始化
    int parentId = getCommandData();
    MPI_Status status;
    float position[2] = {0};
    if (parentId == 0) {
        squirrel->x = 0.0;
        squirrel->y = 0.0;
    } else {
        MPI_Recv(position, 2, MPI_FLOAT, parentId, 0, MPI_COMM_WORLD, &status); // 还没写MPI_Send, 接收父松鼠的位置
        squirrel->x = position[0];
        squirrel->y = position[1];
    }
    squirrel->step_total = 0;
    squirrel->step_50 = 0;
    InitQueue(&squirrel->q);

    int shouldStop = shouldWorkerStop();
    while (!shouldStop) {

    }

}


//    EnQueue(&squirrel->q, 3);
//    QueueTraverse(squirrel->q);