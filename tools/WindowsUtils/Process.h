#pragma once

class CProcess
{
	void* m_handle;
public:
	CProcess();
	~CProcess();

	bool spawn(const char* commandLine, bool bAwait= false);
	bool isRunning();
};