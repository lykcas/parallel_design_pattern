//
// Created by YUKE LI on 2020/3/11.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mpi.h"
#include "pool.h"
#include "squirrel-functions.h"
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
    int i, flag = 0, switch_update = 0, fullflag = 0;
    MPI_Status status;
    MPI_Request req;
    land->id = id;
    land->populationInflux = 0;
    for (i = 0; i < 4; i++) {
        land->thisMonthPopulation[i] = 0;
        land->thisMonthInfect[i] = 0;
    }
    land->infectionLevel = 0;
    printf("I am a land, my id = %d\n", land->id);
//    int shouldStop = shouldWorkerStop();
    int workerStatus = 0;
    int months = 0;
    while (!workerStatus){
        int count = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag) { // means need to update!
            // Update
            MPI_Get_count(&status, MPI_INT, &count);
            printf("Get count %d.\n", count);
            if (count == 1) { // 收到来自clock的信息，更新populationInflux和infectionLevel的值
                MPI_Recv(&switch_update, 1, MPI_INT, LANDCELL+1, 0, MPI_COMM_WORLD, &status);
                months++;
                if (fullflag != 3) {
                    land->thisMonthPopulation[fullflag] = land->thisMonthPopulation[3];
                    land->thisMonthInfect[fullflag] = land->thisMonthInfect[3];
                    land->thisMonthPopulation[3] = 0;
                    land->thisMonthInfect[3] = 0;
                    fullflag++;
                    printf("Month %d update fullflag %d.\n", months, fullflag);
                } else {
                    for (i = 0; i < 3; i++) {
                        land->thisMonthPopulation[i] = land->thisMonthPopulation[i+1];
                        land->thisMonthInfect[i] = land->thisMonthInfect[i+1];
                    }
                    land->thisMonthPopulation[3] = 0;
                    land->thisMonthInfect[3] = 0;
                    printf("Month %d update fullflag %d.\n", months, fullflag);
                }
                for (i = 0; i < 3; i++) {
                    land->populationInflux += land->thisMonthPopulation[i];
                }
                for (i = 0; i < 2; i++) {
                    land->infectionLevel += land->thisMonthInfect[i];
                }
                printf("No. %d land receive update command %d.\n My population is %d, My infection is %d\n", land->id, switch_update, land->populationInflux, land->infectionLevel);

            } else if (count == 2) { // 收到来自松鼠的信息，发送回自己的populationInflux和infectionLevel，并++
                printf("Now is month %d~~~~~\n", months);
                int squirrelInfo[2]={0};
                int landInfo[2] = {land->populationInflux, land->infectionLevel};
//                int squirrel_id = 0, whetherItInfect = 0;
                MPI_Isend(landInfo, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &req);
                MPI_Recv(squirrelInfo, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);
                // 给松鼠发回去自己的populationInflux和infectionLevel Isend
                if (squirrelInfo[1] == 1) land->thisMonthInfect[3]++;
                land->thisMonthPopulation[3]++;
            } else {
                printf("I don't know what message is it ??? \n");
            }

        }
        workerStatus = shouldWorkerStop();
        if (workerStatus == 1) printf("Now landcell %d workerStatus is %d.\n", id, workerStatus);
    }
}

void clockWorkerCode() {
    int months = 12, i, switch_update = 1;
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
    squirrel->id = id;
    long seed = -1 - id;
    initialiseRNG(&seed);
    int parentId = getCommandData();
    MPI_Status status;
    MPI_Request req;
    float position[2] = {0};
    if (parentId == 0) {
        squirrel->x = 0.0;
        squirrel->y = 0.0;
    } else {
        MPI_Recv(position, 2, MPI_FLOAT, parentId, 1, MPI_COMM_WORLD, &status); // 接收父松鼠的位置
        squirrel->x = position[0];
        squirrel->y = position[1];
    }
    squirrel->step_total = 0;
    squirrel->step_50 = 0;
    squirrel->switch_infect = 0;
    squirrel->population50 = 0.0;
    InitQueue(&squirrel->q);
    printf("---Squirrel %d start to ready.\n", id);
//    printf("I am a squirrel %d and I am %d infect.\n", id, squirrel->whetherInfect);

//    int shouldStop = shouldWorkerStop();
    int workerStatus = 0, nosleep = 1;
    int asd = 0;
    while (!workerStatus && nosleep) {

//        printf("asdfghjkl--------%d--------.\n", asd);
//        asd++;
        float x = squirrel->x, y = squirrel->y;
        squirrelStep(x, y, &squirrel->x, &squirrel->y, &seed);
        position[0] = squirrel->x;
        position[1] = squirrel->y;
        int this_land_Pid = getCellFromPosition(squirrel->x, squirrel->y);
        squirrel->step_total += 1;
        squirrel->step_50 += 1;
        int info_squirrel[2] = {0};
        info_squirrel[0] = squirrel->id;
        info_squirrel[1] = squirrel->whetherInfect;
        MPI_Isend(info_squirrel, 2, MPI_INT, this_land_Pid, 0, MPI_COMM_WORLD, &req); //给landcell互通消息
        int info_landcell[2] = {0}; // 【0】是populationInflux，【1】是infectionLevel
        MPI_Recv(info_landcell, 2, MPI_INT, this_land_Pid, 0, MPI_COMM_WORLD, &status);
//        printf("-Squirrel %d Receive information from landcell %d.\n", id, this_land_Pid);
        squirrel->population50 += info_landcell[0]; //把populationInflux 加到松鼠info里
        EnQueue(&squirrel->q, info_landcell[1]);
//        EnQueue(&squirrel->q, info_landcell[])
//        printf("The new position of squirrel %d is %f, %f.\n", id, squirrel->x, squirrel->y);
        // 判断是否生孩子
//        if(squirrel->step_50 == 50) {
//            printf("---Squirrel %d start to be an adult.\n", id);
//            squirrel->population50 = squirrel->population50 / 50.0;
//            int giveBirth = 0;
//            giveBirth = willGiveBirth(squirrel->population50, &seed);
//            if (giveBirth == 1) {
//                // 生孩子
//                int childPid = startWorkerProcess();
//                int child_info[2] = {3, childPid};
//                MPI_Isend(child_info, 2, MPI_INT, childPid, 0, MPI_COMM_WORLD, &req);
//                MPI_Isend(position, 2, MPI_FLOAT, childPid, 1, MPI_COMM_WORLD, &req);
//                printf("---Squirrel %d start to give birth to squirrel %d.\n", id, childPid);
//            }
//            squirrel->step_50 = 0;
//            squirrel->population50 = 0;
//        }
//        // 判断是否染病
//        if (squirrel->step_total >=50 && squirrel->switch_infect == 0) {
//            float infect50 = QueueCount(squirrel->q) / 50.0;
//            int catchDisease = 0;
//            catchDisease = willCatchDisease(infect50, &seed);
//            if (catchDisease) {
//                squirrel->whetherInfect = 1;
//                squirrel->switch_infect = 1;
//                printf("---Squirrel %d start to catch disease.\n", id);
//            }
//        } else if (squirrel->switch_infect > 0) {
//            if (squirrel->switch_infect < 50) {
//                squirrel->switch_infect ++;
//            } else { // 已染病且至少走了50步了，判断是否死亡
//                int whetherDie = 0;
//                whetherDie = willDie(&seed);
//                if (whetherDie) { // 如果被判死刑
//                    nosleep = workerSleep();
//                    printf("Now squirrel &d is dead.\n", id);
//                }
//            }
//        }

//        workerStatus = shouldWorkerStop();
//        if (workerStatus == 1) printf("Now squirrel %d workerStatus is %d.\n", id, workerStatus);
    }
    printf("Squirrel %d out.\n", id);

}


//    EnQueue(&squirrel->q, 3);
//    QueueTraverse(squirrel->q);