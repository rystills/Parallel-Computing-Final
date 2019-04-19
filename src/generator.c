#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define boardSize 16  // size of both board dimensions
int regionSize;
int** board;

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
	regionSize = sqrt(boardSize);
	initBoard();
	generateBoard(true);
	printBoard();
	return EXIT_SUCCESS;
}
