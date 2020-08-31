#include "stdafx.h"
#include "CExceptionObject.h"
#include "CMessage.h"

CMessage::CMessage(int iSize)
{
	this->front = -1;
	this->rear = -1;
	this->m_iDataSize = 0;
	this->m_iBufferSize = iSize;
	this->m_buffer = new char[iSize];
}

CMessage::~CMessage()
{
	delete[] this->m_buffer;
}

void CMessage::Release(void)
{
	delete[] this->m_buffer;
}

void CMessage::Clear(void)
{
	this->front = -1;
	this->rear = -1;
	this->m_iDataSize = 0;
}

int CMessage::GetBufferSize(void)
{
	return this->m_iBufferSize;
}

int CMessage::GetDataSize(void)
{
	return this->m_iDataSize - (this->front + 1);
}

char* CMessage::GetBufferPtr(void)
{
	return this->m_buffer;
}

int CMessage::MoveWritePos(int iSize)
{
	this->m_iDataSize += iSize;

	this->rear += iSize;

	return iSize;
}

int CMessage::MoveReadPos(int iSize)
{
	this->m_iDataSize -= iSize;

	this->front += iSize;

	return iSize;
}

int CMessage::GetData(char* chpDest, int iSize)
{
	memcpy(chpDest, &this->m_buffer[this->front + 1], iSize);

	return iSize;
}

int CMessage::PutData(char* chpSrc, int iSize)
{
	
	memcpy(&this->m_buffer[this->rear + 1], chpSrc, iSize);

	return iSize;
}

CMessage& CMessage::operator << (char chValue)
{
	this->m_buffer[this->rear + 1] = chValue;

	this->rear += sizeof(char);

	this->m_iDataSize += sizeof(char);

	return *this;
}

CMessage& CMessage::operator << (unsigned char uchValue)
{
	this->m_buffer[this->rear + 1] = (unsigned char)uchValue;

	this->rear += sizeof(char);

	this->m_iDataSize += sizeof(char);

	return *this;
}

CMessage& CMessage::operator << (short sValue)
{
	short* shortBuffer = (short*)&(this->m_buffer[this->rear + 1]);

	*shortBuffer = sValue;

	this->rear += sizeof(short);

	this->m_iDataSize += sizeof(short);

	return *this;
}

CMessage& CMessage::operator << (unsigned short usValue)
{
	unsigned short* unShortBuffer = (unsigned short*)&(this->m_buffer[this->rear + 1]);

	*unShortBuffer = usValue;

	this->rear += sizeof(short);

	this->m_iDataSize += sizeof(short);

	return *this;
}

CMessage& CMessage::operator << (int iValue)
{
	int* intBuffer = (int*)&(this->m_buffer[this->rear + 1]);

	*intBuffer = iValue;

	this->rear += sizeof(int);

	this->m_iDataSize += sizeof(int);

	return *this;
}

CMessage& CMessage::operator << (unsigned int uiValue)
{
	int* unIntBuffer = (int*)&(this->m_buffer[this->rear + 1]);

	*unIntBuffer = uiValue;

	this->rear += sizeof(int);

	this->m_iDataSize += sizeof(int);

	return *this;
}

CMessage& CMessage::operator << (long lValue)
{
	long* longBuffer = (long*)&(this->m_buffer[this->rear + 1]);

	*longBuffer = lValue;

	this->rear += sizeof(long);

	this->m_iDataSize += sizeof(long);

	return *this;
}

CMessage& CMessage::operator << (unsigned long ulValue)
{

	unsigned long* unLongBuffer = (unsigned long*)&(this->m_buffer[this->rear + 1]);

	*unLongBuffer = ulValue;

	this->rear += sizeof(long);

	this->m_iDataSize += sizeof(long);

	return *this;
}

CMessage& CMessage::operator >> (char& chValue)
{
	if (this->m_iDataSize < sizeof(chValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " char\n");

		throw exception;
	}

	chValue = *((char*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(char);

	this->m_iDataSize -= sizeof(char);

	return *this;
}

CMessage& CMessage::operator >> (unsigned char& uchValue)
{
	if (this->m_iDataSize < sizeof(uchValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " unsigned char\n");

		throw exception;
	}

	uchValue = *((unsigned char*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(char);

	this->m_iDataSize -= sizeof(char);

	return *this;
}

CMessage& CMessage::operator >> (short& sValue)
{

	if (this->m_iDataSize < sizeof(sValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " short\n");

		throw exception;
	}

	sValue = *((short*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(short);

	this->m_iDataSize -= sizeof(short);

	return *this;
}

CMessage& CMessage::operator >> (unsigned short& usValue)
{
	if (this->m_iDataSize < sizeof(usValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " unsigned short\n");

		throw exception;
	}

	usValue = *((unsigned short*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(short);

	this->m_iDataSize -= sizeof(unsigned short);

	return *this;
}

CMessage& CMessage::operator >> (int& iValue)
{

	if (this->m_iDataSize < sizeof(iValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " int\n");

		throw exception;
	}

	iValue = *((int*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(int);

	this->m_iDataSize -= sizeof(int);

	return *this;
}

CMessage& CMessage::operator >> (unsigned int& uiValue)
{
	if (this->m_iDataSize < sizeof(uiValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " unsigned int\n");

		throw exception;
	}

	uiValue = *((unsigned int*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(int);

	this->m_iDataSize -= sizeof(int);

	return *this;
}

CMessage& CMessage::operator >> (long& lValue)
{

	if (this->m_iDataSize < sizeof(lValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " long\n");

		throw exception;
	}

	lValue = *((long*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(long);

	this->m_iDataSize -= sizeof(long);

	return *this;
}

CMessage& CMessage::operator >> (unsigned long& ulValue)
{

	if (this->m_iDataSize < sizeof(ulValue))
	{
		CExceptionObject exception(this->m_buffer, this->rear + 1, " unsigned long\n");

		throw exception;
	}

	ulValue = *((unsigned long*)&(this->m_buffer[this->front + 1]));

	this->front += sizeof(long);

	this->m_iDataSize -= sizeof(long);

	return *this;
}
