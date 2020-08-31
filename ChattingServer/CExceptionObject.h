#pragma once
class CExceptionObject
{
public:
	CExceptionObject(char* buffer, int bufferSize, const char* data);

	~CExceptionObject();



	int m_BufferSize;

	char m_MessageLog[200];

	char m_ErrorDataLog[200];
};

