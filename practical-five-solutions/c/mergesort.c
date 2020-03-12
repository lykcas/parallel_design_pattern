// Sample mergesort solutions using D&Q with a process pool
#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static void workerCode(int);
static int startMergeSort(double*, int);
static void sort(double*, int, int);
static void merge(double*, int, int);
static int qsort_compare_function(const void*, const void*);
static double* generateUnsortedData(int);
static void displayData(double *, int);

double start_task_time, calc_time, comm_time;

int main(int argc, char * argv[]) {
	MPI_Init(&argc, &argv);
	start_task_time=0.0;
	calc_time=0.0;
	comm_time=0.0;
	double startTime=MPI_Wtime();

	if (argc != 4) {
		fprintf(stderr, "You must provide three command line arguments, the number of elements to be sorted, the serial threshold and whether to display raw and sorted data\n");
		return -1;
	}

	int data_length=atoi(argv[1]);
	int serial_threshold=atoi(argv[2]);
	int should_displayData=atoi(argv[3]);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) printf("Problem size of %d elements, serial threshold of %d\n", data_length, serial_threshold);
	// printf("I am here !\n");
	int statusCode = processPoolInit();
	printf("Now I am %d !\n", statusCode);
	if (statusCode == 1) {
		// A worker so do the worker tasks
		workerCode(serial_threshold);
	} else if (statusCode == 2) {
		double * data = generateUnsortedData(data_length);
		if (should_displayData) {
			printf("Unsorted\n");
			displayData(data, data_length);
		}
		int pid=startMergeSort(data, data_length);
		int masterStatus = masterPoll();
		while (masterStatus) {
			masterStatus=masterPoll();
			printf("Now masterStatus is %d \n", masterStatus);
		}
		MPI_Recv(data, data_length, MPI_DOUBLE, pid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if (should_displayData) {
			printf("\nSorted\n");
			displayData(data, data_length);
		}
	}
	processPoolFinalise();	
	double local_timings[3], local_minned_timings[3], min_timings[3], max_timings[3], sum_timings[3];
	local_timings[0]=start_task_time;
	local_timings[1]=calc_time;
	local_timings[2]=comm_time;
	local_minned_timings[0]=start_task_time == 0.0 ? 99999 : start_task_time;
	local_minned_timings[1]=calc_time == 0.0 ? 99999 : calc_time;
	local_minned_timings[2]=comm_time == 0.0 ? 99999 : comm_time;
	MPI_Reduce(local_minned_timings, min_timings, 3, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(local_timings, max_timings, 3, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(local_timings, sum_timings, 3, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	int non_zero[3], global_nonzero[3], i;
	for (i=0;i<3;i++) non_zero[i]=local_timings[i] > 0.0 ? 1 : 0;
	MPI_Reduce(non_zero, global_nonzero, 3, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	
	if (rank == 0) {
		printf("Parallel mergesort completed in %f seconds\n", MPI_Wtime()-startTime);
		printf("Task start timings: %f (s) avg, %f (s) min, %f (s) max\n", sum_timings[0]/global_nonzero[0], min_timings[0], max_timings[0]);
		printf("Computation timings : %f (s) avg, %f (s) min, %f (s) max\n", sum_timings[1]/global_nonzero[1], min_timings[1], max_timings[1]);
		printf("Communication timings : %f (s) avg, %f (s) min, %f (s) max\n", sum_timings[2]/global_nonzero[2], min_timings[2], max_timings[2]);
	}
	MPI_Finalize();
	return 0;
}

static int startMergeSort(double * data, int length) {
	int workerPid = startWorkerProcess();
	MPI_Send(&length, 1, MPI_INT, workerPid, 0, MPI_COMM_WORLD);
	MPI_Send(data, length, MPI_DOUBLE, workerPid, 0, MPI_COMM_WORLD);
	return workerPid;
}

static void workerCode(int serial_threshold) {
	int workerStatus = 1;
	int parentId, dataLength;
	double * data;
	while (workerStatus) {
		parentId = getCommandData();
		double t=MPI_Wtime();
		MPI_Recv(&dataLength, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		data=(double*) malloc(sizeof(double) * dataLength);
		MPI_Recv(data, dataLength, MPI_DOUBLE, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		comm_time+=MPI_Wtime()-t;
		sort(data, dataLength, serial_threshold);
		if (parentId==0) shutdownPool();
		t=MPI_Wtime();
		MPI_Send(data, dataLength, MPI_DOUBLE, parentId, 0, MPI_COMM_WORLD);
		comm_time+=MPI_Wtime()-t;
		free(data);
		workerStatus=workerSleep();
	}
}

static void sort(double * data, int length, int serial_threshold) {
	if (length < serial_threshold) {
		double t=MPI_Wtime();
		qsort(data, length, sizeof(double), qsort_compare_function);
		calc_time+=MPI_Wtime()-t;
	} else {
		int pivot=length/2;
		double t=MPI_Wtime();
		int workerPid = startWorkerProcess();
		start_task_time+=MPI_Wtime()-t;
		t=MPI_Wtime();
		MPI_Send(&pivot, 1, MPI_INT, workerPid, 0, MPI_COMM_WORLD);
		MPI_Send(data, pivot, MPI_DOUBLE, workerPid, 0, MPI_COMM_WORLD);
		comm_time+=MPI_Wtime()-t;
		sort(&data[pivot], length-pivot, serial_threshold);
		t=MPI_Wtime();
		MPI_Recv(data, pivot, MPI_DOUBLE, workerPid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		comm_time+=MPI_Wtime()-t;
		t=MPI_Wtime();
		merge(data, pivot, length);
		calc_time+=MPI_Wtime()-t;
	}
}

static int qsort_compare_function(const void * a, const void * b) {
	double d1=*((double*) a);
	double d2=*((double*) b);
	if (d1 < d2) return -1;
	if (d1 > d2) return 1;
	return 0;
}

static void merge(double * data, int pivot, int length) {
	double new_data[length];
	int i, pre_index=0, post_index=pivot;
	for (i=0;i<length;i++) {
		if (pre_index >= pivot) {
			new_data[i]=data[post_index++];
		} else if (post_index >= length) {
			new_data[i]=data[pre_index++];
		} else if (data[pre_index] < data[post_index]) {
			new_data[i]=data[pre_index++];
		} else {
			new_data[i]=data[post_index++];
		}
	}
	memcpy(data, new_data, sizeof(double) * length);
}

static double* generateUnsortedData(int data_length) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long seed = -1-tv.tv_usec;
	ran2(&seed);
	int i;
	double * data = (double*) malloc(sizeof(double) * data_length);
	for (i=0;i<data_length;i++) {
		data[i]=(double) ran2(&seed);
	}
	return data;
}

static void displayData(double * data, int length) {
	int i;
	for (i=0;i<length;i++) {
		printf("%.3f ", data[i]);
	}
	printf("\n");
}
