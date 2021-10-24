#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>

class Engine
{
public:
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
	void Start();
	void SetPosition(const std::string& fen);
	void SetPositionWithMoves(const std::string& moves);
	void SetAnalyseMode(bool value);
	void SendCommand(const std::string& command);
	void Stop();
	
	std::string GetName() const;
	const AnalysisData& GetAnalysisData() const;

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
	bool m_Read;
	bool m_WhiteToMove;
	AnalysisData m_AnalysisData;
	std::thread m_Thread;
	std::string m_Position;
	void* m_hProcess;
	void* m_hThread;
	void* m_PipinW;
	void* m_PipinR;
	void* m_PipoutW;
	void* m_PipoutR;
};