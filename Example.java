public final class Example
{
    // | prints current thread count and queues the next iteration.
    static void step() {
        System.out.println (
            "[second thread] active threads: " + 
            Thread.activeCount() +
            "."
        );   
        new Thread(() -> step()).start();
    }
    
    public static void main(String[] args) throws Exception
    {
        // | queue the first iteration.
        new Thread(() -> step()).start();      
    }
}
