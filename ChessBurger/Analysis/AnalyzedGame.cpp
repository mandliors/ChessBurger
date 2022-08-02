#include "AnalyzedGame.h"

AnalyzedGame::AnalyzedGame(int moveCount)
	: m_Moves({}), m_BestMoves({})
{
	m_BestMoves.resize(moveCount);
	m_Moves.resize(moveCount);
}

void AnalyzedGame::SetMove(int moveIdx, const AnalyzedMove& bestMove, const AnalyzedMove& move)
{
	m_BestMoves[moveIdx] = bestMove;
	m_Moves[moveIdx] = move;
}

std::pair<AnalyzedMove, AnalyzedMove> AnalyzedGame::GetMove(int moveIdx)
{
	return std::make_pair(m_BestMoves[moveIdx], m_Moves[moveIdx]);
}
