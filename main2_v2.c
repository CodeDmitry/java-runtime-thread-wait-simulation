/**
 * @file main2_v2.c
 */
#include "my_runtime.h"
#include <stdio.h>

DWORD CALLBACK ThreadProc0001(LPVOID unused) {
    printf (
        "[second thread] active threads: %ld.\n",
        globalStrongThreadCount
    );
    fflush(stdout);        
    CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);
    
    return 0;
}

// | The process should continue running even after main2 terminates,
// |     waiting for all threads created by CreateThreadWrapper to finish.
int main2(int argc, char **argv)
{   
    CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);

    return 0;
}
