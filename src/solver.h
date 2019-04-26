// external references to variables and functions defined in the generator
extern const int boardSize;  // size of both board dimensions
extern const int removePercent;  // what percentage of cells to remove for non-evil puzzles
extern int regionSize;
extern int** board;
extern int**** peers;
extern int numRanks;
extern int rank;
extern int numPeers;
bool boardIsSolved(int** iBoard);
bool cellIsValid(int row, int col, int** iBoard);
int boardIsFilled(int** iBoard);
int ***alloc_3d_int(int x, int y, int z);
void dealloc_3d_int(int x, int ***arr);

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
bool serialBruteForceSolver(int** iBoard) {
	return serialBruteForceSolverInternal(iBoard);
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
 * remove the specified value from the possibleValues list for the cell at peerRow,peerCol
 * @param possibleValues: the full possibleValues array
 * @param peerRow: the row of the cell from whom we wish to remove the possible value
 * @param peerCol: the column of the cell from whom we wish to remove the possible value
 * @param removeVal: the value we wish to remove
 * @returns: whether the value was found and removed (true) or not (false)
 */
bool removePossibleValue(int*** possibleValues, int peerRow, int peerCol, int removeVal) {
	for (int r = 0; r < boardSize && possibleValues[peerRow][peerCol][r] != 0; ++r) {
		if (possibleValues[peerRow][peerCol][r] == removeVal) {
			// value found; remove it and shift remaining values left
			for (int k = r+1; k < boardSize && possibleValues[peerRow][peerCol][k-1] !=0; ++k)
				possibleValues[peerRow][peerCol][k-1] = possibleValues[peerRow][peerCol][k];
			possibleValues[peerRow][peerCol][boardSize-1] = 0;
			return true;
		}
	}
	return false;
}

/**
 * determine whether or not any cells have more than one remaining possible value
 * @param possibleValues: the full possibleValues array
 * @returns whether at least one cell has more than one remaining possible value (true) or not (false)
 */
bool possibilitiesRemain(int*** possibleValues) {
	for (int i = 0; i < boardSize; ++i) {
		for (int r = 0; r < boardSize; ++r) {
			if (possibleValues[i][r][1] != 0) return true;
		}
	}
	return false;
}

/**
 * check if the specified value is in the missing values array. If so, remove the value and shift the remaining values down, terminating with 0.
 * @param val: the value to remove from the array, if present
 * @param arr: the missing values array from which to remove the specified value
 * @param arrLen: the length of the missing values array (not accounting for 0's)
 * @returns: whether the value was found in and removed from the missing values array (true) or not (false)
 */
bool checkRemoveValFromMissingList(int val, int* arr, int arrLen) {
	for (int i = 0; i < arrLen && arr[i] != 0; ++i) {
		if (val == arr[i]) {
			// value found: remove it and shift remaining values down
			for (int k = i+1; k < arrLen && arr[k-1] !=0; ++k)
				arr[k-1] = arr[k];
			arr[arrLen-1] = 0;
			return true;
		}
	}
	return false;
}

/**
 * copy all possible values from pva to pvb
 * @param pva: possible values list to copy from
 * @param pvb: possible values list to copy to
 */
void copyPossibleValues(int*** pva, int*** pvb) {
	for (int row = 0; row < boardSize; ++row) {
		for (int col = 0; col < boardSize; ++col) {
			for (int i = 0; i < boardSize; ++i) {
				pvb[row][col][i] = pva[row][col][i];
			}
		}
	}
}

/**
 * copy all cell single possiblities to iBoard
 * @param iBoard: 2d array containing the board data
 * @param possibleValues: the full possibleValues array
 */
void copyPossibilitiesToBoard(int** iBoard, int*** possibleValues) {
	for (int row = 0; row < boardSize; ++row) {
		for (int col = 0; col < boardSize; ++col) {
			iBoard[row][col] = possibleValues[row][col][0];
		}
	}
}

/**
 * core recursive internal function for serial constraint propagation solver; recursion branches each time CP can't reduce any further.
 * @param iBoard: 2d array containing the board data
 * @param possibleValues: the full possibleValues array
 * @returns: whether this branch led to a solution (true) or not (false)
 */
bool serialCPSolverInternal(int** iBoard, int*** possibleValues) {
	// run constraint propagation until we solve the board
	bool createdNewSingleton = true;
	while (createdNewSingleton && possibilitiesRemain(possibleValues)) {
		createdNewSingleton = false;
		for (int row = 0; row < boardSize; ++row) {
			for (int col = 0; col < boardSize; ++col) {
				// skip cells that are already completed
				if (possibleValues[row][col][1] == 0)
					continue;
				// apply CP rule 1 (remove peer known values from the current cell's possibility values list)
				for (int i = 0; i < numPeers; ++i) {
					int peerRow = peers[row][col][i][0], peerCol = peers[row][col][i][1];
					if (possibleValues[peerRow][peerCol][1] == 0) {
						removePossibleValue(possibleValues, row, col, possibleValues[peerRow][peerCol][0]);
					}
				}
				// apply CP rule 2 (choose value if all peers have removed it from their possibility list)
				// start with an array considering all values as missing
				int peersMissingValues[boardSize];
				for (int i = 0; i < boardSize; ++i)
					peersMissingValues[i] = i+1;

				// for each peer, take values out of the missing array if that peer isn't missing the value
				for (int i = 0; i < numPeers; ++i) {
					int peerRow = peers[row][col][i][0], peerCol = peers[row][col][i][1];
					// iterate over all possible values in current peer
					for (int r = 0; r < boardSize && possibleValues[peerRow][peerCol][r] != 0; ++r) {
						checkRemoveValFromMissingList(possibleValues[peerRow][peerCol][r],peersMissingValues,boardSize);
						if (peersMissingValues[0] == 0) break;
					}
					if (peersMissingValues[0] == 0) break;
				}
				// we found a value that was missing in all of our peers, so it must be our value
				if (peersMissingValues[0] != 0) {
					possibleValues[row][col][0] = peersMissingValues[0];
					possibleValues[row][col][1] = 0;
					// we created a new singleton (cell with only 1 possible value), so we can keep running CP
					createdNewSingleton = true;
				}
			}
		}
	}

	// if we have reduced all cell possibilities to singletons, we have either a solution or a contradiction
	if (!possibilitiesRemain(possibleValues)) {
		copyPossibilitiesToBoard(iBoard,possibleValues);
		return boardIsSolved(iBoard);
	}

	// if any cells have no possibilities, we've reached a contradiction
	for (int row = 0; row < boardSize; ++row) {
		for (int col = 0; col < boardSize; ++col) {
			if (possibleValues[row][col][0] == 0)
				return false;
		}
	}

	// find the cell with the fewest possibilities
	int fewestRow = -1, fewestCol = -1;
	int fewestPossibilities = boardSize+1;
	for (int row = 0; row < boardSize; ++row) {
		for (int col = 0; col < boardSize; ++col) {
			// possible candidate
			if (possibleValues[row][col][1] != 0) {
				int curPossibilities = 0;
				for (int k = 0; k < boardSize && possibleValues[row][col][k] != 0; ++k, ++curPossibilities);
				// found a new cell with the fewest possibilities
				if (curPossibilities < fewestPossibilities) {
					fewestPossibilities = curPossibilities;
					fewestRow = row;
					fewestCol = col;
				}
			}
		}
	}
	// copy the full possibilities list as we might have to undo future decisions if this branch is unsuccessful
	int*** possibleValuesCopy = alloc_3d_int(boardSize,boardSize,boardSize);
	copyPossibleValues(possibleValues, possibleValuesCopy);
	// recurse on the cell with the fewest possibilities for each potential possibility
	for (int i = 0; i < fewestPossibilities; ++i) {
		possibleValues[fewestRow][fewestCol][0] = possibleValuesCopy[fewestRow][fewestCol][i];
		possibleValues[fewestRow][fewestCol][1] = 0;
		if (serialCPSolverInternal(iBoard, possibleValues)) {
			dealloc_3d_int(boardSize, possibleValuesCopy);
			return true;
		}
		// branch was unsuccessful; revert possible values and try the next branch
		copyPossibleValues(possibleValuesCopy, possibleValues);
	}

	// all branches failed; a previous guess must have been wrong
	dealloc_3d_int(boardSize, possibleValuesCopy);
	return false;
}

/**
 * solve the specified board serially using constraint propagation to determine missing values.
 * @param iBoard: 2d array containing the board data
 */
bool serialCPSolver(int** iBoard) {
	// init possibility values for each cell
	int*** possibleValues = alloc_3d_int(boardSize,boardSize,boardSize);
	for (int i = 0; i < boardSize; ++i) {
		for (int r = 0; r < boardSize; ++r) {
			// current cell is unknown: start will all possible values
			if (iBoard[i][r] == 0) {
				for (int k = 0; k < boardSize; ++k) {
					possibleValues[i][r][k] = k+1;
				}
			}
			// current cell is known: start only with the given value
			else {
				possibleValues[i][r][0] = iBoard[i][r];
				possibleValues[i][r][1] = 0;
			}
		}
	}

	// run the core recursive CP solver method
	serialCPSolverInternal(iBoard, possibleValues);

	// apply resulting values to iBoard
	copyPossibilitiesToBoard(iBoard, possibleValues);
	return true;
}

/**
 * solve the specified board in parallel using constraint propagation to determine missing values.
 * @param iBoard: 2d array containing the board data
 */
bool parallelCPSolver(int** iBoard) {
	return true;
}
