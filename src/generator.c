#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <mpi.h>
#include "solver.h"

// #define BGQ 1 // when running BG/Q, comment out when testing on mastiff
#ifdef BGQ
#include<hwi/include/bqc/A2_inlines.h>
#define processor_frequency 1600000000.0 // processing speed for BG/Q
#else
#define GetTimeBase MPI_Wtime
#define processor_frequency 1.0 // 1.0 for mastiff since Wtime measures seconds, not cycles
#endif

// MPI data
int numRanks = -1; // total number of ranks in the current run
int rank = -1; // our rank

// puzzle data
const int boardSize = 16;  // size of both board dimensions
const int removePercent = 50;  // what percentage of cells to remove for non-evil puzzles
int regionSize;
int** board;

/**
 * allocate a contiguous 2d array of ints
 * @param rows: number of rows in the array
 * @param cols: number of columns in the array
 * @returns: a dynamically allocated contiguous 2d array
 */
int **alloc_2d_int(int rows, int cols) {
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    for (int i=0; i<rows; ++i)
        array[i] = &(data[cols*i]);
    return array;
}

/**
 * generate and return a random integer between min (inclusive) and max (inclusive)
 * @param min: the lowest (inclusive) value we should be able to generate
 * @param max: the highest (inclusive) value we should be able to generate
 * @returns: a random integer between min (inclusive) and max (inclusive)
 */
int randInt(int min, int max) {
	return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

/**
 * initialize the board to a zeroed 2-dimensional array of boardSize x boardSize
 */
void initBoard() {
	board = alloc_2d_int(boardSize, boardSize);
	for (int i = 0; i < boardSize; ++i)
		for (int r = 0; r < boardSize; ++r)
			board[i][r] = 0;
}

/**
 * output the board in ascii form
 */
void printBoard() {
	int boardSizeDigits = log10(boardSize) + 1;
	for (int i = 0; i < boardSize; ++i) {
		putchar('[');
		for (int r = 0; r < boardSize; ++r) {
			// draw blank
			if (board[i][r] == 0)
				for (int j = 0; j < boardSizeDigits; ++j) putchar(' ');
			else {
				// draw given
				int curDigits = log10(board[i][r]) + 1;
				for (int j = 0; j < boardSizeDigits - curDigits; ++j) putchar(' ');
				printf("%d", board[i][r]);
			}
			if ((r+1)%regionSize == 0)
				printf(r == boardSize-1 ? "]" : "] [");
			else
				putchar('|');
		}
		putchar('\n');
		if ((i+1)%regionSize == 0 && i != boardSize-1) {
			for (int r = 0; r < (boardSizeDigits+1)*(boardSize+2) + 1; ++r) putchar('-');
			putchar('\n');
		}
	}
}

/**
 * insert an integer in place into a sorted integer array
 * @param arr: the array in which to insert the specified value
 * @param newVal: the value to insert into the array
 * @param arrLen: the number of elements currently stored in the array (we assume at least 1 additional slot is available for insertion)
 */
void insertInPlace(int *arr, int newVal, int arrLen) {
	int i;
	for (i=arrLen-1; i >= 0  && arr[i] > newVal; --i) arr[i+1] = arr[i];
	arr[i+1] = newVal;
}

/**
 * determine whether or not the board is in a solved state (adheres to all sudoku rules)
 * @param iBoard: 2d array containing the board data
 * @returns: whether the board is solved (true) or unsolved (false)
 */
bool boardIsSolved(int** iBoard) {
	// check for row/col duplicates
	for (int i = 0; i < boardSize; ++i)
		for (int r = 0; r < boardSize; ++r)
			for (int k = 0; k < boardSize; ++k)
				if ((iBoard[i][k] == iBoard[i][r] && k != r) || (iBoard[k][r] == iBoard[i][r] && k != i)) return false;

	// check for region duplicates
	int regionVals[boardSize];
	for (int i = 0; i < regionSize; ++i) {
		for (int r = 0; r < regionSize; ++r) {
			for (int j = 0; j < regionSize; ++j) {
				for (int k = 0; k < regionSize; ++k) {
					insertInPlace(regionVals, iBoard[i*regionSize + j][r*regionSize + k], j*regionSize+k);
				}
			}
			if (regionVals[0] == 0) return false;
			for (int i = 1; i < boardSize; ++i)
				if (regionVals[i] != regionVals[i-1]+1) return false;
		}
	}
	return true;
}

/**
 * determine whether or not a single cell on the board adheres to the sudoku rules
 * @param row: the row of the cell we wish to check for validity
 * @param col: the column of the cell we wish to check for validity
 * @param iBoard: 2d array containing the board data
 * @returns: whether the cell at iBoard[row][col] is valid (true) or not (false)
 */
bool cellIsValid(int row, int col, int** iBoard) {
	// check for row/col duplicates
	for (int k = 0; k < boardSize; ++k)
		if ((iBoard[row][k] == iBoard[row][col] && k != col) || (iBoard[k][col] == iBoard[row][col] && k != row)) return false;

	//check for region duplicates
	int regionRow = row-(row%regionSize);
	int regionCol = col-(col%regionSize);
	for (int i = 0; i < regionSize; ++i) {
		for (int r = 0; r < regionSize; ++r) {
			if (iBoard[regionRow + i][regionCol + r] == iBoard[row][col] && !(regionRow + i == row && regionCol + r == col)) return false;
		}
	}
	return true;
}

/**
 * determine whether or not the board contains a value in every cell
 * @param iBoard: 2d array containing the board data
 * @returns: the location of the first unfilled cell (in the form row*boardSize + col), or -1 if all cells are filled
 */
int boardIsFilled(int** iBoard) {
	for (int i = 0; i < boardSize; ++i) {
		for (int r = 0; r < boardSize; ++r) {
			if (iBoard[i][r] == 0) return i*boardSize + r;
		}
	}
	return -1;
}

/**
 * replace all occurrences of digit a on the board with digit b and vice versa
 */
void swapDigits(int a, int b) {
	if (a == b) return;
	for (int i = 0; i < boardSize; ++i) {
		for (int r = 0; r < boardSize; ++r) {
			if (board[i][r] == a) board[i][r] = b;
			else if (board[i][r] == b) board[i][r] = a;
		}
	}
}

/**
 * swap two board rows
 * @param r1: the first row to swap
 * @param r2: the second row to swap
 */
void swapRows(int r1, int r2) {
	if (r1 == r2) return;
	int swp;
	for (int i = 0; i < boardSize; ++i) {
		swp = board[r1][i];
		board[r1][i] = board[r2][i];
		board[r2][i] = swp;
	}
}

/**
 * swap two board columns
 * @param c1: the first column to swap
 * @param c2: the second column to swap
 */
void swapCols(int c1, int c2) {
	if (c1 == c2) return;
	int swp;
	for (int i = 0; i < boardSize; ++i) {
		swp = board[i][c1];
		board[i][c1] = board[i][c2];
		board[i][c2] = swp;
	}
}

/**
 * generate a boardSize x boardSize board
 * @param isEvil: whether the board should force only one solution (true) or allow for potentially multiple solutions (false)
 */
void generateBoard(bool isEvil) {
	// start with repeated regions, offset by regionSize
	for (int i = 0; i < boardSize; ++i) {
		for (int r = 0; r < boardSize; ++r) {
			board[i][r] = (r + i*regionSize) % boardSize + 1;
		}
	}

	// now shift regions horizontally by their region number
	for (int i = 1; i < regionSize; ++i) {
		for (int k = 0; k < regionSize; ++k) {
			for (int h = 0; h < regionSize; ++h) {
				for (int r = 0; r < i; ++r) {
					int swp = board[h+i*regionSize][k*regionSize];
					for (int j = 0; j < regionSize-1; ++j) {
						board[h+i*regionSize][k*regionSize+j] = board[h+i*regionSize][k*regionSize+j+1];
					}
					board[h+i*regionSize][k*regionSize+regionSize-1] = swp;
				}
			}
		}
	}

	// we now have a valid sudoku board; time to make it unique
	// randomly swap boardSize^2 number of number pairs
	for (int i = 0; i < boardSize*boardSize; ++i) {
		int r1 = randInt(1,boardSize);
		int r2 = r1;
		while (r2 == r1) r2 = randInt(1,boardSize);
		swapDigits(r1, r2);
	}

	// randomly swap boardSize^2 number of row and column pairs within regions
	for (int i = 0; i < boardSize*boardSize; ++i) {
		// swap rows
		int r1 = randInt(0,boardSize-1);
		int r2 = r1;
		while (r2 == r1) r2 = randInt(r1/regionSize*regionSize, r1/regionSize*regionSize + regionSize-1);
		swapRows(r1,r2);

		// swap columns
		int c1 = randInt(0,boardSize-1);
		int c2 = c1;
		while (c2 == c1) c2 = randInt(c1/regionSize*regionSize, c1/regionSize*regionSize + regionSize-1);
		swapCols(c1,c2);

	}

	// randomly swap boardSize number of regionSize row and column groups
	for (int i = 0; i < boardSize; ++i) {
		int rr1 = randInt(0,regionSize-1);
		int rr2 = rr1;
		while (rr2 == rr1) rr2 = randInt(0,regionSize-1);

		int cr1 = randInt(0,regionSize-1);
		int cr2 = cr1;
		while (cr2 == cr1) cr2 = randInt(0,regionSize-1);

		for (int r = 0; r < regionSize; ++r) {
			swapRows(rr1*regionSize+r, rr2*regionSize+r);
			swapCols(cr1*regionSize+r, cr2*regionSize+r);
		}
	}

	puts("Finished board:");
	printBoard();
	puts(boardIsSolved(board) ? "Board passed validation test" : "Board failed validation test");

	// remove some cell values
	if (isEvil) {

	}
	else {
		// remove cells at random until we reach the defined threshold
		int removeNum = boardSize*boardSize * (removePercent/100.0f);
		printf("Removing %d cells (%d%% removal threshold)\n",removeNum, removePercent);
		for (int i = 0; i < removeNum; ++i) {
			int row = randInt(0,boardSize-1);
			int col = randInt(0,boardSize-1);
			if (board[row][col] == 0) {
				--i;
				continue;
			}
			board[row][col] = 0;
		}
	}
	puts("Stripped board:");
	printBoard();
}

int main(int argc, char *argv[]) {
	// init random using current time in seconds as seed
	srand(time(0));

	// init MPI + get size & rank, then calculate board data
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// everyone allocates memory for the starting board
	regionSize = sqrt(boardSize);
	initBoard();

	// rank 0 runs the board generation algorithm
	if (rank == 0) {
		puts("-----Generating board-----");
		fflush(stdout);
		generateBoard(false);
		puts("\n-----Solving Board-----");
		fflush(stdout);
	}
	// rank 0 sends initial board to all other ranks
	MPI_Bcast(&(board[0][0]), boardSize*boardSize, MPI_INT, 0, MPI_COMM_WORLD);

	// analyze solver performance
	double g_start_cycles = GetTimeBase();
	serialBruteForceSolver(board);
	double time_in_secs = (GetTimeBase() - g_start_cycles) / processor_frequency;
	printf("Solved board (elapsed time %fs):\n",time_in_secs);
	printBoard();
	puts(boardIsSolved(board) ? "Board passed validation test" : "Board failed validation test");

	// all done
	free(board[0]);
	free(board);
	return EXIT_SUCCESS;
}
