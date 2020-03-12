module mergesort_mod
	use pool
	use qsort_c_module
	use mpi
	use ran2_mod
	implicit none  

	real(kind=8) :: start_task_time, calc_time, comm_time

contains

	subroutine workerCode(serial_threshold)
		integer, intent(in) :: serial_threshold

		integer :: workerStatus, parentId, dataLength, ierr
		real(kind=8), dimension(:), allocatable :: data

		workerStatus=1
		do while (workerStatus == 1)
			parentId = getCommandData()
			! Here you will need to receive the data and amount of data to sort from the parent. You will need to allocate the data variable to hold this.
       			call sort(data, dataLength, serial_threshold)
			if (parentId == 0) call shutdownPool()
			! Here you will need to send the sorted data back to the parent
			call workerSleep(workerStatus)
		end do
	end subroutine workerCode

	recursive subroutine sort(data, dataLength, serial_threshold)
		integer, intent(in) :: dataLength, serial_threshold
		real(kind=8), dimension(:), intent(inout) :: data

		integer :: pivot, workerPid, ierr

		if (dataLength < serial_threshold) then			
			call QsortC(data)
		else						
			workerPid=startWorkerProcess()
			! Need to figure out the pivot, split the data, send half to the newly created worker.
			! Then this process should call sort with the other half of the data. Once that has completed
			! need to get the sorted data back from the worker and merge the two together (using merge function)
		end if
	end subroutine sort

	subroutine merge_data(data, pivot, length)
		real(kind=8), dimension(:), intent(inout) :: data
		integer, intent(in) :: pivot, length

		real(kind=8), dimension(length) :: tempData
		integer i, pre_index, post_index

		pre_index=1
		post_index=pivot+1
		do i=1,length
			if (pre_index > pivot) then
				tempData(i)=data(post_index)
				post_index=post_index+1
			else if (post_index > length) then
				tempData(i)=data(pre_index)
				pre_index=pre_index+1
			else if (data(pre_index) .lt. data(post_index)) then
				tempData(i)=data(pre_index)
				pre_index=pre_index+1
			else
				tempData(i)=data(post_index)
				post_index=post_index+1
			end if
		end do
		data=tempData
	end subroutine merge_data
end module mergesort_mod

program mergesort_prog
	use mergesort_mod
	use pool
	use mpi
	implicit none

	call entryPoint()
contains

	subroutine entryPoint()
		integer :: ierr, process_pool_status, data_length, serial_threshold, int_display_data, i, rank, size, nonzero(3), globalnonzero(3)
		character(len=32) :: arg
		real(kind=8) :: start_time

		start_task_time=0.0
 		calc_time=0.0 
		comm_time=0.0

		call MPI_Init(ierr)
		call processPoolInit(process_pool_status)
		call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
		call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
		start_time=MPI_Wtime()

		if (command_argument_count() .lt. 3) then
			print *, &
				"You must provide three command line arguments, the number of elements to be sorted, ", &
				"the serial threshold and whether to display raw and sorted data"
			return
		end if
		call get_command_argument(1, arg)
		read(arg,*) data_length
		call get_command_argument(2, arg)
		read(arg,*) serial_threshold
		call get_command_argument(3, arg)
		read(arg,*) int_display_data

		if (rank == 0) then
			print *, "Problem size of ",  data_length, " elements, serial threshold of ", serial_threshold
		end if

		if (process_pool_status == 1) then
			call workerCode(serial_threshold)
		else if (process_pool_status == 2) then
			call mergesort_master(data_length, int_display_data==1)		
		end if

		call processPoolFinalise() 		

		if (rank == 0) then
			print *, "Parallel mergesort completed in ", MPI_Wtime()-start_time, " seconds"			
		end if
		call MPI_Finalize(ierr)
	end subroutine entryPoint

	subroutine mergesort_master(data_length, display_data)
		integer, intent(in) :: data_length
		logical, intent(in) :: display_data
		real(kind=8), dimension(:), allocatable :: data

		integer :: pid, ierr, len, masterStatus

		allocate(data(data_length))
		call generateUnsortedData(data, data_length)
		if (display_data) then
			print *, "Unsorted data"
			call displayData(data, data_length)
		end if
		pid=startMergeSort(data, data_length)
		call masterPoll(masterStatus)
		do while (masterStatus == 1) 
			call masterPoll(masterStatus)
		end do
		! The master will need to get back the overall sorted data from the worker it first created to then display it
		if (display_data) then
			print *, "Sorted data"
			call displayData(data, data_length)
		end if
		deallocate(data)
	end subroutine mergesort_master

	integer function startMergeSort(data, data_length)
		integer, intent(in) :: data_length
		real(kind=8), dimension(data_length), intent(in) :: data

		integer :: pid, ierr

		pid=startWorkerProcess()
		! This initialises the first worker - here the master will need to send the overall size of the data
		! and the data itself to the worker that it has just created
		startMergeSort=pid
	end function startMergeSort  

	subroutine displayData(data, data_length)
		integer, intent(in) :: data_length
		real(kind=8), dimension(data_length), intent(in) :: data

		integer :: i
		do i=1, data_length
			write (6, "(f10.3)", advance='no') data(i)
		end do
		print *, ""
	end subroutine displayData

	subroutine generateUnsortedData(data, data_length)
		integer, intent(in) :: data_length
		real(kind=8), dimension(data_length), intent(out) :: data

		integer :: seed, i, clocktick

		call system_clock(clocktick)
		seed=-1-clocktick
		data(1)=ran2(seed)
		do i=2, data_length
			data(i)=ran2(seed)
		end do    
	end subroutine generateUnsortedData
end program mergesort_prog

