! Sample mergesort solutions using D&Q with a process pool
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
		real(kind=8) :: t

		workerStatus=1
		do while (workerStatus == 1)
			parentId = getCommandData()
			t=MPI_Wtime()
			call MPI_Recv(dataLength, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
			allocate(data(dataLength))
			call MPI_Recv(data, dataLength, MPI_DOUBLE, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
			comm_time=comm_time+(MPI_Wtime()-t)
       			call sort(data, dataLength, serial_threshold)
			if (parentId == 0) then
				call shutdownPool()
			end if
			t=MPI_Wtime()
			call MPI_Send(data, dataLength, MPI_DOUBLE, parentId, 0, MPI_COMM_WORLD, ierr)
			comm_time=comm_time+(MPI_Wtime()-t)
			deallocate(data)
			call workerSleep(workerStatus)
		end do
	end subroutine workerCode

	recursive subroutine sort(data, dataLength, serial_threshold)
		integer, intent(in) :: dataLength, serial_threshold
		real(kind=8), dimension(:), intent(inout) :: data
		real(kind=8) :: t

		integer :: pivot, workerPid, ierr

		if (dataLength < serial_threshold) then
			t=MPI_Wtime()
			call QsortC(data)
			calc_time=calc_time+(MPI_Wtime()-t)
		else
			pivot=dataLength/2
			t=MPI_Wtime()
			workerPid=startWorkerProcess()
			start_task_time=start_task_time+(MPI_Wtime()-t)
			t=MPI_Wtime()
			call MPI_Send(pivot, 1, MPI_INT, workerPid, 0, MPI_COMM_WORLD, ierr)
			call MPI_Send(data(:pivot), pivot, MPI_DOUBLE, workerPid, 0, MPI_COMM_WORLD, ierr)
			comm_time=comm_time+(MPI_Wtime()-t)
			call sort(data(pivot+1:), dataLength-pivot, serial_threshold)
			t=MPI_Wtime()
			call MPI_Recv(data, pivot, MPI_DOUBLE, workerPid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
			comm_time=comm_time+(MPI_Wtime()-t)
			t=MPI_Wtime()
			call merge_data(data, pivot, dataLength)
			calc_time=calc_time+(MPI_Wtime()-t)
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
		real(kind=8) :: start_time, local_timings(3), local_minned_timings(3), min_timings(3), max_timings(3), sum_timings(3)

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

		local_timings(1)=start_task_time
		local_timings(2)=calc_time
		local_timings(3)=comm_time
		local_minned_timings(1)=merge(99999.9_8, start_task_time, start_task_time == 0.0)
		local_minned_timings(2)=merge (99999.9_8, calc_time, calc_time == 0.0)
		local_minned_timings(3)=merge(99999.9_8, comm_time, comm_time == 0.0)
		do i=1,3
			nonzero(i)=merge(1, 0, local_timings(i) .gt. 0.0)
		end do

		call MPI_Reduce(local_minned_timings, min_timings, 3, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD, ierr)
		call MPI_Reduce(local_timings, max_timings, 3, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD, ierr)
		call MPI_Reduce(local_timings, sum_timings, 3, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD, ierr)
		call MPI_Reduce(nonzero, globalnonzero, 3, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD, ierr)

		if (rank == 0) then
			print *, "Parallel mergesort completed in ", MPI_Wtime()-start_time, " seconds"
			print *, "Task start timings: ", sum_timings(1)/globalnonzero(1), "(s) avg, ", min_timings(1), &
				"(s) min, ", max_timings(1), "(s) max"
			print *, "Computation timings: ", sum_timings(2)/globalnonzero(2), "(s) avg, ", min_timings(2), &
				"(s) min, ", max_timings(2), "(s) max"
			print *, "Communication timings: ", sum_timings(3)/globalnonzero(3), "(s) avg, ", min_timings(3), &
				"(s) min, ", max_timings(3), "(s) max"
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
		call MPI_Recv(data, data_length, MPI_DOUBLE, pid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
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
		call MPI_Send(data_length, 1, MPI_INT, pid, 0, MPI_COMM_WORLD, ierr)
		call MPI_Send(data, data_length, MPI_DOUBLE, pid, 0, MPI_COMM_WORLD, ierr)
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

