Full Name: Ben Segal
ID: 318882347
Full Name: Alon Hirshberg
ID: 318301710
Description of the compilation command and how to run the tool: 

1) copy the following files: ex3cpp, make, make.rules
to the target path:<path to pin tool directory>/source/tools/SimpleExamples/
2) run: make ex3.test
3) run: <path to pin tool directory>/pin -t <path to pin tool directory>/source/tools/SimpleExamples/obj-intel64/ex3.so  -mt_dump_counters -- <path to executable for test>
4) for checking the runtime run: time <path to pin tool directory>/pin -t <path to pin tool directory>/source/tools/SimpleExamples/obj-intel64/ex3.so -mt_dump_counters -- <path to executable for test>

Note:
The output .csv file called bbl_counters.csv and it has special format:
For each iteration that the thread runs and derives data from the running program you'll see the following headers:
|    optimizied?    |   diff from previous iteration    |     execution counter of iteration: <current iteration number>    |    bbl address    |
-------------------------------------------------------------------------------------------------------------------------------------------------
|                            |                                                   |                                                                                                          |                           | 

a little explanation about each header:
1) bbl address - as the name says, the address of the bbl where we've decided that the first assembly command which is not jump defines the address of the basic block.
2) execution counter of iteration: <current iteration number>- the number of executions of the basic block for this specific thread iteration.
3) diff from previous iteration - a simple subtraction calculation between the counter of the current thread iteration and the previous iteration to understand the scale of the difference.
4) optimized? - a flag that indicates if we used the "RAX kill" optimization. meaning that instead of store RAX before we are using it as our counter incrementor and restore it after doing the calculation, RAX kill allow us to skip these store and load commands.

* We've decided to print only the basic block with execution counter greater than 0.
* Each iteration will have an indentation of 4 tabs so you can distinguish between the previous and the next iteration.
* Another interesting information that you can see - at the end of the prints of first iteration (iteration number 0),
you may see another line which tells the number of optimized basic blocks, the number of non-optimized basic blocks and the 
fraction (%) of the optimized in basic block compared with the total basic blocks that had execution counter greater than 0.

