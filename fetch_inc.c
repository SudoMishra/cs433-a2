#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

int ticket=0;
unsigned char FetchAndInc(int *ticket) 
{
    	// int result;
        // int oldVal;
        int oldValOut;
        int inc = 1;
    	// unsigned char result;
        asm("lock xchgl %0 %1 \n add %0 %2"
            :"+m"(*ticket), "=r" (oldValOut) 
            :"r"(inc)
            :);
    	// // asm("lock xchgl %4, %1 \n addi %0x1 %0"
        //         :"=qm"(result),  "+m" (*ptr), "=a" (oldValOut)
        //         :"a" (oldVal),  "r" (newVal)
        //         : );
    	return oldValOut;
}
int main(int argc, char* argv[])
{
    int i,*id;
    if(argc!=2)
    {
        printf("Need no. of threads");
        exit(1);
    }
    int result;
    for(i=0;i<10;i++)
    {
        result = FetchAndInc(&ticket);
        printf("Result: %d \nTicket: %d\n",result,ticket);
    }

    return 0;
}