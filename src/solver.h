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
 * @param adjustedRank: a value propagated through the initial traversal yielding our current rank minus the number of ranks that have claimed a starting location
 * @param adjustedNumRanks: a value propagated through the initial traversal yielding the total number of ranks minus the number of ranks that have claimed a starting location
 * @param startRecursionLayer: counter which keeps track of the recursion layer at which the current rank begins solving, for debugging purposes
 * @returns: whether the current board is solved (true) or not (false)
 */
bool parallelBruteForceSolverInternal(int** iBoard, int adjustedRank, int adjustedNumRanks, int startRecursionLayer) {
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

	// nothing else for this rank to do if it hit a wall before starting
	if (numValidCellValues == 0) {
		return false;
	}

	// now attempt to evenly split up the initial tree traversal by rank
	int cellStartIndex = -1;
	if (numValidCellValues == adjustedNumRanks) {
		// we have the same number of remaining ranks as valid cell values, so we can assign a 1:1 pairing
		cellStartIndex = adjustedRank;
	}
	else if (numValidCellValues > adjustedNumRanks) {
		// we have more valid cell values than remaining ranks, so divide the ranks up as evenly as possible
		cellStartIndex = round(numValidCellValues/adjustedNumRanks)*adjustedRank;
	}
	else {
		// we have more remaining ranks than valid cell values; stop here if our adjusted rank is low enough, otherwise iterate one level deeper
		if (adjustedRank < numValidCellValues) {
			cellStartIndex = adjustedRank;
		}
		else {
			iBoard[row][col] = validCellValues[adjustedRank%numValidCellValues];
			return parallelBruteForceSolverInternal(iBoard, adjustedRank-numValidCellValues, adjustedNumRanks-numValidCellValues, startRecursionLayer+1);
		}
	}
	printf("rank %d: cellStartIndex = %d numValidCellValues = %d startRecursionLayer = %d\n",rank,cellStartIndex,numValidCellValues, startRecursionLayer);

	for (int i = cellStartIndex; i < numValidCellValues; ++i) {
		// now that we've found our parallel initial traversal, we can switch to the serial solver
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
	return parallelBruteForceSolverInternal(iBoard, rank, numRanks, 1);
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
