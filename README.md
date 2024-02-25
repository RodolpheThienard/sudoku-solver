# sudoku-solver
MASTER 2 CHPS, APM Project 

## Description
This project is a sudoku solver. It can solve a grid of size 1x1 to 8x8. It uses a Wave-Function Collapse algorithm to solve the grid.

## Modules
The modules necessary to run the project on romeo cluster are :
- llvm/14.0.5-gcc\_10.2.0-targets\_X86\_NVPTX 
- cmake/3.27.9\_spack2021\_gcc-12.2.0-rvyk 

## Tests
To create test, you have to check some parameters. Decide if the grid is deterministe or not, and if it will be a grid or a simple. After, create the file by naming it with "det" or "nondet", "grid" or "simple" and its size.  
Example : det-grid-2x2.data  
Ensure that you can compute his result by yourself and add it in test_output by naming it with the same name excepting the extension ".data". 
Now run `./run_test.sh`

## How to run
To run the different versions of the project, you can use those commands :
- Sequential CPU version       : `./wfc -lcpu    -s<seed(s)> <input_file>`
- Parallel CPU version         : `./wfc -lomp    -s<seed(s)> <input_file>`
- GPU version using OMP Target : `./wfc -ltarget -s<seed(s)> <input_file>`
