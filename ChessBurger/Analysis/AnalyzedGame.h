#pragma once

#include "AnalyzedMove.h"
#include <vector>

class AnalyzedGame
{
public:
	AnalyzedGame(int moveCount);
	~AnalyzedGame() { }

	void SetMove(int moveIdx, const AnalyzedMove& bestMove, const AnalyzedMove& move);
	std::pair<AnalyzedMove, AnalyzedMove> GetMove(int moveIdx);

private:
	std::vector<AnalyzedMove> m_BestMoves;
	std::vector<AnalyzedMove> m_Moves;
};