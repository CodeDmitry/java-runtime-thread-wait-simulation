/**
 * @file my_runtime.h
 */
#ifndef MY_RUNTIME_H_7FBD466E_0E79_47B7_ADF9_10CAF32F6BE6
#define MY_RUNTIME_H_7FBD466E_0E79_47B7_ADF9_10CAF32F6BE6

#include <Windows.h>

// | A mutex to only allow one thread access the globalStrongThreadCount
// |     at a time.
extern HANDLE globalStrongThreadCountMutex;
// | Global thread count of threads that need to be waited for
// |     before the thread terminates.
volatile extern LONG globalStrongThreadCount;

// | Since main is busy making sure it does not exit before any
// |     "strong" threads exit, the runtime will use main2 as the
// |     actual entry point which doesn't need to worry about these
// |     details.
extern int main2(int argc, char **argv);

// | A wrapper of CreateThread that decrements globalStrongThreadCount
// |     when the thread is about to terminate. 
extern HANDLE WINAPI CreateThreadWrapper (
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId 
);

#endif /* MY_RUNTIME_H_7FBD466E_0E79_47B7_ADF9_10CAF32F6BE6 */
