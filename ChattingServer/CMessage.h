#pragma once
class CMessage
{
public:
	CMessage(int iSize = 3000);

	virtual ~CMessage();

	void         Release(void);

	void         Clear(void);

	int          GetBufferSize(void);

	int          GetDataSize(void);

	char* GetBufferPtr(void);

	int          MoveWritePos(int iSize);

	int          MoveReadPos(int iSize);

	int          GetData(char* chpDest, int iSize);

	int          PutData(char* chpSrc, int iSize);

	CMessage& operator << (char chValue);

	CMessage& operator << (unsigned char uchValue);

	CMessage& operator << (short sValue);
	CMessage& operator << (unsigned short usValue);

	CMessage& operator << (int iValue);
	CMessage& operator << (unsigned int uiValue);

	CMessage& operator << (long lValue);
	CMessage& operator << (unsigned long lValue);


	CMessage& operator >> (char& chValue);
	CMessage& operator >> (unsigned char& uchValue);

	CMessage& operator >> (short& sValue);
	CMessage& operator >> (unsigned short& usValue);

	CMessage& operator >> (int& iValue);
	CMessage& operator >> (unsigned int& uiValue);

	CMessage& operator >> (long& lValue);
	CMessage& operator >> (unsigned long& ulValue);



protected:

	int front;

	int rear;

	int m_iBufferSize;

	int m_iDataSize;

	char* m_buffer;

};

