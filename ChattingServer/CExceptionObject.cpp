#include "stdafx.h"
#include "CExceptionObject.h"


CExceptionObject::CExceptionObject(char* buffer, int bufferSize, const char* data)
{
	memcpy(this->m_MessageLog, buffer, bufferSize);

	this->m_BufferSize = bufferSize;

	strcpy_s(this->m_ErrorDataLog, data);
}

CExceptionObject::~CExceptionObject()
{

	delete[] this->m_MessageLog;
	delete[] this->m_ErrorDataLog;

}