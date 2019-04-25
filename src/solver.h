// external references to variables and functions defined in the generator
extern const int boardSize;  // size of both board dimensions
extern const int removePercent;  // what percentage of cells to remove for non-evil puzzles
extern int regionSize;
extern int** board;
extern int numRanks;
extern int rank;
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
 * solve the specified board serially using brute force to determine missing values.
 * @param iBoard: 2d array containing the board data
 */
void serialBruteForceSolver(int** iBoard) {
	serialBruteForceSolverInternal(iBoard);
}

/**
 * core recursive internal function for parallel brute force solver; recursively fills in cell values.
 * @param iBoard: 2d array containing the board data
 * @returns: whether the current board is solved (true) or not (false)
 */
bool parallelBruteForceSolverInternal(int** iBoard) {
	// get location of unfilled cell
	int missingPos = boardIsFilled(iBoard);

	// base case: board is full and solved
	if (missingPos == -1 && boardIsSolved(iBoard)) return true;
	int row = missingPos/boardSize, col = missingPos%boardSize;

	// first gather a list of all valid cells at this recursion level
	int validCellValues[boardSize];
	int numValidCellValues = 0;
	for (int i = 1; i <= boardSize; ++i) {
		iBoard[row][col] = i;
		if (cellIsValid(row,col,iBoard))
			validCellValues[numValidCellValues++] = i;
	}

	// now attempt to evenly split up the initial tree traversal by rank
	int cellStartIndex = -1;
	if (numValidCellValues == numRanks) {
		cellStartIndex = rank;
	}
	else if (numValidCellValues > numRanks) {
		cellStartIndex = round(numValidCellValues/numRanks)*rank;
	}
	else {
		//todo: traverse the tree one level deeper and try again
	}
	printf("rank %d: cellStartIndex = %d numValidCellValues = %d\n",rank,cellStartIndex,numValidCellValues);

	for (int i = cellStartIndex; i < numValidCellValues; ++i) {
		//now that we've found our parallel initial traversal, we can switch to the serial solver
		iBoard[row][col] = validCellValues[i];
		if (serialBruteForceSolverInternal(iBoard))
			return true;
	}

	iBoard[row][col] = 0;
	return false;
}

/**
 * solve the specified board in parallel using brute force to determine missing values.
 * @param iBoard: 2d array containing the board data
 * @returns whether this rank found a solution (true) or not (false)
 */
bool parallelBruteForceSolver(int** iBoard) {
	return parallelBruteForceSolverInternal(iBoard);
}

/**
 * solve the specified board serially using constraint propagation to determine missing values.
 * @param iBoard: 2d array containing the board data
 */
void serialCPSolver(int** iBoard) {

}

/**
 * solve the specified board in parallel using constraint propagation to determine missing values.
 * @param iBoard: 2d array containing the board data
 */
void parallelCPSolver(int** iBoard) {

}
