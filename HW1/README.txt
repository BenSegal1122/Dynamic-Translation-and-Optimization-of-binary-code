Full Name: Ben Segal
ID: 318882347
Description of the compilation command and how to run the tool: 

1) copy the following files: ex1.cpp, make, make.rules
to the target path:<path to pin tool directory>/source/tools/SimpleExamples/
2) run: make ex1.test
3) run: <path to pin tool directory>/pin -t <path to pin tool directory>/source/tools/SimpleExamples/obj-intel64/ex1.so -- <path to executable for test>
4) for checking the runtime run: time <path to pin tool directory>/pin -t <path to pin tool directory>/source/tools/SimpleExamples/obj-intel64/ex1.so -- <path to executable for test>