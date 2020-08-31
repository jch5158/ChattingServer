#include "stdafx.h"
#include "CExceptionObject.h"
#include "CMessage.h"
#include "CRingBuffer.h"


CRingBuffer::CRingBuffer(int queueLen)
{
	this->mRingBuffer = new char[queueLen];
	
	this->mQueueLen = queueLen;
	this->front = 0;
	this->rear = 0;
}

CRingBuffer::~CRingBuffer(void)
{
//	delete[] this->mRingBuffer;
}

bool CRingBuffer::IsEmpty(void)
{
	if (front == rear)
	{
		return true;
	}

	return false;
}



// 현재 사용중인 용량 얻기, Return : 사용중인 용량
int CRingBuffer::GetUseSize(void)
{
	// 정확함
	if (this->rear >= this->front)
	{
		return this->rear - this->front;
	}
	else
	{
		return (this->mQueueLen - this->front) + this->rear;
	}

}

// 현재 버퍼에 남은 용량 얻기, Return : 남은 용량
int CRingBuffer::GetFreeSize(void)
{

	if (this->rear >= this->front)
	{
		return (this->mQueueLen - this->rear) + this->front - 1;
	}
	else
	{
		return this->front - this->rear - 1;
	}
}

/////////////////////////////////////////////////////////////////////////
// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
// (끊기지 않은 길이)
//
// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
//
// Parameters: 없음.
// Return: (int)사용가능 용량.
////////////////////////////////////////////////////////////////////////
int CRingBuffer::DirectEnqueueSize(void)
{

	// 
	if (this->rear == this->mQueueLen - 1 && this->front == this->mQueueLen - 1)
	{
		return this->mQueueLen - 1;
	}
	else if (this->rear == this->mQueueLen - 1)
	{
		return this->front;
	}
	else if (this->rear >= this->front)
	{
		return this->mQueueLen - this->rear - 1;
	}
	else
	{
		return this->front - this->rear - 1;
	}


	//int queueLen = this->mQueueLen;


	//return queueLen - this->rear - 1;

}

int CRingBuffer::DirectDequeueSize(void)
{
	if (this->front == this->mQueueLen - 1 && this->rear < this->front)
	{
		return this->rear;
	}

	else if (this->rear >= this->front)
	{
		return  this->rear - this->front;
	}
	else
	{
		return this->mQueueLen - this->front - 1;
	}
}

/////////////////////////////////////////////////////////////////////////
// rear 에 데이타 넣음.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)넣은 크기.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Enqueue(char* chpData, int iSize)
{
	int queueLen = this->mQueueLen;

	//int freeSize = GetFreeSize();

	//if (iSize > freeSize)
	//{
	//	int* ptr = nullptr;

	//	*ptr = 10;
	//	iSize = freeSize;
	//}

	int sizeCheck = this->rear + iSize;

	if (sizeCheck >= queueLen)
	{
		int directSize = queueLen - this->rear - 1;

		memcpy(&this->mRingBuffer[this->rear + 1], chpData, directSize);

		int addSize = iSize - directSize;

		memcpy(&this->mRingBuffer[(this->rear + directSize + 1) % queueLen], &chpData[directSize], addSize);
	}
	else
	{
		memcpy(&this->mRingBuffer[this->rear + 1], chpData, iSize);

	}

	this->rear = (this->rear + iSize) % queueLen;


	return iSize;
}



/////////////////////////////////////////////////////////////////////////
// front 에서 데이타 가져옴. ReadPos 이동.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Dequeue(char* chpDest, int iSize)
{
	int queueLen = this->mQueueLen;

	/*int useSize = GetUseSize();

	if (iSize > useSize)
	{
		int* ptr = nullptr;
		*ptr = 10;
		iSize = useSize;
	}*/

	int sizeCheck = this->front + iSize;

	if (sizeCheck >= queueLen)
	{
		int directSize = queueLen - this->front - 1;

		memcpy(chpDest, &this->mRingBuffer[this->front + 1], directSize);

		int addSize = iSize - directSize;

		memcpy(&chpDest[directSize], &this->mRingBuffer[(this->front + directSize + 1) % queueLen], addSize);
	}
	else
	{
		memcpy(chpDest, &this->mRingBuffer[this->front + 1], iSize);

	}

	this->front = (this->front + iSize) % queueLen;

	return iSize;
}




/////////////////////////////////////////////////////////////////////////
// front 에서 데이타 읽어옴. ReadPos 고정.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Peek(char* chpDest, int iSize)
{
	int useSize = GetUseSize();

	int cFront = this->front;

	int queueLen = this->mQueueLen;

	/*
	if (iSize > useSize)
	{
		int* ptr = nullptr;
		*ptr = 10;
		iSize = useSize;
	}*/


	int sizeCheck = cFront + iSize;

	if (sizeCheck >= queueLen)
	{
		int directSize = queueLen - cFront - 1;

		memcpy(chpDest, &this->mRingBuffer[cFront + 1], directSize);

		cFront = cFront + directSize;

		int addSize = iSize - directSize;

		memcpy(&chpDest[directSize], &this->mRingBuffer[(cFront + 1) % queueLen], addSize);

		//cFront = (cFront + addSize) % queueLen;
	}
	else
	{
		memcpy(chpDest, &this->mRingBuffer[cFront + 1], iSize);

		//cFront = cFront + iSize;
	}

	return iSize;
}


/////////////////////////////////////////////////////////////////////////
// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
//
// Parameters: 없음.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////
void CRingBuffer::MoveRear(int iSize)
{
	this->rear = (this->rear + iSize) % this->mQueueLen;

	return;
}



void CRingBuffer::MoveFront(int iSize)
{
	int queueLen = this->mQueueLen;
	
	this->front = (this->front + iSize) % this->mQueueLen;

	return;
}



/////////////////////////////////////////////////////////////////////////
// 버퍼의 모든 데이타 삭제. rear랑 front만 조절하자.
//
// Parameters: 없음.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////
void CRingBuffer::ClearBuffer(void)
{
	this->front = 0;
	this->rear = 0;
}


/////////////////////////////////////////////////////////////////////////
// 버퍼의 Front 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetFrontBufferPtr(void)
{
	return &this->mRingBuffer[(this->front + 1) % this->mQueueLen];
}


/////////////////////////////////////////////////////////////////////////
// 버퍼의 RearPos 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetRearBufferPtr(void)
{
	return &this->mRingBuffer[(this->rear + 1) % this->mQueueLen];
}




void CRingBuffer::Release(void)
{
	delete[] this->mRingBuffer;
}
