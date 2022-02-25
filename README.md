# Tester

This is a simple tool to execute a program on Windows and report its CPU and memory usage, or to optionally kill it if it takes too long. You can think of it as limited versions of Unix `time` and `timeout` rolled into one, with some memory usage tracking added on top.

Minimally, the tool just executes another program and reports its CPU and memory usage:
```
>tester testee
CPU time used: 15 ms
Memory used: 3 MB
```

If the command to execute contains spaces, it must be enclosed in quotes
```
>tester "testee command line options"
CPU time used: 15 ms
Memory used: 3 MB
```

I originally wrote this tool to help me evaluate solutions to programming assignments. To support batch scripting, the tool allows setting of CPU and memory usage limits and returns non-zero error codes when the limits are exceeded:
```
>tester -t 1000 cpu_hog_testee
CPU time used: 1500 / 1000 ms
Memory used: 3 MB
CPU time limit exceeded!
>echo %errorlevel%
3
>tester -m 10 memory_hog_testee
CPU time used: 15 ms
Memory used: 15 / 10 MB
Memory limit exceeded!
>echo %errorlevel%
4
```

The tool also kills the program after twice the time limit has elapsed in wall clock time, even when the program uses no CPU time (most likely blocking on input that's not there):
```
>tester -t 1000 idle_testee
Wall time limit exceeded!
Attempting to kill... OK
>echo %errorlevel%
3
```

The tool also returns an error code when the program under test fails or can't be executed at all:
```
>tester failing_testee
Process exit code: 5
>echo %errorlevel%
1
```

The tool can track CPU and memory usage of multithreaded programs, but not of programs spawning multiple processes. The latter case is detected and reported:
```
>tester forking_testee
Process forked!
>echo %errorlevel%
2
```

Recently I had a need for such a tool again and since it seems to still work on current versions of Windows, decided to upload it here in case it's useful for someone else as well. But keep in mind, this is not a sandbox for safely executing potentially malicious code!

## Files

- `tester.c` - source code of the tool
- `tester.exe` - statically linked 64-bit executable of the tool in case you don't have a C compiler
- `test_time.c` - a program that uses CPU time in multpiple threads, to test time accounting of the tool
- `test_mem.c` - a program that allocates some memory in multpiple threads, to test memory accounting of the tool
- `test_idle.c` - a program that idles without using CPU time
- `test_fork.c` - a program that spawns a sub-process
