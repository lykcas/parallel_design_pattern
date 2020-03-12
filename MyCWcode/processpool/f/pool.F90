module pool
    implicit none
    private
    include "mpif.h"

    ! An example data package which combines the command with some optional data, an example and can be extended
    type PP_Control_Package
        sequence
        integer command
        integer data
    end type PP_Control_Package

    ! Pool master - worker communication commands
    integer, parameter :: PP_STOP=0, PP_SLEEPING=1, PP_WAKE=2, PP_STARTPROCESS=3, PP_RUNCOMPLETE=4

    ! MPI P2P tag to use for command communications, it is important not to reuse this
    integer, parameter :: PP_CONTROL_TAG = 16384, PP_PID_TAG = 16383
    ! Pool options
    logical, parameter :: PP_QuitOnNoProcs = .true., PP_IgnoreOnNoProcs = .false., PP_DEBUG = .false.

    ! Example command package data type which can be extended
    integer :: PP_COMMAND_TYPE

    ! Internal pool global state
    integer :: PP_myRank, PP_numProcs, PP_pollRecvCommandRequest = MPI_REQUEST_NULL, &
        PP_processesAwaitingStart, ierr
    logical, dimension(:), allocatable :: PP_active
    type(PP_Control_Package) :: in_command

    ! Public subroutines available to those that use this module
    public :: processPoolInit, processPoolFinalise, masterPoll, startWorkerProcess, shutdownPool,&
                workerSleep, getCommandData, shouldWorkerStop

    contains

    ! Initialises the processes pool. Note that a worker will not return from this until it has been instructed to do some work
    ! or quit. The return code zero indicates quit, one indicates loop and work for the worker and two indicates that this is the
    ! master and it should loop and call master pool.
    subroutine processPoolInit(returnCode)
        integer, intent(out) :: returnCode

        call initialiseType()
        call MPI_Comm_rank(MPI_COMM_WORLD, PP_myRank, ierr)
        call MPI_Comm_size(MPI_COMM_WORLD, PP_numProcs, ierr)

        if (PP_myRank == 0) then
            if(PP_numProcs .lt. 2) then
                call errorMessage("No worker processes available for pool, run with more than one MPI process");
            end if
            allocate(PP_active(PP_numProcs))
            PP_active(:) = .false.
            PP_processesAwaitingStart=0
            if (PP_DEBUG) write(6,"(A,I0)") "[Master] Initialised Master ", PP_myRank
            returnCode = 2
        else
            call MPI_Recv(in_command, 1, PP_COMMAND_TYPE, 0, PP_CONTROL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
            call handleRecievedCommand(returnCode)
        end if
    end subroutine processPoolInit

    ! Each process calls this to finalise the process pool
    subroutine processPoolFinalise()
        integer :: i
        type(PP_Control_Package) :: out_command
        if (PP_myRank == 0) then
            deallocate(PP_active)
            call createCommandPackage(PP_STOP, out_command)
            do i=1,PP_numProcs - 1
                if (PP_DEBUG) write(6,"(A,I0)") "[Master] Shutting down process ", i
                call MPI_Send(out_command, 1, PP_COMMAND_TYPE, i, PP_CONTROL_TAG, MPI_COMM_WORLD, ierr)
            end do
        end if
        call MPI_Barrier(MPI_COMM_WORLD, ierr)
        call MPI_Type_Free(PP_COMMAND_TYPE, ierr)
    end subroutine processPoolFinalise

    ! Called by the master in a loop, will wait for commands, instruct starts and returns whether to continue polling or
    ! zero (false) if the program is to end
    subroutine masterPoll(returnCode)
        integer, intent(out) :: returnCode
        integer :: status(MPI_STATUS_SIZE), returnedRank

        if (PP_myRank == 0) then
            call MPI_Recv(in_command, 1, PP_COMMAND_TYPE, MPI_ANY_SOURCE, PP_CONTROL_TAG, MPI_COMM_WORLD, status, ierr)
            if(in_command%command==PP_SLEEPING) then
                if (PP_DEBUG) write(6,"(A,I0)") "[Master] Received sleep command from ", status(MPI_SOURCE)
                PP_active(status(MPI_SOURCE))=.false.
            end if
            if(in_command%command==PP_RUNCOMPLETE) then
                if (PP_DEBUG) write(6,"(A)") "[Master] Received shutdown command"
                returnCode=0
                return
            end if
            if(in_command%command==PP_STARTPROCESS) then
                PP_processesAwaitingStart = PP_processesAwaitingStart+1
            end if

            call startAwaitingProcessesIfNeeded(PP_processesAwaitingStart, status(MPI_SOURCE), returnedRank)

            if(in_command%command==PP_STARTPROCESS) then
                ! If the master was to start a worker then send back the process rank that this worker is now on
                call MPI_Send(returnedRank, 1, MPI_INT, status(MPI_SOURCE), PP_PID_TAG, MPI_COMM_WORLD, ierr);
            end if
            returnCode=1
        else
            call errorMessage("Worker process called master poll")
            returnCode=0
        end if
    end subroutine masterPoll

    ! A worker or the master can instruct to start another worker process
    integer function startWorkerProcess()
        type(PP_Control_Package) :: out_command

        if (PP_myRank == 0) then
            PP_processesAwaitingStart = PP_processesAwaitingStart+1
            call startAwaitingProcessesIfNeeded(PP_processesAwaitingStart,0,startWorkerProcess)
        else
            call createCommandPackage(PP_STARTPROCESS, out_command)
            call MPI_Send(out_command, 1, PP_COMMAND_TYPE, 0, PP_CONTROL_TAG, MPI_COMM_WORLD, ierr)
            call MPI_Recv(startWorkerProcess, 1, MPI_INT, 0, PP_PID_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
        end if
    end function startWorkerProcess

    ! A worker can instruct the master to shutdown the process pool. The master can also call this
    ! but it does nothing (they just need to call the finalisation step)
    subroutine shutdownPool
        type(PP_Control_Package) :: out_command

        if (PP_myRank .ne. 0) then
            if (PP_DEBUG) write(6,"(A)") "[Worker] Commanding a pool shutdown"
            call createCommandPackage(PP_RUNCOMPLETE, out_command)
            call MPI_Send(out_command, 1, PP_COMMAND_TYPE, 0, PP_CONTROL_TAG, MPI_COMM_WORLD, ierr)
        end if
    end subroutine shutdownPool

    ! Called by the worker at the end of each task when it has finished and is to sleep. Will be interrupted from
    ! sleeping when given a command from the master, which might be to do more work of shutdown
    ! Returns one (true) if the caller should do some more work or zero (false) if it is to quit
    subroutine workerSleep(returnCode)
        integer, intent(out) :: returnCode
        type(PP_Control_Package) :: out_command

        if (PP_myRank .ne. 0) then
            if (in_command%command==PP_WAKE) then
                ! The command was to wake up, it has done the work and now it needs to switch to sleeping mode
                call createCommandPackage(PP_SLEEPING, out_command)
                call MPI_Send(out_command, 1, PP_COMMAND_TYPE, 0, PP_CONTROL_TAG, MPI_COMM_WORLD, ierr)
                if (PP_pollRecvCommandRequest .ne. MPI_REQUEST_NULL) then
                    call MPI_Wait(PP_pollRecvCommandRequest, MPI_STATUS_IGNORE, ierr)
                end if
            end if
            call handleRecievedCommand(returnCode)
        else
            call errorMessage("Master process called worker poll")
            returnCode=0
        end if
    end subroutine workerSleep

    ! Retrieves the data associated with the latest command, this is just an illustration of how you can associate optional
    ! data with pool commands and we commonly associate the parent ID of a started worker
    integer function getCommandData()
        getCommandData = in_command%data
    end function getCommandData

    ! Determines whether or not the worker should stop (i.e. the master has send the STOP command to all workers)
    logical function shouldWorkerStop()
        integer flag
        if (PP_pollRecvCommandRequest .ne. MPI_REQUEST_NULL) then
            call MPI_Test(PP_pollRecvCommandRequest, flag, MPI_STATUS_IGNORE, ierr)
            if(flag == 1 .and. in_command%command==PP_STOP) then
                shouldWorkerStop = .true.
                return
            end if
        end if
        shouldWorkerStop = .false.
    end function shouldWorkerStop

    ! **********************************************************************************
    ! Below this point are private subroutines which are called internally by the module
    ! **********************************************************************************

    ! Called by the worker once we have received a pool command and will determine what to do next
    subroutine handleRecievedCommand(returnCode)
        ! We have just (most likely) received a command, therefore decide what to do
        integer, intent(out) :: returnCode
        if (in_command%command == PP_WAKE) then
            ! If we are told to wake then post a recv for the next command and return true to continues
            if (PP_DEBUG) write(6,"(A,I0,A)") "[Worker] Process ",PP_myRank," woken to work"
            call  MPI_Irecv(in_command, 1, PP_COMMAND_TYPE, 0, PP_CONTROL_TAG, MPI_COMM_WORLD, PP_pollRecvCommandRequest, ierr)
            returnCode = 1
        else if (in_command%command == PP_STOP) then
            ! Stopping so return zero to denote stop
            if (PP_DEBUG) write(6,"(A,I0,A)") "[Worker] Process ",PP_myRank," commanded to stop"
            returnCode = 0
        else
            call errorMessage("Unexpected control command")
            returnCode = 0;
        end if
    end subroutine handleRecievedCommand


    ! Called by the master and will start awaiting processes (signal to them to start) if required.
    ! By default this works as a process pool, so if there are not enough processes to workers then it will quit
    ! but this can be changed by modifying options in the pool.
    ! It takes in the awaiting worker number to send rank data to/from, the parent rank of the process to start
    ! and returns the process rank that this awaiting worker was on. In the default case of #workers > pool capacity
    ! causing an abort, then this will only be called to start single workers and as such the parent ID and return rank
    ! will match perfectly. If you change the options to allow for workers to queue up if there is not enough MPI
    ! capacity then the parent and return rank will be -1 for all workers started which do not match the provided awaiting Id
    subroutine startAwaitingProcessesIfNeeded(awaitingId, parent, resultRank)
        integer, intent(in) :: awaitingId, parent
        integer, intent(out) :: resultRank

        integer :: i
        type(PP_Control_Package) :: out_command

        resultRank=-1
        if (PP_processesAwaitingStart .ge. 1) then
            do i=1,PP_numProcs - 1
                if (PP_active(i) .eqv. .false.) then
                    PP_active(i) = .true.
                    call createCommandPackage(PP_WAKE, out_command)
                    out_command%data = merge(parent, -1, awaitingId == PP_processesAwaitingStart)
                    if (PP_DEBUG) write(6,"(A,I0)") "[Master] Starting process ",i
                    call MPI_Send(out_command, 1, PP_COMMAND_TYPE, i, PP_CONTROL_TAG, MPI_COMM_WORLD, ierr)
                    if (awaitingId == PP_processesAwaitingStart) resultRank = i  !Will return this rank to the caller
                    PP_processesAwaitingStart = PP_processesAwaitingStart - 1
                    if (PP_processesAwaitingStart == 0) then
                        exit
                    end if
                end if

                if (i == PP_numProcs-1) then
                    ! If I reach this point, I must have looped through the whole array and found no available processes
                    if(PP_QuitOnNoProcs) then
                        call errorMessage("No more processes available")
                    end if

                    if (PP_IgnoreOnNoProcs) then
                        write(0,*) "[ProcessPool] Warning. No processes available. Ignoring launch request."
                        PP_processesAwaitingStart = PP_processesAwaitingStart - 1
                    end if
                    ! otherwise, do nothing; a process may become available on the next iteration of the loop
                end if
            end do
        end if
    end subroutine startAwaitingProcessesIfNeeded

    ! A helper function which will create a command package from the desired command
    subroutine createCommandPackage(desiredCommand, package)
        integer, intent(in) :: desiredCommand
        type(PP_Control_Package), intent(inout) :: package

        package%command = desiredCommand
    end subroutine

    ! Writes an error message to stderr and MPI Aborts
    subroutine errorMessage(message)
        character(len=*), intent(in) :: message
        write(0,"(A)") message
        call MPI_Abort(MPI_COMM_WORLD, 1, ierr);
    end subroutine errorMessage

    ! Initialises the command package MPI type, we use this to illustrate how additional information (this case
    ! a worker ID) can be associated with commands
    subroutine initialiseType
        call MPI_Type_Contiguous(2, MPI_INTEGER, PP_COMMAND_TYPE, ierr)
        call MPI_Type_commit(PP_COMMAND_TYPE, ierr)
    end subroutine initialiseType
end module pool
