#include <stdio.h>
#include <stdlib.h>


int main()
{
  // int c[4]={1,2,3,4};

  // int *a[4]; //指针数组
  // int (*b)[4]; //数组指针
  // b=&c;
  // //将数组c中元素赋给数组a
  // for(int i=0;i<4;i++)
  // {
  //   a[i]=&c[i];
  // }
  int i;
  int a[2][4] = {1, 2, 3, 4, 5, 6, 7, 8};
  for (i = 0; i < 8; i++) {
    printf("ai= %d ", a[i]);
  }
  printf("\n");
  for (i = 0; i < 8; i++) {
    printf("a*= %d ", *a[i]);
  }
  int *p[2];
  for (i = 0; i < 2; i++) {
    p[i] = a[i];
    printf("a: %d\n", a[i]);
    printf("p: %d\n", p[i]);
    // printf("- %d\n", *p[i]);
  }
  for (i = 0; i < 2; i++) {
    printf("- %d ", *p[i]);
  }
  // printf("--- %s", *p[2]);

  return 0;
} 
