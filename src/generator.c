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
int numPeers; // number of peers per cell

// puzzle data
const int boardSize = 9;  // size of both board dimensions
const int removePercent = 55;  // what percentage of cells to remove
int regionSize;
int** board;
int**** peers;

/**
 * allocate a contiguous 2d array of ints
 * @param rows: number of rows in the array
 * @param cols: number of columns in the array
 * @returns: a dynamically allocated contiguous 2d array of dims row x column
 */
int **alloc_2d_int(int rows, int cols) {
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    for (int i=0; i<rows; ++i)
        array[i] = &(data[cols*i]);
    return array;
}

/**
 * deallocate a contiguous 2d array of ints
 * @param arr: the array we wish to free
 */
void dealloc_2d_int(int **arr) {
	free(arr[0]);
	free(arr);
}

/**
 * allocate a contiguous 3d array of ints
 * @param x: number of values in the first dimension
 * @param y: number of values in the second dimension
 * @param z: number of values in the third dimension
 * @returns: a dynamically allocated contiguous 3d array of dims x x y x z
 */
int ***alloc_3d_int(int x, int y, int z) {
	int *p = (int*) malloc(x * y * z * sizeof(int));
	int ***q = (int***) malloc(x * sizeof(int**));
	for (int i = 0; i < x; i++) {
		q[i] = (int**) malloc(y * sizeof(int*));
		for (int j = 0; j < y; j++) {
			int idx = x*j + x*y*i;
			q[i][j] = &p[idx];
		}
	}
	return q;
}

/**
 * deallocate a contiguous 3d array of ints
 * @param x: number of values in the first dimension
 * @param arr: the array we wish to free
 */
void dealloc_3d_int(int x, int ***arr) {
	free(arr[0][0]);
	for(int i = 0; i < x; i++) {
		free(arr[i]);
	}
	free(arr);
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
 * initialize the peers list to a 4d array of boardSize x boardSize x numPeers x 2
 */
void initPeers() {
	peers = malloc(boardSize * sizeof(int*));
	// init all 4 dimensions first
	for (int i = 0; i < boardSize; ++i) {
		peers[i] = malloc(boardSize * sizeof(int*));
		for (int r = 0; r < boardSize; ++r) {
			peers[i][r] = malloc(numPeers * sizeof(int*));
			for (int k = 0; k < numPeers; ++k) {
				peers[i][r][k] = malloc(2 * sizeof(int));
			}
		}
	}

	// now add peer row,col pairs to each cell
	for (int row = 0; row < boardSize; ++row) {
		for (int col = 0; col < boardSize; ++col) {
			int peerInd = 0;
			// row/col peers
			for (int k = 0; k < boardSize; ++k) {
				if (k!=row) {
					peers[row][col][peerInd][0] = k;
					peers[row][col][peerInd][1] = col;
					++peerInd;
				}
				if (k!=col) {
					peers[row][col][peerInd][0] = row;
					peers[row][col][peerInd][1] = k;
					++peerInd;
				}
			}

			//region peers
			int regionRow = row-(row%regionSize);
			int regionCol = col-(col%regionSize);
			for (int i = 0; i < regionSize; ++i) {
				for (int r = 0; r < regionSize; ++r) {
					if (regionRow+i == row || regionCol+r == col)
						continue;
					peers[row][col][peerInd][0] = regionRow + i;
					peers[row][col][peerInd][1] = regionCol + r;
					++peerInd;
				}
			}
		}
	}
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
 */
void generateBoard() {
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

	puts("Finished generating board:");
	printBoard();
	puts(boardIsSolved(board) ? "Board passed validation test" : "Board failed validation test");

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

	// print final board output
	puts("Stripped board:");
	printBoard();
}

/**
 * create the board from the data located in the specified file
 * @param fName: the name of the file from which to load the board
 */
void readBoardFromFile(char fName[]) {
	FILE * fp;
	if ((fp = fopen(fName, "r")) == NULL) {
		fprintf(stderr,"Unable to locate file %s\n",fName);
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < boardSize*boardSize; ++i) {
		int t = fscanf(fp,"%d ",&board[i/boardSize][i%boardSize]);
		if (t == 0) {
			fprintf(stderr,"Error reading board data from %s\n",fName);
			exit(EXIT_FAILURE);
		}
	}
	fclose(fp);

	puts("Finished loading board:");
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
	numPeers = 2*(boardSize-1) + regionSize*regionSize - 2*(regionSize-1) - 1;
	initBoard();
	initPeers();

	// rank 0 runs the board generation algorithm
	if (rank == 0) {
		puts("-----Generating board-----");
		fflush(stdout);
		//readBoardFromFile("boardFile.txt");  // *use me to load an existing board for testing / performance analysis
		generateBoard();  // *use me to generate a new board at random
		puts("\n-----Solving Board-----");
		fflush(stdout);
	}
	// rank 0 sends initial board to all other ranks
	MPI_Bcast(&(board[0][0]), boardSize*boardSize, MPI_INT, 0, MPI_COMM_WORLD);

	// analyze solver performance
	double g_start_cycles = GetTimeBase();
	if (serialCPSolver(board)) {  // replace me with your desired solver method
		// rather than bogging down performance with passive recv tests, the first rank to find a solution outputs the result and aborts
		double time_in_secs = (GetTimeBase() - g_start_cycles) / processor_frequency;
		printf("rank %d Solved board (elapsed time %fs):\n",rank, time_in_secs);
		printBoard();
		puts(boardIsSolved(board) ? "Board passed validation test" : "Board failed validation test");
		if (numRanks > 1) MPI_Abort(MPI_COMM_WORLD,1);
	}
	// all done
	MPI_Finalize();
	return EXIT_SUCCESS;
}
