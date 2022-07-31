# java-runtime-thread-wait-simulation
A proof of concept for simulating Java's "wait for threads to finish before process termination" behavior.

There are bugs in this implementation, do not use in anything serious.

- [build.sh](build.sh) makes the exponential test, `main`.
- build2.sh makes the simple test, `main2`.
- buildpoc.sh makes the original single-file test(obsolete), `poc`.
