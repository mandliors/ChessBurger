#pragma once

#include "raylib.h"
#include "Piece/Piece.h"
#include "Guides/Highlight.h"
#include "Guides/Arrow.h"
#include "Guides/Spot.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#define PIECE_SIZE_P 1.0f
#define NAMETAG_HEIGHT_A 60
#define PLAYER_IMAGE_SIZE_A (NAMETAG_HEIGHT_A - 10)
#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

enum class Result
{
	NONE = 0, WHITE_WIN, BLACK_WIN, STALEMATE, DRAW
};

class Game;

class IBoard
{
public:

public:
	IBoard(const Rectangle& bounds, Game* owner);
	~IBoard();

	virtual void Update();
	virtual void UpdateBounds();
	virtual void Draw() const;

	virtual void Reset(bool resetEnginePosition = true);
	bool LoadFEN(const std::string& fen);
	bool LoadPGN(std::string& pgn); //ONLY WITH 1 LINE!!!
	void Flip();
	void Clear();

protected:
	virtual bool _Move(const Vector2& fromSquare, const Vector2& toSquare, bool animated = false, bool updateEnginePosition = true);
	bool _Move(const std::string& move, bool animated = false, bool updateEnginePosition = true);
	void _DoMove(const Vector2& fromSquare, const Vector2& toSquare, bool registerMove = true, bool animated = false, bool playSound = true);
	void _DoMove(const std::string& move, bool registerMove = true, bool animated = false, bool playSound = true);
	bool _TestMove(const Vector2& fromSquare, const Vector2& toSquare, bool* capture = nullptr);
	bool _TestMove(const std::string& move, bool* capture = nullptr);
	void _CastleKing(Piece* king, int8_t direction, bool registerMove = true, bool animated = false, bool playSound = true);
	bool _TestCastle(Piece* king, int8_t direction);
	void _GetLegalMoves(uint8_t side);
	bool _IsAttacked(Piece* piece);
	bool _IsOnBoard(const Vector2& square);
	void _RegisterMove(Piece* piece, const std::string& move, bool capture = false, bool halfMove = false);
	void _ReloadBoard(bool lastMoveVisible);
	std::string _ToChessNote(const Vector2& square) const;
	std::string _GetShortNotation(const std::string& move, bool capture);
	std::string _GetLongNotation(std::string& move);
	Vector2 _ToSquare(const std::string& move) const;
	Vector2 _ToRealSquare(const std::string& move) const;
	Vector2 _GetSquare(const Vector2& position) const;
	Vector2 _GetRealSquare(const Vector2& position) const;
	std::string _MovesToString() const;
	virtual std::string _GetFEN() const;
	virtual std::string _GetPGN() const;

protected:
	Piece m_Board[8][8];
	std::vector<std::string> m_Moves;
	std::vector<std::string> m_MovesSN;
	std::vector<std::string> m_LegalMoves;
	std::vector<Spot> m_SelectedMoves;
	std::string m_StartingFEN;
	uint32_t m_HalfMoveClock;
	int8_t m_Side;
	bool m_AnalyseMode;
	int8_t m_SideToMove;
	Result m_Result;
	std::string m_WhiteName;
	std::string m_BlackName;
	std::string m_EnPassant;
	int32_t m_StartingFEN_Movecount;
	int32_t m_MoveIndex;
	std::vector<Highlight> m_Highlights;
	std::vector<Arrow> m_Arrows;
	Highlight m_SelectionFrom;
	Highlight m_LastMoveFrom;
	Highlight m_LastMoveTo;
	Vector2 m_LeftDragStart;
	Vector2 m_RightDragStart;
	Vector2 m_MouseDownPosition;
	Rectangle m_BoardBounds;
	uint32_t m_SquareSize;
	Piece* m_DraggedPiece;
	Piece* m_SelectedPiece;
	Piece* m_WhiteKing;
	Piece* m_BlackKing;
	bool m_Flipped;
	bool m_PointingHand;
	bool m_WhiteInCheck;
	bool m_BlackInCheck;
	bool m_WhiteShort;
	bool m_WhiteLong;
	bool m_BlackShort;
	bool m_BlackLong;
	bool m_ShowNametag;
	bool m_ShowLegalMoves;
	bool m_OnlyLegalMoves;
	Game* m_Owner;
	static std::unordered_map<PieceType, char> FEN_Codes;
	static std::unordered_map<char, PieceType> FEN_Codes_Backwards;
};