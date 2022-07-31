# java-runtime-thread-wait-simulation
A proof of concept for simulating Java's ["wait for threads to finish before process termination"](Example.java) behavior.


There are bugs in this implementation, do not use in anything serious.

- [build.sh](build.sh) makes the exponential test, `main`, using [main.c](main.c) [main2.c](main2.c) [my_runtime.c](my_runtime.c).
- [build2.sh](build2.sh) makes the simple test, `main2`, using [main.c](main.c) [main2_v2.c](main2_v2.c) [my_runtime.c](my_runtime.c).
- [buildpoc.sh](buildpoc.sh) makes the original single-file test(obsolete), `poc`, using [poc.c](poc.c).
