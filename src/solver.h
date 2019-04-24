// external references to variables and functions defined in the generator
extern const int boardSize;  // size of both board dimensions
extern const int removePercent;  // what percentage of cells to remove for non-evil puzzles
extern int regionSize;
extern int** board;
bool boardIsSolved(int** iBoard);
bool cellIsValid(int row, int col, int** iBoard);
int boardIsFilled(int** iBoard);

/**
 * core recursive internal function for serial brute force solver; recursively fills in cell values
 * @param iBoard: 2d array containing the board data
 * @returns: whether the current board is solved (true) or not (false)
 */
bool serialBruteForceSolverInternal(int** iBoard) {
	// get location of unfilled cell
	int missingPos = boardIsFilled(iBoard);
	// base case: board is full and solved
	if (missingPos == -1 && boardIsSolved(iBoard)) return true;
	int row = missingPos/boardSize, col = missingPos%boardSize;

	// recursively iterate through possible values for unfilled cell
	for (int i = 1; i <= boardSize; ++i) {
		iBoard[row][col] = i;
		if (cellIsValid(row,col,iBoard) && serialBruteForceSolverInternal(iBoard)) return true;
	}
	iBoard[row][col] = 0;
	return false;
}

/**
 * solve the current board serially using brute force to solve for missing values
 * @param iBoard: 2d array containing the board data
 */
int** serialBruteForceSolver(int** iBoard) {
	serialBruteForceSolverInternal(iBoard);
	return iBoard;
}
