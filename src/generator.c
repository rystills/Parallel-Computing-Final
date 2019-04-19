#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define boardSize 9  // size of both board dimensions
int regionSize;
int** board;

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
	board = malloc(boardSize * sizeof(int*));
	for (int i = 0; i < boardSize; ++i) {
		board[i] = malloc(boardSize * sizeof(int));
		for (int r = 0; r < boardSize; ++r) {
			board[i][r] = 0;
		}
	}
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
	// randomly swap boardSize^2 number pairs
	for (int i = 0; i < boardSize*boardSize; ++i) {
		int r1 = randInt(1,boardSize);
		int r2 = r1;
		while (r2 == r1) r2 = randInt(1,boardSize);
		swapDigits(r1, r2);
	}

	// randomly swap boardSize row and column pairs within regions
	for (int i = 0; i < boardSize; ++i) {
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
 * @returns: whether the board is solved (true) or unsolved (false)
 */
bool isSolved() {
	// check for row/col duplicates
	for (int i = 0; i < boardSize; ++i)
		for (int r = 0; r < boardSize; ++r)
			for (int k = 0; k < boardSize; ++k)
				if ((board[i][k] == board[i][r] && k != r) || (board[k][r] == board[i][r] && k != i)) return false;

	// check for region duplicates
	for (int i = 0; i < regionSize; ++i) {
		for (int r = 0; r < regionSize; ++r) {
			int regionVals[boardSize];
			for (int j = 0; j < regionSize; ++j) {
				for (int k = 0; k < regionSize; ++k) {
					insertInPlace(regionVals, board[i*regionSize + j][r*regionSize + k], j*regionSize+k);
				}
			}
			for (int i = 1; i < boardSize; ++i)
				if (regionVals[i] != regionVals[i-1]+1) return false;
		}
	}
	return true;
}

/**
 * output the board in ascii form
 */
void printBoard() {
	int boardSizeDigits = log10(boardSize) + 1;
	for (int i = 0; i < boardSize; ++i) {
		putchar('|');
		for (int r = 0; r < boardSize; ++r) {
			// draw blank
			if (board[i][r] == -1)
				for (int j = 0; j < boardSizeDigits; ++j) putchar(' ');
			else {
				// draw given
				int curDigits = log10(board[i][r]) + 1;
				for (int j = 0; j < boardSizeDigits - curDigits; ++j) putchar(' ');
				printf("%d", board[i][r]);
			}
			putchar('|');
		}
		putchar('\n');
	}
}

int main(void) {
	srand(time(0));
	regionSize = sqrt(boardSize);
	initBoard();
	generateBoard(true);
	printBoard();
	puts(isSolved() ? "solved" : "unsolved");
	return EXIT_SUCCESS;
}
