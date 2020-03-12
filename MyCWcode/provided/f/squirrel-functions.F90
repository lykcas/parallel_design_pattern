module squirrel_functions
  use ran2_mod
  implicit none

contains
  ! Initialises the random number generator, call it once at the start of the program on each process. The input
  ! value should be negative, non-zero and different on each process, I suggest something like -1-rank where
  ! rank is the MPI rank.
  ! ran2 is a flawed RNG and must be used with particular care in parallel however it is perfectly sufficient
  ! for this course work
  ! The seed is modified and then passed to each other subsequent function that relies on the random generator,
  ! which also modify the seed
  subroutine initialiseRNG(seed)
    integer, intent(inout) :: seed

    real :: notused
    notused=ran2(seed)
  end subroutine initialiseRNG

  ! Simulates the step of a squirrel. You can call this with the arguments (0,0,&x,&y,&state)
  ! to determine a random initial starting point.
  ! x_new and y_new are the new x and y coordinates, state is used in the random number
  ! generator and is also modified.
  ! x_new can point to x, and y_new can point to y
  subroutine squirrelStep (x, y, x_new, y_new, state)
    real, intent(in) :: x, y
    real, intent(out) :: x_new, y_new
    integer, intent(inout) :: state

    real :: diff

    diff=ran2(state) !don't worry that diff is always positive; this is deal with by the periodic BCs
    x_new=(x+diff)-int(x+diff)
    diff=ran2(state)
    y_new=(y+diff)-int(y+diff)
  end subroutine squirrelStep

  ! Determines whether a squirrel will give birth or not based upon the average population and a random seed
  ! which is modified. You can enclose this function call in an if statement if that is useful.
  logical function willGiveBirth(avg_pop, state)
    real, intent(in):: avg_pop
    integer, intent(inout):: state

    real:: tmp, probability

    probability=100.0 ! Decrease this to make more likely, increase less likely
    tmp=avg_pop/probability

    willGiveBirth = (ran2(state).lt.(atan(tmp*tmp)/4*tmp))
  end function willGiveBirth

  ! Determines whether a squirrel will catch the disease or not based upon the average infection level
  ! and a random seed which is modified. You can enclose this function call in an if statement if that is useful.
  logical function willCatchDisease(avg_inf_level, state)
    real, intent(in):: avg_inf_level
    integer, intent(inout):: state

    real :: probability

    probability=1000.0 ! Decrease this to make more likely, increase less likely

    if(avg_inf_level.lt.40000) then
      willCatchDisease=ran2(state).lt.atan((avg_inf_level)/probability/(4*atan(1.)))
    else
      willCatchDisease=ran2(state).lt.atan((40000)/probability/(4*atan(1.0)))
    end if
  end function willCatchDisease

  ! Determines if a squirrel will die or not. The state is used in the random number generation and
  ! is modified. You can enclose this function call in an if statement if that is useful.
  logical function willDie(state)
    integer, intent(inout) :: state

    willDie = (ran2(state).lt.0.166666666)
  end function willDie

  ! Returns the id of the cell from its x and y coordinates.
  integer function getCellFromPosition(x, y)
    real, intent(in) :: x, y

    getCellFromPosition=(int(x*4)+4*(int(y*4)))
  end function getCellFromPosition
end module squirrel_functions

