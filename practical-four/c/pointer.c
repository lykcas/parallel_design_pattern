#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


int main()
{
  int i, j, k;
  // int c[4]={1,2,3,4};

  // // int *a[4]; //指针数组
  // // int (*b)[4]; //数组指针
  // // b=&c;
  // // //将数组c中元素赋给数组a
  // // for(int i=0;i<4;i++)
  // // {
  // //   a[i]=&c[i];
  // // }
  // int i;
  // // int a[2][4] = {1, 2, 3, 4, 5, 6, 7, 8};
  
  // int (*p)[4];
  // p = &c;
  // // for (i = 0; i < 4; i++) {
  // //   printf("p: %d, %d\n", p[i], *p[i]);
  // // }
  // for (i = 0; i < 4; i++) {
  //   printf("c %d ", &c[i]);
  // }
  // for (i = 0; i < 4; i++) {
  //   printf("* %d ", p[i]);
  //   // p++;
  // }
  
  
  // for (i = 0; i < 2; i++) {
  //   p[i] = a[i];
  //   printf("a: %d\n", a[i]);
  //   printf("p: %d\n", p[i]);
  //   // printf("- %d\n", *p[i]);
  // }
  // for (i = 0; i < 2; i++) {
  //   printf("- %d ", *p[i]);
  // }
  // printf("--- %s", *p[2]);
  for (i = 0; i < 5; i++) {
    
    printf("Hello from %ld\n", time(NULL));
    sleep(3);
  }

  return 0;
} 
