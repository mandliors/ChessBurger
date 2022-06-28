#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>

#define BUFFER_SIZE 8192
//#define ENGINE_DUMP 1

class Engine
{
public:
	enum class Mode
	{
		WAIT = 0, ANALYZE, PLAY
	};
	struct AnalysisData
	{
		std::vector<std::vector<std::string>> BestLines;
		std::vector<std::vector<std::string>> BestLinesSN;
		std::vector<std::string> Evaluations;
		uint32_t Depth;
	};

public:
	Engine(const std::string& path, const std::string& name);
	~Engine();

	bool Init();
	void ResetForAnalyzing();
	void ResetForPlaying();
	void ResetForWaiting();
	void SetPosition(const std::string& fen);
	void SetPositionWithMoves(const std::string& moves);
	void SetAnalyseMode(bool value);
	void GoInfinite();
	void SearchMove(uint32_t wtime, uint32_t btime, uint32_t winc, uint32_t binc);
	void SendCommand(const std::string& command);
	void Stop();
	
	std::string GetName() const;
	const AnalysisData& GetAnalysisData() const;
	std::string& GetBestMove();

private:
	void _Worker();
	void _WritePipe(const std::string& message) const;
	std::string _ReadPipe() const;
	bool _WaitForResponse(const std::string& message, uint32_t maxms) const;
	std::string _GetSubstringUntilChar(const std::string& str, int fromIdx, char c) const;
	std::vector<std::string> _Split(std::string line, char c) const;
	void _ClosePipe();

private:
	std::string m_Name;
	bool m_Working;
	Mode m_Mode;
	bool m_WhiteToMove;
	AnalysisData m_AnalysisData;
	std::string m_BestMove;
	std::thread m_Thread;
	std::string m_Position;
	void* m_hProcess;
	void* m_hThread;
	void* m_PipinW;
	void* m_PipinR;
	void* m_PipoutW;
	void* m_PipoutR;
};