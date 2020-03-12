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
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (rank == 0) printf("Problem size of %d elements, serial threshold of %d\n", data_length, serial_threshold);
	printf("I am here!\n");
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
		}
		/*
		 * The master will need to get back the overall sorted data from the worker it first created
		 * to then display it
		 */
		if (should_displayData) {
			printf("\nSorted\n");
			displayData(data, data_length);
		
		}
	}
	processPoolFinalise();
	if (rank == 0) printf("Parallel mergesort completed in %f seconds\n", MPI_Wtime()-startTime);
	MPI_Finalize();
	return 0;
}

static int startMergeSort(double * data, int length) {
	int workerPid = startWorkerProcess();
	/*
	 * This initialises the first worker - here the master will need to send the overall size of the data
	 * and the data itself to the worker that it has just created
	 */
	return workerPid;
}

static void workerCode(int serial_threshold) {
	int workerStatus = 1;
	int parentId, dataLength;
	double * data;
	while (workerStatus) {
		parentId = getCommandData();
		printf("parentID = %d \n", parentId);
		/*
		 * Here you will need to receive the data and amount of data to sort from the parent. You
		 * will need to allocate the data variable to hold this.
		 */
		sort(data, dataLength, serial_threshold);
		if (parentId==0) shutdownPool();
		/*
		 * Here you will need to send the sorted data back to the parent
		 */
		workerStatus=workerSleep();
	}
}

static void sort(double * data, int length, int serial_threshold) {
	if (length < serial_threshold) {
		qsort(data, length, sizeof(double), qsort_compare_function);
	} else {
		int workerPid = startWorkerProcess();
		/*
		 * Need to figure out the pivot, split the data, send half to the newly created worker.
		 * Then this process should call sort with the other half of the data. Once that has completed
		 * need to get the sorted data back from the worker and merge the two together (using merge function)
		 */
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
