/**
 * @file poc.c
 */
#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// | My version of MinGW does not have <handleapi.h> for 
// |     CompareObjectHandles which is necessary for comparing handles, 
// |     == does not work for handles because different handles can 
// |     hold the same thread, GetCurrentThread() returns a pseudohandle, 
// |     and DuplicateHandle creates a completely new handle which is 
// |     not equal to the handles in the registry.
BOOL WINAPI (*CompareObjectHandles2) (
    HANDLE hFirstObjectHandle, 
    HANDLE hSecondObjectHandle 
);

// | A mutex to only allow one thread access the 
// |     globalStrongThreadCount at a time.
HANDLE globalThreadRegistryMutex = 0;
// | Global thread list of threads that need to be waited for
// |     before the thread terminates.
// | NEVER actually waited on, because it reallocates too often.
HANDLE *globalThreadRegistryHandles = 0;
// | Global thread count of threads that need to be waited for
// |     before the thread terminates.
volatile LONG globalThreadRegistrySize = 0;

// | Used by CreateThreadWrapper to provide the original thread proc
// |     to the thread alongside the original arguments.
typedef struct CreateThreadProcWrapperArguments {
    LPTHREAD_START_ROUTINE original_proc;
    LPVOID *original_arguments;    
} CreateThreadProcWrapperArguments;

// | Proxy to actual user proc, used to intercept when the thread finishes.
// | Passed the original callback alongside the original arguments.
// | The parameter is passed as a UNIQUE pointer to mallocated memory,
// |     CreateThreadProcWrapper is responsible for releasing it. 
DWORD CALLBACK CreateThreadProcWrapper(LPVOID parameter) 
{
    DWORD innerResult;
    CreateThreadProcWrapperArguments *args;

    // | Alias our arguments to a usable form.
    args = (CreateThreadProcWrapperArguments *)parameter;
        
    // | Run the users proc with the users original arguments, 
    // |     we will use the result as the return value.
    innerResult = args->original_proc(args->original_arguments);
    
    // | We want to remove the thread from the registry.
    // | Only one thread at a time can access the thread registry. 
    WaitForSingleObject(globalThreadRegistryMutex, INFINITE);
    // | Critical section, we can't race on our registry handles.    
    {
        LONG i;
        HANDLE hCurrentThread;
        HANDLE hRegisteredThread;
        
        // | Reminder: This is a pseudohandle which can only be
        // |     compared with CompareObjectHandle.
        hCurrentThread = GetCurrentThread();
        
        // | Remove the current thread from the registry.
        for (LONG i = 0; i < globalThreadRegistrySize; ++i) {
            HANDLE hRegisteredThread = globalThreadRegistryHandles [
                i
            ];
            // | if the thread is our thread, remove it,
            // |    then swap it for the last thread in the registry,
            // |    then realloc it down and reduce the size.
            if (CompareObjectHandles2(hRegisteredThread, hCurrentThread)) {
                // | Overwrite our index with last thread in the registry.
                // |     the last index will be realloc-truncated.
                globalThreadRegistryHandles [
                    i
                ] = globalThreadRegistryHandles [
                    globalThreadRegistrySize - 1
                ];
   
                // | Truncate the registry down by one element using realloc.
                globalThreadRegistryHandles =
                    (HANDLE *)realloc (
                        globalThreadRegistryHandles,
                        sizeof(HANDLE) * (globalThreadRegistrySize - 1)
                    ); 

                // | Decrement our registry size.
                --globalThreadRegistrySize;
                printf (
                    "[deregistration] registry size: %ld.\n", 
                    globalThreadRegistrySize
                );
            }
        }
        
        // | Ending the critical section.
        ReleaseMutex(globalThreadRegistryMutex);
    }

    // | Cleanup.
    free(parameter);

    return innerResult;
}

// | A wrapper of CreateThread that decrements globalStrongThreadCount
// |     when the thread is about to terminate. 
HANDLE WINAPI CreateThreadWrapper (
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId )
{
    HANDLE hThread;
    CreateThreadProcWrapperArguments *wrapper_args;

    // | We want to pass the arguments to the wrapper thread proc.
    // | wrapper_args behave as a UNIQUE pointer to dynamic memory,
    // |     responsible to be cleaned up by CreateThreadProcWrapper.    
    wrapper_args = (CreateThreadProcWrapperArguments *)malloc (
        sizeof(CreateThreadProcWrapperArguments)
    );
    wrapper_args->original_proc = lpStartAddress;
    wrapper_args->original_arguments = lpParameter;
    
    WaitForSingleObject(globalThreadRegistryMutex, INFINITE);    
    // | Critical section, we can't race on our registry handles.    
    // | The thread might finish faster than we can add the 
    // |     thread to the global list, and try removing a thread
    // |     that does not exist, to avoid this, lock.
    {
        // puts("[registration] got mutex");    
        hThread = CreateThread (
            lpThreadAttributes,
            dwStackSize,
            CreateThreadProcWrapper,
            wrapper_args,
            dwCreationFlags,
            lpThreadId
        );
        // | Anonymous scope, resize the global thread registry
        // |     and append our thread handle to it.
        {
            // | Should be safe to directly increment because of 
            // |     the critical section.
            ++globalThreadRegistrySize;
            
            // | Realloc the registry up to our new registry size.
            globalThreadRegistryHandles = (HANDLE *)realloc (
                globalThreadRegistryHandles,
                sizeof(HANDLE) * globalThreadRegistrySize
            );
            
            // | Append our newly created thread to the thread registry.
            globalThreadRegistryHandles[globalThreadRegistrySize - 1] =
                hThread;
        }
        // | Log the number of threads after appending the thread.
        printf (
            "[registration] registry size: %ld.\n", 
            globalThreadRegistrySize
        );
        // | Bypass the need to use winpty to run the program in MinGW.
        fflush(stdout);
        
        // | Ending the critical section. 
        ReleaseMutex(globalThreadRegistryMutex);    
    }
    // | End of the critical section.

    return hThread;
}

// | Limit our test to 100 threads to actually do anything in this test.
volatile long globalRepeatCountdown = 100;

// | Thread callback that creates more threads exponentially(2 each)
// |     with chain ending when globalRepeatCounter reaches zero.
DWORD CALLBACK ThreadProc0001(LPVOID unused) {
    // | Do nothing if the countdown is below one, terminating the chain
    // |     of exponential thread creation.
    if (globalRepeatCountdown > 0) {        
        // | Log the number of threads seen by main 
        // |     each time it wakes up.
        printf (
            "[ThreadProc0001] registry size: %ld\n", 
            globalThreadRegistrySize
        );
        // | Bypass the need to use winpty to run the program in MinGW.
        fflush(stdout);
        
        InterlockedDecrement(&globalRepeatCountdown);
        CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);
        CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);
    }
    return 0;    
}

int main(int argc, char **argv)
{   
    // | Anonymous scope: fetch CompareObjectHandle from kernelbase.dll
    // | My version of MinGW does not have <handleapi.h> for 
    // |     CompareObjectHandles which is necessary for comparing handles, 
    // |     == does not work for handles because different handles can 
    // |     hold the same thread, GetCurrentThread() returns a pseudohandle, 
    // |     and DuplicateHandle creates a completely new handle != 
    // |     to the handles in the registry.
    {
        HMODULE hDllKernelbase = 0;
        
        assert(hDllKernelbase = LoadLibrary("kernelbase.dll"));
        
        // | Anonymous scope: Fetching CompareObjectHandles 
        // |     from kernelbase.dll.
        // | Strict aliasing memcpy workaround.
        // |     it is tempting to *(void **)&, but that's technically
        // |     undefined behavior now.
        {
            FARPROC proc;
            proc = GetProcAddress (
                hDllKernelbase, 
                "CompareObjectHandles"
            );
            memcpy (
                &CompareObjectHandles2,
                &proc, 
                sizeof(CompareObjectHandles2)
            );            
            assert(CompareObjectHandles2);
        }
        
        FreeLibrary(hDllKernelbase);
    }
    
    // | Initialization of the global mutex.
    assert(globalThreadRegistryMutex = CreateMutex(0, 0, 0));

    // | The only statement main actually wants to do.
    CreateThreadWrapper(0, 0, ThreadProc0001, 0, 0, 0);
    
    // | Do not return from main until the 
    // |     counted thread counter reaches zero.
    while (globalThreadRegistrySize != 0) {
        // | Log the number of threads seen by main 
        // |     each time it wakes up.
        printf (
            "[main] globalThreadRegistrySize: ld.\n", 
            globalThreadRegistrySize
        );
        // | Bypass the need to use winpty to run the program in MinGW.
        fflush(stdout);
        
        // | Reduce strain on the CPU from the infinite loop.
        Sleep(1000);
    }

    return 0;
}