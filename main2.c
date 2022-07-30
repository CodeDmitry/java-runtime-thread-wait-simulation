/**
 * @file main2.c
 */
#include "my_runtime.h"
#include <stdio.h>

// | Limit our test to 100 threads to actually do anything in this test.
volatile long globalRepeatCountdown = 100;

// | Thread callback that creates more threads exponentially(2 each)
// |     with chain ending when globalRepeatCounter reaches zero.
DWORD CALLBACK ThreadProc0001(LPVOID unused) {
    // | Do nothing if the countdown is below one, terminating the chain
    // |     of exponential thread creation.
    if (globalRepeatCountdown > 0) {
        // | Log the the active strong thread count
        // |     as seen by this callback.
        printf (
            "[ThreadProc0001] globalStrongThreadCount: ld.\n", 
            globalStrongThreadCount
        );
        // | Bypass the need to use winpty to run the program in MinGW.
        fflush(stdout);
        
        InterlockedDecrement(&globalRepeatCountdown);
        
        // | Start two threads, this is guaranteed to exceed our 
        // |     globalRepeatCountdown.
        // | It grows exponentially,
        // |     to test our race conditions.
        CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);
        CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);
    }
    
    return 0;
}

// | The process should continue running even after main2 terminates,
// |     waiting for all threads created by CreateThreadWrapper to finish.
int main2(int argc, char **argv)
{   
    CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);

    return 0;
}
