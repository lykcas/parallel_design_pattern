! Example code to run and test the process pool. To compile use something like mpif90 -o test pool.f90 test.f90
program test
    use pool    ! Use the process pool module
    implicit none
    include "mpif.h"

    integer :: ierr, returnCode, requestReturnCode, i, workerPid, initialWorkerRequests(10), activeWorkers=0

    ! Call MPI initialize first
    call MPI_Init(ierr)
    ! Initialise the process pool
    ! The return code is = 1 for worker to do some work, 0 for do nothing and stop and 2 for this is the master so call master poll
    ! For workers this subroutine will block until the master has woken it up to do some work
    call processPoolInit(returnCode)
    if (returnCode == 1) then
        ! The worker
        call workerCode()
    else if (returnCode == 2) then
        ! This is the master, each call to master poll will block until a message is received and then will handle it and return
        ! 1 to continue polling and running the pool and 0 to quit.
        ! Basically it just starts 10 workers and then registers when each one has completed. When they have all completed it
        ! shuts the entire pool down
        initialWorkerRequests(:) = MPI_REQUEST_NULL
        do i=1,10
            workerPid = startWorkerProcess()
            call MPI_Irecv(0, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, initialWorkerRequests(i), ierr)
            activeWorkers=activeWorkers+1;
            write(*,"(A,I0,A,I0)") "Master started worker ",i," on MPI process ",workerPid
        end do
        call masterPoll(returnCode)

        do while (returnCode == 1)
            call masterPoll(returnCode)
            do i=1,10
                ! Checks all outstanding workers that master spawned to see if they have completed
                if (initialWorkerRequests(i) .ne. MPI_REQUEST_NULL) then
                    call MPI_Test(initialWorkerRequests(i), requestReturnCode, MPI_STATUS_IGNORE, ierr)
                    if (requestReturnCode .ne. 0) activeWorkers = activeWorkers -1
                end if
            end do
            ! If we have no more active workers then quit poll loop which will effectively shut the pool down when  processPoolFinalise is called
            if (activeWorkers == 0) exit
        end do
    end if
    ! Finalizes the process pool, call this before closing down MPI
    call processPoolFinalise()
    ! Finalize MPI, ensure you have closed the process pool first
    call MPI_Finalize(ierr)

    contains

    subroutine workerCode()
        integer :: workerStatus = 1;
        integer :: myRank, parentId, childOnePid, childTwoPid, childRequests(2)
        childRequests(:) = 0
        do while (workerStatus == 1)
            call MPI_Comm_rank(MPI_COMM_WORLD, myRank, ierr)
            parentId = getCommandData() ! We encode the parent ID in the wake up command data
            if (parentId == 0) then
                ! If my parent is the master (one of the 10 that the master started) then spawn two further children
                childOnePid = startWorkerProcess()
                childTwoPid = startWorkerProcess()
                write(*,"(A,I0,A,I0,A,I0,A)") "Worker on process ", myRank, " started two child processes (", &
                    childOnePid, " and ", childTwoPid,")"
                ! Wait for these children to send me a message
                call MPI_Irecv(0, 0, MPI_INT, childOnePid, 0, MPI_COMM_WORLD, childRequests(1), ierr)
                call MPI_Irecv(0, 0, MPI_INT, childTwoPid, 0, MPI_COMM_WORLD, childRequests(2), ierr)
                call MPI_Waitall(2, childRequests, MPI_STATUS_IGNORE, ierr)
                ! Now tell the master that I am finished
                call MPI_Send(0, 0, MPI_INT, 0, 0, MPI_COMM_WORLD, ierr)
            else
                write(*,"(A,I0,A,I0)") "Child worker on process ", myRank," tarted with parent ", parentId
                ! Tell my parent that I have been alive and am about to die
                call MPI_Send(0, 0, MPI_INT, parentId, 0, MPI_COMM_WORLD, ierr)
            end if

            ! Worker sleep will sleep until it is woken by a wake (status=1) or stop (status=0)
            call workerSleep(workerStatus)
        end do
    end subroutine
end program test
