//
// Created by YUKE LI on 2020/3/11.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
//#include <dos.h>
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
    // printf("I am %d landcell\n", id);
    // Init a struct of landcell
    struct landcell *land;
    land = (struct landcell *) malloc(sizeof(struct landcell));
    // Do as landcell's do
    // landcellWorkerCode(land, id);
    int i, switch_update = 0, fullflag = 0, myRank;
    // int printflag = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    land->id = id;
    land->populationInflux = 0;
    for (i = 0; i < 4; i++) {
        land->thisMonthPopulation[i] = 0;
        land->thisMonthInfect[i] = 0;
    }
    land->infectionLevel = 0;
    // printf("I am a land, my id = %d, my rank is %d\n", land->id, myRank);
    int workerStatus = 0;
    int months = 0;
    while (!workerStatus) {
        int count = 0, flag = 0;
        MPI_Status status;
        MPI_Request req;
//        MPI_Request req;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag) { // means need to update!
            // Update
            count = 0;
            MPI_Get_count(&status, MPI_INT, &count);
//            printf("Get count %d.\n", count);
            if (count == 1) { // 收到来自clocsk的信息，更新populationInflux和infectionLevel的值
                MPI_Recv(&switch_update, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                months++;
                // printflag = 0;
//                printf("Land %d Receive update command from %d.\n", myRank, status.MPI_SOURCE);
                land->populationInflux = 0;
                land->infectionLevel = 0;
                if (fullflag != 3) {
                    land->thisMonthPopulation[fullflag] = land->thisMonthPopulation[3];
                    land->thisMonthInfect[fullflag] = land->thisMonthInfect[3];
                    land->thisMonthPopulation[3] = 0;
                    land->thisMonthInfect[3] = 0;
//                    printf("Hey in month %d land %d fullflag %d.\n", months, land->id, land->thisMonthPopulation[fullflag]);
                    fullflag++;
//                    printf("Month %d update fullflag %d.\n", months, fullflag);
                } else {
                    for (i = 0; i < 3; i++) {
                        land->thisMonthPopulation[i] = land->thisMonthPopulation[i+1];
                        land->thisMonthInfect[i] = land->thisMonthInfect[i+1];
//                        printf("Heyyy in month %d land %d fullflag %d.\n", months, land->id, land->thisMonthPopulation[i]);
                    }
                    land->thisMonthPopulation[3] = 0;
                    land->thisMonthInfect[3] = 0;
//                    printf("Month %d update fullflag %d.\n", months, fullflag);
                }
                for (i = 0; i < 3; i++) {
                    land->populationInflux += land->thisMonthPopulation[i];
                }
                for (i = 0; i < 2; i++) {
                    land->infectionLevel += land->thisMonthInfect[i];
                }
                printf("No. %d land. My population is %d, My infection is %d\n", land->id, land->populationInflux, land->infectionLevel);

            } else if (count == 2) { // 收到来自松鼠的信息，发送回自己的populationInflux和infectionLevel，并++
                // if (!printflag) {
                //     // printf("Now is month %d~~~%d~~\n\n", months, myRank);
                //     printflag = 1;
                // }

                int squirrelInfo[2] = {0};
                int landInfo[2] = {land->populationInflux, land->infectionLevel};
//                int squirrel_id = 0, whetherItInfect = 0;
//                MPI_Isend(landInfo, 2, MPI_INT, statuss.MPI_SOURCE, 0, MPI_COMM_WORLD, &req);
                MPI_Recv(squirrelInfo, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//                MPI_Send(landInfo, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                MPI_Isend(landInfo, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &req);
                // 给松鼠发回去自己的populationInflux和infectionLevel Isend
                if (squirrelInfo[1] == 1) land->thisMonthInfect[3]++;
                land->thisMonthPopulation[3]++;
//                printf("land %d: %d, %d.\n", land->id, land->thisMonthInfect[3], land->thisMonthPopulation[3]);
            }


        }
        workerStatus = shouldWorkerStop();
        // if (workerStatus == 1) printf("Now landcell %d workerStatus is %d.\n", id, workerStatus);
//        MPI_Request_free(&req);
    }
}

void workerClockCode(int id) {
    // Do as clock's do
//    clockWorkerCode();
    int months = MONTH, i, switch_update = 1, myRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    int landcount = LANDCELL;
    MPI_Request req;
    while (months) {
        sleep(1);
        printf("------------This is month %d----------\n", MONTH-months+1);
        for (i = 0; i < landcount; i++) {
            MPI_Isend(&switch_update, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, &req);
//            printf("Now clock send update message to landcell %d.\n", i + 1);
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
    //    squirrelWorkerCode(squirrel, id);
    // 初始化
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
    //        3 8
        if ((LANDCELL + 3) < myRank && myRank < (LANDCELL + 8)) {
            squirrel->whetherInfect = 1;
        }
        // printf("I am %d squirrel, am I healthy? %d\n", id, squirrel->whetherInfect);
    } else {
        MPI_Recv(position, 2, MPI_FLOAT, parentId, 1, MPI_COMM_WORLD, &status); // 接收父松鼠的位置
        squirrel->x = position[0];
        squirrel->y = position[1];
        squirrel->whetherInfect = 0;
        // printf("Hi! I am Squirrel %d, my parent is %d.\n", myRank, parentId);
    }
    squirrel->step_total = 0;
    squirrel->step_50 = 0;
    squirrel->switch_infect = 0;
    squirrel->population50 = 0.0;
    InitQueue(&squirrel->q);
    //    printf("---Squirrel %d, %d start to ready.\n", myRank, id);
    //    printf("I am a squirrel %d and I am %d infect.\n", id, squirrel->whetherInfect);

    //    int shouldStop = shouldWorkerStop();
    int workerStatus = 0, nosleep = 1;
    int asd = 0;
    while (!workerStatus && nosleep) {
        MPI_Request req;
    //    printf("asdfghjkl--------%d--------.\n", asd);
    //         asd++;
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
    //        printf("hereherehere.\n");
        MPI_Isend(info_squirrel, 2, MPI_INT, this_land_Pid, 0, MPI_COMM_WORLD, &req); //给landcell互通消息

    //        MPI_Wait(&reqq, MPI_STATUS_IGNORE);
        int info_landcell[2] = {0}; // 【0】是populationInflux，【1】是infectionLevel
        workerStatus = shouldWorkerStop();
        if (workerStatus == 1) {
            printf("This is squirrel %d. It jumps %d.\n", id, squirrel->step_total);
            break;
        }
    //        printf("Squirrel %d is waiting.\n", myRank);
        MPI_Recv(info_landcell, 2, MPI_INT, this_land_Pid, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //        printf("-Squirrel %d, %d Receive information from landcell %d.\n", myRank, squirrel->id, this_land_Pid);
        squirrel->population50 += info_landcell[0]; //把populationInflux 加到松鼠info里
        EnQueue(&squirrel->q, info_landcell[1]);
        // 判断是否生孩子
        if(squirrel->step_50 == 50) {
    //            printf("---Squirrel %d start to be an adult.\n", id);
            squirrel->population50 = squirrel->population50 / 50.0;
            int giveBirth = 0;
            giveBirth = willGiveBirth(squirrel->population50, &seed);
            if (giveBirth == 1) {
                // 生孩子
                int childPid = startWorkerProcess();
                int child_info[2] = {3, childPid};
                // printf("Squirrel %d have a baby %d.\n", myRank, childPid);
                MPI_Isend(child_info, 2, MPI_INT, childPid, 0, MPI_COMM_WORLD, &req);
                MPI_Isend(position, 2, MPI_FLOAT, childPid, 1, MPI_COMM_WORLD, &req);
            }
            squirrel->step_50 = 0;
            squirrel->population50 = 0;
        }
       // 判断是否染病
        if (squirrel->step_total >=50 && squirrel->switch_infect == 0) {
    //            printf("进来了.\n");
            float infect50 = QueueCount(squirrel->q) / 50.0;
            int catchDisease = 0;
            catchDisease = willCatchDisease(infect50, &seed);
            if (catchDisease) {
                squirrel->whetherInfect = 1;
                squirrel->switch_infect = 1;
                // printf("! ! ! ! ! ! Squirrel %d start to catch disease.\n", id);
            }
        }
        else if (squirrel->whetherInfect == 1) {
            if (squirrel->switch_infect < 50) {
                squirrel->switch_infect ++;
    //                printf("I've live with %d steps, hahaha.\n", squirrel->switch_infect);
            } else { // 已染病且至少走了50步了，判断是否死亡
                int whetherDie = 0;
                whetherDie = willDie(&seed);
    //                printf("I am squirrel %d, Will I die?????????? Answer: %d\n", myRank, whetherDie);
                if (whetherDie) { // 如果被判死刑
                    // printf("Now squirrel %d is dead.\n", myRank);
                    nosleep = workerSleep();
                }
            }
        }
        struct timespec ts, ts1;
        ts.tv_nsec = 100000000; // 0.1s
    //        ts.tv_nsec = 10000000; // 0.01s
        ts.tv_sec = 0;
        nanosleep(&ts, &ts1);


    //        MPI_Request_free(&req);
    }
    // printf("Squirrel %d out.\n", id);
}

void workerCode() {
    int whoamI[2] = {0}; // 1: landcell; 2: clock; 3:squirrel
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
    
    // printf("Init landcell success!\n Init clock success!\n Init squirrel success!\n");
    // MPI_Request_free(landrequest);
    // MPI_Request_free(squirrelrequest);
    // MPI_Request_free(&clockrequest);

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

//void workers(void (*fn_pointer)(int), int id) {
//    (*fn_pointer)(id);
//}

//void landcellWorkerCode(struct landcell *land, int id) {}

//void squirrelWorkerCode(struct squirrels *squirrel, int id) {}

//void clockWorkerCode() {}

//void workerCode() {
//    int whoamI[2] = {0}; // 1: landcell; 2: clock; 3:squirrel
//    MPI_Recv(whoamI, 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//    if (whoamI[0] == 1) {
////        workerLandCode(whoamI[1]);
//        workers(workerLandCode, whoamI[1]);
//    } else if (whoamI[0] == 2) {
////        workerClockCode(whoamI[1]);
//        workers(workerClockCode, whoamI[1]);
//    } else if (whoamI[0] == 3) {
////        workerSquirrelCode(whoamI[1]);
//        workers(workerSquirrelCode, whoamI[1]);
//    }
//}

//    EnQueue(&squirrel->q, 3);
//    QueueTraverse(squirrel->q);