// external references to variables and functions defined in the generator
extern const int boardSize;  // size of both board dimensions
extern const int removePercent;  // what percentage of cells to remove for non-evil puzzles
extern int regionSize;
extern int** board;
bool boardIsSolved(int** iBoard);
bool cellIsValid(int row, int col, int** iBoard);
int boardIsFilled(int** iBoard);

void serialBruteForceSolverInternal() {

}

/**
 * solve the current board serially using brute force to solve for missing values
 * @param iBoard: 2d array containing the board data
 */
int** serialBruteForceSolver(int** iBoard) {
	int missingPos = boardIsFilled(iBoard);
	if (missingPos == -1) return iBoard;
	int row = missingPos/boardSize, col = missingPos%boardSize;
	for (int i = 0; i < boardSize; ++i) {
		iBoard[row][col] = i+1;
		if (boardIsSolved(iBoard)) break;
	}
	return iBoard;
}
