/**
 * @file main.c
 */
#include "my_runtime.h"
#include <stdio.h>

int main(int argc, char **argv)
{   
    int returnValue;
    long currentStrongThreadCount;
    
    // Do the actual work of the program.
    returnValue = main2(argc, argv);

    // | main might finish before the first spawned thread starts.
    // |     sleep a second.
    Sleep(1000);
    
    // | Do not return from main until the 
    // |     counted thread counter reaches zero.
    while (globalStrongThreadCount != 0) {
        // | Log the number of threads seen by main 
        // |     each time it wakes up.
        printf (
            "[main] globalStrongThreadCount: %ld.\n", 
            globalStrongThreadCount
        );
        // | Bypass the need to use winpty to run the program in MinGW.
        fflush(stdout);
        
        // | Reduce strain on the CPU from the infinite loop.
        Sleep(1000);
    }

    return 0;
}