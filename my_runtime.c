/**
 * @file my_runtime.c
 */

#include <Windows.h>
#include <stdio.h>

// | A mutex to only allow one thread access the globalStrongThreadCount
// |     at a time.
HANDLE globalStrongThreadCountMutex = 0;
// | Global thread count of threads that need to be waited for
// |     before the thread terminates.
volatile LONG globalStrongThreadCount = 0;

// | This function will can start threads without worrying about them
// |     ending as soon as it finishes.
extern int main2(int argc, char **argv);

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
    LONG newThreadCount;
    CreateThreadProcWrapperArguments *args;
    
    // | Alias our arguments to a usable form.
    args = (CreateThreadProcWrapperArguments *)parameter;
        
    // | Run the users proc with the users original arguments, 
    // |     we will use the result as the return value.
    innerResult = args->original_proc(args->original_arguments);
    
    WaitForSingleObject(globalStrongThreadCountMutex, INFINITE);
    // | Start of a critical section: reduce thread count by one, 
    // |     as it has finished its job.
    {

        // | We can no longer use ++ because of an unknown bug, 
        // |     InterlockedIncrement is used instead(regarding the 
        // |     single commented-out statement below).
        // newThreadCount = globalStrongThreadCount--;
        newThreadCount = InterlockedDecrement(&globalStrongThreadCount);

        // | Log the thread counter after decrementing.
        printf (
            "[CreateThreadProcWrapper] globalStrongThreadCount: %ld.\n", 
            newThreadCount
        );
        // | Bypass the need to use winpty to run the program in MinGW.
        fflush(stdout);

        // | Ending the critical section. 
        ReleaseMutex(globalStrongThreadCountMutex);
    }
    // | End of the critical section.

    // | Relase the dynamic memory used by parameter. 
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
    LONG newThreadCount;
    
    // | We want to pass the arguments to the wrapper thread proc.
    // | wrapper_args behave as a UNIQUE pointer to dynamic memory,
    // |     responsible to be cleaned up by CreateThreadProcWrapper.
    wrapper_args = (CreateThreadProcWrapperArguments *)malloc (
        sizeof(CreateThreadProcWrapperArguments)
    );
    wrapper_args->original_proc = lpStartAddress;
    wrapper_args->original_arguments = lpParameter;
    
    WaitForSingleObject(globalStrongThreadCountMutex, INFINITE); 
    // | Start of critical section: we can't race on our registry handles.    
    // | The thread might finish faster than we can add the 
    // |     thread to the global list, and try removing a thread
    // |     that does has not been yet added to avoid this, lock.
    {       
        // | Create a new thread using the wrapped arguments, allowing it to
        // |     proxy all the behavior of the original thread while having
        // |     an opportunity to subtract the thread count after it has
        // |     finished.
        hThread = CreateThread (
            lpThreadAttributes,
            dwStackSize,
            CreateThreadProcWrapper,
            wrapper_args,
            dwCreationFlags,
            lpThreadId
        );
        
        // | We can no longer use -- because of an unknown bug, 
        // |     InterlockedDecement is used instead(regarding the 
        // |     single commented-out statement below).
        // newThreadCount = globalStrongThreadCount--;
        newThreadCount = InterlockedIncrement(&globalStrongThreadCount);

        // | Log the thread counter after incrementing.
        printf (
            "[CreateThreadWrapper] globalStrongThreadCount: %ld.\n", 
            newThreadCount
        );
        // | Bypass the need to use winpty to run the program in MinGW.
        fflush(stdout);
        
        // | Ending the critical section. 
        ReleaseMutex(globalStrongThreadCountMutex);    
    }
    // | End critical section.

    return hThread;
}

