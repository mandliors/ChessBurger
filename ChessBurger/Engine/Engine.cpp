#include "Engine.h"
#include "GameData/GameData.h"
#include "Utilities/Utilities.h"
#include <algorithm>
#include <Windows.h>

#ifdef ENGINE_DUMP
	#include <iostream>
#endif

Engine::Engine(const std::string& path, const std::string& name)
	: m_Name(name), m_Working(true), m_Mode(Mode::WAIT), m_WhiteToMove(true), m_AnalysisData({}), m_BestMove(""), m_Position(""), m_hProcess(NULL), m_hThread(NULL), m_PipinW(NULL), m_PipinR(NULL), m_PipoutW(NULL), m_PipoutR(NULL)
{
	SECURITY_ATTRIBUTES securityAttribs = { 0 };
	securityAttribs.nLength = sizeof(securityAttribs);
	securityAttribs.bInheritHandle = TRUE;
	securityAttribs.lpSecurityDescriptor = NULL;

	CreatePipe(&m_PipoutR, &m_PipoutW, &securityAttribs, 0);
	CreatePipe(&m_PipinR, &m_PipinW, &securityAttribs, 0);

	STARTUPINFOA startupInfo = { 0 };
	startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	startupInfo.wShowWindow = SW_HIDE;
	startupInfo.hStdInput = m_PipinR;
	startupInfo.hStdOutput = m_PipoutW;
	startupInfo.hStdError = m_PipoutW;

	PROCESS_INFORMATION processInfo = {0};
	CreateProcessA(NULL, (LPSTR)path.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo);
	m_hProcess = processInfo.hProcess;
	m_hThread = processInfo.hThread;

	m_AnalysisData.BestLines.resize(GameData::EngineLines);
	m_AnalysisData.BestLinesSN.resize(GameData::EngineLines);
	m_AnalysisData.Evaluations.resize(GameData::EngineLines);
}

Engine::~Engine()
{
	Stop();
	_WritePipe("quit");
	m_Working = false;
	if (m_Thread.joinable())
		m_Thread.join();
	_ClosePipe();
}

bool Engine::Init()
{
	_WritePipe("uci");
	if (!_WaitForResponse("uciok", 5000))
	{
		_WritePipe("quit");
		_ClosePipe();
		return false;
	}
	_WritePipe("isready");
	if (!_WaitForResponse("readyok", 5000))
	{
		_WritePipe("quit");
		_ClosePipe();
		return false;
	}
	_WritePipe("setoption name MultiPV value " + std::to_string(GameData::EngineLines));

	m_Thread = std::thread([this]() { this->_Worker(); });
	return true;
}

void Engine::ResetForAnalyzing()
{
	if (m_Mode != Mode::ANALYZE)
	{
		m_Mode = Mode::ANALYZE;
		_WritePipe("ucinewgame");
		_WritePipe("position startpos");
		SetAnalyseMode(true);
	}
}

void Engine::ResetForPlaying()
{
	if (m_Mode != Mode::PLAY)
	{
		m_Mode = Mode::PLAY;
		_WritePipe("ucinewgame");
		_WritePipe("position startpos");
		SetAnalyseMode(false);
	}
}

void Engine::ResetForWaiting()
{
	if (m_Mode != Mode::WAIT)
	{
		m_Mode = Mode::WAIT;
		_WritePipe("stop");
	}
}

void Engine::SetPosition(const std::string& fen)
{
	m_AnalysisData.Depth = 0;
	m_Position = fen;
	m_WhiteToMove = fen.find('w') == -1 ? false : true;
	if (m_Mode == Mode::ANALYZE)
	{
		_WritePipe("stop");
		_WritePipe("position fen " + m_Position);
		_WritePipe("go infinite");
	}
	else
		_WritePipe("position fen " + m_Position);
}

void Engine::SetPositionWithMoves(const std::string& moves)
{
	m_AnalysisData.Depth = 0;
	m_Position = moves;
	m_WhiteToMove = (std::count(moves.begin(), moves.end(), ' ') + 1) % 2;	
	if (m_Mode == Mode::ANALYZE)
	{
		_WritePipe("stop");
		_WritePipe("position startpos moves " + m_Position);
		_WritePipe("go infinite");
	}
	else
		_WritePipe("position startpos moves " + m_Position);
}

void Engine::SetAnalyseMode(bool value)
{
	if (value)
		_WritePipe("setoption name UCI_AnalyseMode value true");
	else
		_WritePipe("setoption name UCI_AnalyseMode value false");
}

void Engine::GoInfinite()
{
	_WritePipe("go infinite");
}

void Engine::SearchMove(uint32_t wtime, uint32_t btime, uint32_t winc, uint32_t binc)
{
	_WritePipe("go wtime " + std::to_string(wtime) + " btime " + std::to_string(btime) + " winc " + std::to_string(winc) + " binc " + std::to_string(binc));
}

void Engine::SendCommand(const std::string& command)
{
	_WritePipe(command);
}

void Engine::Stop()
{
	_WritePipe("stop");
}

std::string Engine::GetName() const
{
	return m_Name;
}

const Engine::AnalysisData& Engine::GetAnalysisData() const
{
	return m_AnalysisData;
}

std::string& Engine::GetBestMove()
{
	return m_BestMove;
}

void Engine::_Worker()
{
	while (m_Working)
	{
		if (m_Mode != Mode::WAIT)
		{
			std::lock_guard<std::mutex> lock(GameData::EngineMutex);
			std::string message = _ReadPipe();
			std::vector<std::string> messageLines = _Split(message, '\n');
#ifdef ENGINE_DUMP
			if (message != "no message")
				std::cout << message;
#endif			
			if (m_Mode == Mode::ANALYZE)
			{
				for (int i = 0; i < messageLines.size(); i++)
				{
					message = messageLines[i];
					if (message.find("currmove") != -1)
						continue;

					//Read depth
					int depthIdx = message.find("depth");
					if (depthIdx != -1)
						m_AnalysisData.Depth = stoi(_GetSubstringUntilChar(message, depthIdx + 6, ' '));

					//Read evaluation and line
					int mpvIdx = message.find("multipv");
					if (mpvIdx != -1)
					{
						int rank = stoi(_GetSubstringUntilChar(message, mpvIdx + 8, ' '));

						//Read evaluation
						int evalIdx = message.find("score cp");
						if (evalIdx != -1)
						{
							float eval = stof(_GetSubstringUntilChar(message, evalIdx + 9, ' ')) / 100.0f * (m_WhiteToMove * 2 - 1);
							m_AnalysisData.Evaluations[rank - 1] = (eval >= 0 ? "+" : "-") + Utils::Round(std::abs(eval), 2);
						}
						else
						{
							evalIdx = message.find("score mate");
							float eval = stoi(_GetSubstringUntilChar(message, evalIdx + 11, ' ')) * (m_WhiteToMove * 2 - 1);
							m_AnalysisData.Evaluations[rank - 1] = (eval >= 0 ? "+M" : "-M") + std::to_string((int)std::abs(eval));
						}

						//Read line
						int pv = message.rfind("pv");
						if (pv != -1)
						{
							std::string line = _GetSubstringUntilChar(message, pv + 3, '\r');
							m_AnalysisData.BestLines[rank - 1] = _Split(line, ' ');
						}
					}
				}
			}
			else if (m_Mode == Mode::PLAY)
			{
				for (int i = 0; i < messageLines.size(); i++)
				{
					message = messageLines[i];
					int bestMoveIdx = message.find("bestmove");
					if (bestMoveIdx == -1)
						continue;
					m_BestMove = _GetSubstringUntilChar(message, bestMoveIdx + 9, ' ');
					if (m_BestMove == "") m_BestMove = _GetSubstringUntilChar(message, bestMoveIdx + 9, '\r');
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Engine::_WritePipe(const std::string& message) const
{
	DWORD write;
#ifdef ENGINE_DUMP
	std::cout << "> " << message << std::endl;
#endif
	WriteFile(m_PipinW, (message + "\n").c_str(), message.size() + 1, &write, NULL);
}

std::string Engine::_ReadPipe() const
{
	BYTE buffer[BUFFER_SIZE];
	DWORD read, bytes;
	std::string message = "";

	PeekNamedPipe(m_PipoutR, buffer, sizeof(buffer), &read, &bytes, NULL);
	if (bytes > 0)
	{
		do
		{
			ZeroMemory(buffer, sizeof(buffer));
			if (!ReadFile(m_PipoutR, buffer, sizeof(buffer), &read, NULL) || !read) break;
			buffer[read > BUFFER_SIZE ? BUFFER_SIZE - 1 : read] = 0;
			message += (char*)buffer;
		} while (read >= sizeof(buffer));

		return message;
	}
	return "no message";
}

bool Engine::_WaitForResponse(const std::string& message, uint32_t maxms) const
{
	std::string response = _ReadPipe();
	clock_t prevTime = clock();

	while (response.find(message) == -1)
	{
		response = _ReadPipe();
		clock_t currentTime = (clock() - prevTime) * 1000 / CLOCKS_PER_SEC;
		if (currentTime > maxms)
			return false;
	}
	return true;
}

std::string Engine::_GetSubstringUntilChar(const std::string& str, int fromIdx, char c) const
{
	if (str.substr(fromIdx, str.size() - fromIdx).find(c) == -1) return "";
	int endIdx = fromIdx;
	while (str[endIdx] != c) endIdx++;
	return str.substr(fromIdx, endIdx - fromIdx);
}

std::vector<std::string> Engine::_Split(std::string line, char c) const
{
	std::vector<std::string> moves;
	moves.reserve(std::count(line.begin(), line.end(), c));

	size_t pos = 0, i = 0;
	while ((pos = line.find(c)) != std::string::npos)
	{
		moves.emplace_back(line.substr(0, pos));
		line.erase(0, pos + 1);
	}
	moves.emplace_back(line);

	return moves;
}

void Engine::_ClosePipe()
{
	if (m_PipinW != NULL) CloseHandle(m_PipinW);
	if (m_PipinR != NULL) CloseHandle(m_PipinR);
	if (m_PipoutW != NULL) CloseHandle(m_PipoutW);
	if (m_PipoutR != NULL) CloseHandle(m_PipoutR);
	if (m_hProcess != NULL) CloseHandle(m_hProcess);
	if (m_hThread != NULL) CloseHandle(m_hThread);
}