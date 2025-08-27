These are some multithreaded sorting programs.
In the file MultiThreadedSort.c is combined quicksort with merge sort.
To compile you need Linux , gcc and gnu and lpthread available.
This file contains a program that generates a string of 100.000 integers,
split the string in 4, spawns 4 threads that sort the 4 substrings using
qsort C library function , use 2 threads to merge the 4 substrings in 2 strings and after merge the 2 strings in one string.
It also sorts the whole string using single threaded qsort and display the time
from single threaded approach compared to multi-threaded approach.
To compile the program with optimizations:
gcc -std=gnu17 -O3 -march=native -ftree-vectorize  -fsimd-cost-model=unlimited  MultiThreadedSort.c -lpthread -o MultiThreadedSort
You can skip the -std=gnu17 if you do not have a very modern gcc
The CppParalelSorting.cpp is a very simple program that generates a 100.000 integers vector and sorts the vector using C++ paralel sort from <algorithm> standard library using also SIMD instructions and display the time for this operation.
You will need a modern g++ and the library libtbb-dev to compile and build this C++ program.
To install the libtbb-dev on Linux Ubuntu and Debian like:
sudo apt install libtbb-dev
The command to compile and build the C++ CppParalelSorting.cpp:
g++ -std=c++17 -O3 CppParalelSorting.cpp -o cppSort -ltbb

