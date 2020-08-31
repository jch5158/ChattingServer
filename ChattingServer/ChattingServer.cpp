#include "stdafx.h"
#include "Protocol.h"
#include "CExceptionObject.h"
#include "CMessage.h"
#include "CRingBuffer.h"

#define SERVERPORT 6000
#define SERVERIP L"127.0.0.1"

#define FD_SET_SIZE 64


int gRecvSize;
int gSendSize;


struct Session
{
	SOCKET mClientSock;

	SOCKADDR_IN mClientAddr;
	
	CRingBuffer mRecvQ;

	CRingBuffer mSendQ;

	DWORD mUserCode;

	// 방에 들어가 있을 경우 RoomNumber
	DWORD mEnterRoomNumber;

	WCHAR mUserName[dfNICK_MAX_LEN];
};

struct ChatRoom
{
	DWORD mRoomNumber;
	WCHAR mRoomTitle[256];
	std::list<DWORD> mUserList;
};


SOCKET gListenSock;

std::unordered_map<DWORD, DWORD> gConnectList;

std::map<DWORD, Session*> gSessionList;

std::map<DWORD, ChatRoom*> gChatRoomList;

DWORD gUserCode = 1;
DWORD gChatRoomNumber = 1;



//===================================================================
//
// 리슨 소켓의 초기 설정
// WSADATA 2.2 버젼 설정, 넌블럭 전환, RST linger 옵션, bind, linsten 
//====================================================================
bool SettingListenSocketOption();

void NetworkProc();

void Accept();

void Disconnect(DWORD clientKey);

void noLoginUserDisconnect();

void SendEvent(DWORD clientKey);

void RecvEvent(DWORD clientKey);

void RecvPacketProc(Session* session, WORD msgType, CMessage* message);

void SelectSocket(DWORD* clientKeyTable, SOCKET* clientSockTable, FD_SET* readSet, FD_SET* writeSet);


void SendUnicasting(Session* session, PacketHeader *packetHeader, CMessage* message);

void SendBroadcasting(PacketHeader *packetHeader, CMessage* message);

void SendRoomBroadcasting(ChatRoom *chatRoom, Session* session,PacketHeader *packetHeader, CMessage* message);


Session* FindClient(DWORD clientCode);

ChatRoom* FindChatRoom(DWORD roomNumber);

bool FindNoLoginClient(DWORD clientCode);



bool CompleteRecvPacket(Session* session);

// 로그인  request
void LoginPacketProcessing(Session* session, CMessage* message);

// 로그인 패킷 만들기
void PackingResLogin(Session* session,PacketHeader* packetHeader,BYTE result, CMessage* message);

// 생성한 로그인 패킷 send 링 버퍼에 인큐하기
void SendPacketResLogin(Session* session, BYTE result);



// RoomList request
void RoomListPacketProcessing(Session* session, CMessage* message);

// 대화방 목록 패킷 만들기
void PackingResRoomList(Session* session, PacketHeader* packetHeader, CMessage* message);

// 생성한 룸 리스트 패킷 send 링 버퍼에 인큐하기
void SendPacketRoomList(Session* session);



// 대화방 생성 request
void CreateRoomPacketProcessing(Session* session, CMessage* message);


// 대화방 생성 패킷 만들기 
void PackingResCreateRoom(ChatRoom* chatRoom, BYTE result,PacketHeader* packetHeader ,CMessage* message);


// 대화방 생성 response 링버퍼 인큐  ( 수시로 브로드캐스트 )
void SendPacketCreateRoom(Session* session, ChatRoom* chatRoom, BYTE result);




// 대화방 입장 request
void EnterRoomPacketProcessing(Session* session, CMessage* message);


// 대화방 입장 패킷 만들기
void PackingResEnterRoom(Session* session, ChatRoom* chatRoom, BYTE result,PacketHeader* packetHeader ,CMessage* message);

// 대화방 입장 패킷 send response
void SendPakcetEnterRoom(Session* session, ChatRoom* chatRoom, BYTE result);


// 채팅 보내기 request
void ChatMsgPacketProcessing(Session* session, CMessage* message);

// 채팅 메시지 ( 룸 있는지 확인 후 입장 보낸 사람은 제외 후 브로드캐스팅)
void PackingResChatMessage(Session *session, WCHAR *wStr,PacketHeader* packetHeader, CMessage* message);

// 채팅 메시지 룸 브로드캐스팅 
void SendChatMessage(Session* session, ChatRoom* chatRoom, WCHAR* wStr);



// 대화방 나가기 send response
void LeaveRoomPacketProcessing(Session* session, CMessage* message);

// 대화방 나가기 ( 수시로 방 유저들 브로드캐스팅 )
void PackingResRoomLeave(Session* session, PacketHeader* packetHeader,CMessage* message);

// 대화방 나가기 send request
void SendRoomLeave(Session* session,ChatRoom* chatRoom,bool roomDeleteFlag);


// 대화방 삭제 ( 수시로 방 삭제 브로드 캐스팅 )
// 대화방의 유저가 0명이면은 방 삭제
void PackingResRoomDelete(Session* session, ChatRoom* chatRoom, PacketHeader* packetHeader, CMessage* message);

// 새로운 유저 대화방 입장 ( 수시로 방 있는 사람들 브로드캐스팅 )
void PackingResNewUserEnter(Session* session, PacketHeader* packetHeader, CMessage* message);



void StressTestProcessing(Session* session, CMessage* message);

void PackingStressTest(Session* session, WCHAR* wStr, unsigned short strSize,PacketHeader* packetHeader, CMessage* message);

void SendStressTest(Session* session, WCHAR* wStr, unsigned short strSize);


BYTE MakeCheckSum(WORD msgType, CMessage* message);


int wmain()
{
	setlocale(LC_ALL, "");

	timeBeginPeriod(1);

	int retval;

	if (!SettingListenSocketOption())
	{
		return -1;
	}

	
	while (1)
	{

		NetworkProc();

	}


	WSACleanup();

	timeEndPeriod(1);

	return 0;
}



bool SettingListenSocketOption()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		wprintf_s(L"%d\n", WSAGetLastError());
		return false;
	}

	int retval;

	gListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (gListenSock == INVALID_SOCKET)
	{
		wprintf_s(L"Socket Create Error : %d\n", WSAGetLastError());
		return false;
	}

	unsigned long on = 1;
	retval = ioctlsocket(gListenSock, FIONBIO, &on);
	if (retval == SOCKET_ERROR)
	{
		wprintf_s(L"Non Blacking Error : %d\n", WSAGetLastError());
		return false;
	}


	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	retval = setsockopt(gListenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (retval == SOCKET_ERROR)
	{
		wprintf_s(L"Setsockopt Error : %d\n", WSAGetLastError());
		return false;
	}


	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVERPORT);
	InetPtonW(AF_INET, SERVERIP, &serverAddr.sin_addr);

	retval = bind(gListenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		wprintf_s(L"bind error %d\n", WSAGetLastError());
		return false;
	}

	
	retval = listen(gListenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		wprintf_s(L"listen socket error %d\n", WSAGetLastError());
		return false;
	}


	return true;
}




void NetworkProc()
{
	int retval;

	DWORD setCount = 0;

	//noLoginUserDisconnect();

	Session* session;

	FD_SET readSet;
	FD_SET writeSet;

	DWORD clientCodeTable[FD_SET_SIZE] = { -1 , };
	SOCKET clientSockTable[FD_SET_SIZE] = { INVALID_SOCKET, };

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	FD_SET(gListenSock,&readSet);
	clientCodeTable[setCount] = 0;
	clientSockTable[setCount] = gListenSock;

	++setCount;

	std::map<DWORD, Session*>::iterator iterE = gSessionList.end();

	for (std::map<DWORD, Session*>::iterator iter = gSessionList.begin(); iter != iterE;)
	{
		session = iter->second;
		++iter;

		clientCodeTable[setCount] = session->mUserCode;
		clientSockTable[setCount] = session->mClientSock;

		FD_SET(session->mClientSock, &readSet);

		if (!session->mSendQ.IsEmpty())
		{

			FD_SET(session->mClientSock, &writeSet);

		}

		++setCount;

		if (FD_SET_SIZE <= setCount)
		{
			SelectSocket(clientCodeTable, clientSockTable, &readSet, &writeSet);
			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			memset(clientCodeTable, -1, sizeof(DWORD) * FD_SET_SIZE);
			memset(clientSockTable, INVALID_SOCKET, sizeof(SOCKET) * FD_SET_SIZE);
			setCount = 0;		
		}
	}

	if (setCount > 0)
	{
		SelectSocket(clientCodeTable, clientSockTable, &readSet, &writeSet);
	}

}

void SelectSocket(DWORD* clientCodeTable, SOCKET* clientSockTable, FD_SET* readSet, FD_SET* writeSet)
{
	int retval;

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	retval = select(0, readSet, writeSet, NULL, &timeout);	
	if (retval > 0)
	{
		
		for (int iCnt = 0; iCnt < FD_SET_SIZE; ++iCnt)
		{

			if (FD_ISSET(clientSockTable[iCnt],writeSet))
			{
				SendEvent(clientCodeTable[iCnt]);
			}

			if (FD_ISSET(clientSockTable[iCnt],readSet))
			{

				if (clientCodeTable[iCnt] == 0)
				{
					Accept();
				}
				else
				{
					RecvEvent(clientCodeTable[iCnt]);
				}
			}

		}	
	}
	else if(retval == SOCKET_ERROR)
	{
		wprintf_s(L"selecet socket error : %d\n", WSAGetLastError());
	}
		
}


void Accept()
{

	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLength = sizeof(clientAddr);
	clientSock = accept(gListenSock, (SOCKADDR*)&clientAddr, &addrLength);
	if (clientSock == INVALID_SOCKET)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			wprintf_s(L"error %d\n", WSAGetLastError());
			return;
		}
	}


	Session *session = new Session();

	session->mClientAddr = clientAddr;
	session->mClientSock = clientSock;
	session->mUserCode = gUserCode;
	session->mEnterRoomNumber = NULL;

	gSessionList.insert(std::pair<DWORD, Session*>(gUserCode, session));

	gConnectList.insert(std::pair<DWORD, DWORD>(gUserCode, timeGetTime()));

	++gUserCode;
}


void noLoginUserDisconnect()
{
	std::unordered_map<DWORD, DWORD>::iterator iterE = gConnectList.end();

	for (std::unordered_map<DWORD, DWORD>::iterator iter = gConnectList.begin(); iter != iterE;)
	{
		if (3000 <= timeGetTime() - iter->second)
		{
			Session *session = FindClient(iter->first);
			if (session == nullptr)
			{
				wprintf_s(L"find error : %d\n", __LINE__);
				return;
			}
			
			closesocket(session->mClientSock);
			Disconnect(session->mUserCode);

			iter = gConnectList.erase(iter);
		}
		else
		{
			++iter;
		}
	}

}


// TODO : SendEvent 함수 정의
void SendEvent(DWORD clientKey)
{
	int retval;
	int sendSize;

	//char sendBuffer[2000];

	Session* session = FindClient(clientKey);
	if (session == nullptr)
	{
		return;
	}

	//sendSize = session->mSendQ.GetUseSize();

	//sendSize = min(sendSize, 2000);

	//if (0 >= sendSize)
	//{
	//	return;
	//}

	//session->mSendQ.Peek(sendBuffer, sendSize);	
	
	char* sendBuffer = session->mSendQ.GetFrontBufferPtr();

	sendSize = session->mSendQ.DirectDequeueSize();
	
	retval = send(session->mClientSock, sendBuffer, sendSize, 0);

	session->mSendQ.MoveFront(retval);

	if (retval == SOCKET_ERROR)
	{
		DWORD errorValue = WSAGetLastError();
		if (errorValue == WSAEWOULDBLOCK)
		{
			wprintf_s(L"would block User Number : %d\n", session->mUserCode);
			return;
		}
		closesocket(session->mClientSock);
		Disconnect(session->mUserCode);
		return;
	}

	return;
}

void RecvEvent(DWORD clientKey)
{
	int retval;



	Session* session = FindClient(clientKey);
	if (session == nullptr)
	{
		return;
	}

	int directEnqueueSize = session->mRecvQ.DirectEnqueueSize();

	retval = recv(session->mClientSock, session->mRecvQ.GetRearBufferPtr(), directEnqueueSize,0);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			wprintf_s(L"recv error : %d\n", WSAGetLastError());
			closesocket(session->mClientSock);
			Disconnect(session->mUserCode);

			return;
		}
		return;
	}
	
	bool retRecvFlag;	
	
	if (retval > 0)
	{	
		session->mRecvQ.MoveRear(retval);

		while (1)
		{
			retRecvFlag = CompleteRecvPacket(session);
			if (retRecvFlag)
			{
				break;
			}
			else
			{
				break;
			}
		}
	}

	return;
}


bool CompleteRecvPacket(Session* session)
{
	int retval;

	int useQueueSize = session->mRecvQ.GetUseSize();

	PacketHeader packetHeader;

	if (sizeof(PacketHeader) > useQueueSize)
	{
		return false;
	}

	retval = session->mRecvQ.Peek((char*)&packetHeader, sizeof(PacketHeader));
	if (retval != sizeof(PacketHeader))
	{
		wprintf_s(L"peek error 서버 종료하시오.\n");
		int* ptr = nullptr;
		*ptr = 0;
		return false;
	}

	if (packetHeader.byCode != dfPACKET_CODE)
	{
		wprintf_s(L"packet code error\n");
		closesocket(session->mClientSock);
		Disconnect(session->mUserCode);
		return false;
	}

	if (packetHeader.wPayloadSize + sizeof(PacketHeader) > useQueueSize)
	{
		return false;
	}

	session->mRecvQ.MoveFront(6);

	CMessage message;

	retval = session->mRecvQ.Dequeue(message.GetBufferPtr(), packetHeader.wPayloadSize);
	if (retval != packetHeader.wPayloadSize)
	{
		wprintf_s(L"payload size error\n");
		closesocket(session->mClientSock);
		Disconnect(session->mUserCode);
		return false;
	}

	message.MoveWritePos(packetHeader.wPayloadSize);

	
	if (packetHeader.byCheckSum != MakeCheckSum(packetHeader.wMsgType, &message))
	{
		wprintf_s(L"checksum error \n");
		closesocket(session->mClientSock);
		Disconnect(session->mUserCode);
		return false;
	}

	try
	{
		RecvPacketProc(session, packetHeader.wMsgType, &message);
	}
	catch (CExceptionObject& exception)
	{
		FILE* fp;

		// 에러 파일 오푼
		fopen_s(&fp, "ErrorDump.txt", "a+t");

		for (int iCnt = 0; iCnt < exception.m_BufferSize; ++iCnt)
		{
			// 메시지 로그
			fprintf_s(fp, "%02x ", exception.m_MessageLog[iCnt]);
		}

		// 에러 함수의 인자 데이터 타입
		fwrite(exception.m_ErrorDataLog, 1, sizeof(exception.m_ErrorDataLog), fp);

		closesocket(session->mClientSock);
		Disconnect(session->mUserCode);

		fclose(fp);
	}
}



// TODO : RecvPacketProc 구성하기
void RecvPacketProc(Session* session,WORD msgType, CMessage* message)
{

	switch (msgType)
	{
	case df_REQ_LOGIN:

		LoginPacketProcessing(session, message);

		break;
	case df_REQ_ROOM_LIST:

		RoomListPacketProcessing(session, message);

		break;
	case df_REQ_ROOM_CREATE:

		CreateRoomPacketProcessing(session, message);

		break;
	case df_REQ_ROOM_ENTER:

		EnterRoomPacketProcessing(session, message);
		
		break;
	case df_REQ_CHAT:

		ChatMsgPacketProcessing(session, message);

		break;
	case df_REQ_ROOM_LEAVE:

		LeaveRoomPacketProcessing(session, message);

		break;

	case df_REQ_STRESS_ECHO:

		StressTestProcessing(session, message);

		break;
	}

}


void LoginPacketProcessing(Session* session, CMessage* message)
{
	WCHAR* wStr;

	bool overlapFlag = false;
	
	wStr = (WCHAR*)message->GetBufferPtr();

	memset((char*)session->mUserName, 0, dfNICK_MAX_LEN);

	wcscpy_s(session->mUserName, wStr);

	message->MoveReadPos(message->GetDataSize());

	auto iterE = gSessionList.end();

	for (auto iter = gSessionList.begin(); iter != iterE; ++iter)
	{
		if (iter->second != session)
		{
			if (!wcscmp(session->mUserName, iter->second->mUserName))
			{
				overlapFlag = true;
			}
		}
	}

	gConnectList.erase(session->mUserCode);

	
	BYTE result;

	if (overlapFlag)
	{
		result = df_RESULT_LOGIN_DNICK;
	}
	else
	{
		result = df_RESULT_LOGIN_OK;
	}

	SendPacketResLogin(session, result);
}

// 로그인 패킷 만들기
void PackingResLogin(Session* session, PacketHeader* packetHeader, BYTE result, CMessage* message)
{

	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_LOGIN;

	*message << (unsigned char)result;
	*message << (unsigned int)session->mUserCode;

	packetHeader->wPayloadSize = message->GetDataSize();

	packetHeader->byCheckSum = MakeCheckSum(df_RES_LOGIN, message);
}

// 만든 로그인 패킷 send 링 버퍼에 인큐하기
void SendPacketResLogin(Session* session, BYTE result)
{
	PacketHeader packetHeader;

	CMessage message;

	PackingResLogin(session, &packetHeader, result, &message);

	SendUnicasting(session, &packetHeader, &message);
}


void RoomListPacketProcessing(Session* session, CMessage* message)
{
	// 로그인 여부 확인
	if (FindNoLoginClient(session->mUserCode))
	{
		closesocket(session->mClientSock);
		Disconnect(session->mUserCode);	
		gConnectList.erase(session->mUserCode);
		return;
	}


	SendPacketRoomList(session);
}

// 대화방 목록 패킷 만들기
void PackingResRoomList(Session* session, PacketHeader* packetHeader, CMessage* message)
{
	unsigned short roomAmount;
	
	roomAmount = gChatRoomList.size();

	*message << roomAmount;
	
	auto iterE = gChatRoomList.end();

	for (auto iter = gChatRoomList.begin(); iter != iterE; ++iter)
	{
		*message << iter->second->mRoomNumber;
		
		unsigned short titleSize = (unsigned short)((wcslen(iter->second->mRoomTitle) * 2));

		*message << titleSize;

		message->PutData((char*)iter->second->mRoomTitle, titleSize);

		message->MoveWritePos(titleSize);

		unsigned char userAmount = iter->second->mUserList.size();

		auto roomUserIterE = iter->second->mUserList.end();

		*message << userAmount;

		for (auto roomUserIter = iter->second->mUserList.begin(); roomUserIter != roomUserIterE; ++roomUserIter)
		{
			Session* roomUserSession = FindClient(*roomUserIter);
			if (roomUserSession == nullptr)
			{
				continue;
			}

			message->PutData((char*)roomUserSession->mUserName,dfNICK_MAX_LEN);
			message->MoveWritePos(dfNICK_MAX_LEN);
		}
	}

	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_ROOM_LIST;
	packetHeader->wPayloadSize = message->GetDataSize();
	packetHeader->byCheckSum = MakeCheckSum(df_RES_ROOM_LIST, message);

	return;
}


// 생성한 룸 리스트 패킷 send 링 버퍼에 인큐하기
void SendPacketRoomList(Session* session)
{
	PacketHeader packetHeader;

	CMessage message;

	PackingResRoomList(session, &packetHeader, &message);

	SendUnicasting(session, &packetHeader, &message);
}



void CreateRoomPacketProcessing(Session* session, CMessage* message)
{
	bool overlapFlag = false;

	unsigned short titleSize;

	*message >> titleSize;

	ChatRoom *chatRoom = new ChatRoom;

	memset(chatRoom->mRoomTitle, '\0', sizeof(chatRoom->mRoomTitle));

	message->GetData((char*)chatRoom->mRoomTitle, titleSize);

	message->MoveReadPos(titleSize);

	auto iterE = gChatRoomList.end();

	for (auto iter = gChatRoomList.begin(); iter != iterE; ++iter)
	{
		if (!wcscmp(iter->second->mRoomTitle, chatRoom->mRoomTitle))
		{
			overlapFlag = true;
		}
	}

	chatRoom->mRoomNumber = gChatRoomNumber;
	++gChatRoomNumber;


	BYTE result;
	if (overlapFlag)
	{
		result = df_RESULT_ROOM_CREATE_DNICK;

	}
	else
	{
		result = df_RESULT_ROOM_CREATE_OK;

		gChatRoomList.insert(std::pair<DWORD, ChatRoom*>(chatRoom->mRoomNumber, chatRoom));
	}

	SendPacketCreateRoom(session, chatRoom, result);
}



// 대화방 생성 패킷 생성 ( 수시로 브로드캐스트 )
void PackingResCreateRoom(ChatRoom* chatRoom, BYTE result, PacketHeader* packetHeader, CMessage* message)
{

	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_ROOM_CREATE;


	*message << (unsigned char)result;

	*message << (unsigned int)chatRoom->mRoomNumber;

	unsigned short titleSize = (unsigned short)(wcslen(chatRoom->mRoomTitle)*2);

	*message << (unsigned short)titleSize;

	message->PutData((char*)chatRoom->mRoomTitle, titleSize);

	message->MoveWritePos(titleSize);

	WORD dataSize = message->GetDataSize();
	packetHeader->wPayloadSize = dataSize;

	packetHeader->byCheckSum = MakeCheckSum(df_RES_ROOM_CREATE, message);

}


// 대화방 생성 링버퍼 인큐 response ( 수시로 브로드캐스트 )
void SendPacketCreateRoom(Session* session, ChatRoom* chatRoom, BYTE result)
{
	PacketHeader packetHeader;

	CMessage message;

	PackingResCreateRoom(chatRoom, result, &packetHeader, &message);

	if (result == df_RESULT_ROOM_CREATE_OK)
	{
		SendBroadcasting(&packetHeader, &message);
	}
	else
	{
		SendUnicasting(session, &packetHeader, &message);
	}
}

// TODO : 모든 메시지들 로그인 예외처리 넣기


// 대화방 입장 recv request
void EnterRoomPacketProcessing(Session* session, CMessage* message)
{
	BYTE result;
	
	unsigned int enterRoomNumber;

	*message >> enterRoomNumber;

	auto iterE = gChatRoomList.end();

	ChatRoom* chatRoom = FindChatRoom(enterRoomNumber);
	if (chatRoom == nullptr)
	{
		result = df_RESULT_ROOM_ENTER_NOT;		
	}
	else
	{
		session->mEnterRoomNumber = enterRoomNumber;
		chatRoom->mUserList.push_back(session->mUserCode);
		result = df_RESULT_ROOM_ENTER_OK;
	}

	SendPakcetEnterRoom(session, chatRoom, result);
}

// 새로운 유저 대화방 입장 ( 수시로 방 있는 사람들 브로드캐스팅 )
void PackingResNewUserEnter(Session* session, PacketHeader* packetHeader, CMessage* message)
{
	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_USER_ENTER;

	message->PutData((char*)session->mUserName, dfNICK_MAX_LEN * 2);
	message->MoveWritePos(dfNICK_MAX_LEN * 2);

	*message << (unsigned int)session->mUserCode;

	packetHeader->byCheckSum = MakeCheckSum(df_RES_USER_ENTER, message);
	packetHeader->wPayloadSize = message->GetDataSize();
}



// 대화방 입장 패킷 만들기
void PackingResEnterRoom(Session* session, ChatRoom* chatRoom, BYTE result, PacketHeader* packetHeader, CMessage* message)
{
	if (result != df_RESULT_ROOM_ENTER_OK)
	{
		packetHeader->byCode = dfPACKET_CODE;
		packetHeader->wMsgType = df_RES_ROOM_ENTER;

		packetHeader->byCheckSum = MakeCheckSum(df_RES_ROOM_ENTER, message);
		packetHeader->wPayloadSize = 0;
		memset(packetHeader, 0, sizeof(PacketHeader));
	}
	
	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_ROOM_ENTER;

	*message << (unsigned char)result;

	*message << (unsigned int)chatRoom->mRoomNumber;
	
	unsigned short titleSize = (unsigned short)(wcslen(chatRoom->mRoomTitle)*2);

	*message << titleSize;

	message->PutData((char*)chatRoom->mRoomTitle,titleSize);

	message->MoveWritePos(titleSize);

	auto *userListPtr = &chatRoom->mUserList;

	*message << (unsigned char)userListPtr->size();

	auto userListIterE = userListPtr->end();

	for (auto userListIter = userListPtr->begin(); userListIter != userListIterE; ++userListIter)
	{
		Session *findClient = FindClient(*userListIter);
		if (findClient == nullptr)
		{
			wprintf_s(L"FindClient error : %d ", __LINE__);
			return;
		}

		message->PutData((char*)findClient->mUserName, dfNICK_MAX_LEN * 2);
		message->MoveWritePos(dfNICK_MAX_LEN * 2);
		*message << (unsigned int)findClient->mUserCode;
	}

	packetHeader->wPayloadSize = message->GetDataSize();

	packetHeader->byCheckSum = MakeCheckSum(df_RES_ROOM_ENTER, message);
}

// 대화방 입장 response send 인큐
void SendPakcetEnterRoom(Session* session, ChatRoom* chatRoom, BYTE result)
{
	// TODO : chatRoom nullptr 일 경우 예외처리 하기

	PacketHeader packetHeader;

	CMessage message;

	PackingResEnterRoom(session, chatRoom, result , &packetHeader, &message);

	SendUnicasting(session, &packetHeader, &message);

	memset((char*)&packetHeader, 0, sizeof(PacketHeader));
	message.Clear();
	PackingResNewUserEnter(session, &packetHeader, &message);

	SendRoomBroadcasting(chatRoom, session, &packetHeader, &message);
}


// 채팅 메시지 request
void ChatMsgPacketProcessing(Session* session, CMessage* message)
{
	unsigned short msgSize;

	*message >> msgSize;

	wprintf_s(L"%d\n", msgSize);

	WCHAR* wStr = (WCHAR*)malloc(msgSize + 2);

	memset(wStr, 0, msgSize + 2);

	message->GetData((char*)wStr, msgSize);

	ChatRoom* chatRoom = FindChatRoom(session->mEnterRoomNumber);

	SendChatMessage(session, chatRoom, wStr);

	free(wStr);
}

// 채팅 메시지 ( 룸 있는지 확인 후 입장 보낸 사람은 제외 후 브로드캐스팅)
void PackingResChatMessage(Session* session,WCHAR* wStr,PacketHeader* packetHeader, CMessage* message)
{

	packetHeader->byCode = dfPACKET_CODE;

	packetHeader->wMsgType = df_RES_CHAT;

	*message << (unsigned int)session->mUserCode;

	unsigned short strSize = (unsigned short)((wcslen(wStr) * 2));

	*message << strSize;

	message->PutData((char*)wStr, strSize);

	message->MoveWritePos(strSize);
	
	packetHeader->byCheckSum = MakeCheckSum(df_RES_CHAT, message);

	packetHeader->wPayloadSize = message->GetDataSize();
}

// 채팅 메시지 룸 브로드캐스팅 
void SendChatMessage(Session* session, ChatRoom* chatRoom, WCHAR* wStr)
{
	PacketHeader packetHeader;

	CMessage message;

	PackingResChatMessage(session ,wStr, &packetHeader, &message);

	SendRoomBroadcasting(chatRoom, session, &packetHeader, &message);
}



// 대화방 나가기 recv processing
void LeaveRoomPacketProcessing(Session* session, CMessage* message)
{
	bool roomDeleteFlag;

	// 방입장한 session이 아니라면 끊는다.
	if (session->mEnterRoomNumber == 0)
	{
		closesocket(session->mClientSock);
		Disconnect(session->mUserCode);
		return;
	}
	
	ChatRoom* chatRoom = FindChatRoom(session->mEnterRoomNumber);
	if (chatRoom == nullptr)
	{
		wprintf_s(L"chatRoom find error : %d\n", __LINE__);
	}

	session->mEnterRoomNumber = 0;

	chatRoom->mUserList.remove(session->mUserCode);

	if (chatRoom->mUserList.size() == 0)
	{
		roomDeleteFlag = true;
		gChatRoomList.erase(chatRoom->mRoomNumber);
	}
	else
	{
		roomDeleteFlag = false;
	}

	SendRoomLeave(session, chatRoom, roomDeleteFlag);
}


// 대화방 나가기 ( 수시로 방 유저들 브로드캐스팅 )
void PackingResRoomLeave(Session* session, PacketHeader* packetHeader, CMessage* message)
{

	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_ROOM_LEAVE;

	*message << (unsigned int)session->mUserCode;

	packetHeader->wPayloadSize = message->GetDataSize();

	packetHeader->byCheckSum = MakeCheckSum(df_RES_ROOM_LEAVE, message);
}


// 대화방 삭제 ( 수시로 방 삭제 브로드 캐스팅 )
// 대화방의 유저가 0명이면은 방 삭제
void PackingResRoomDelete(Session* session, ChatRoom* chatRoom, PacketHeader* packetHeader, CMessage* message)
{
	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_ROOM_DELETE;

	*message << (unsigned int)chatRoom->mRoomNumber;
	
	packetHeader->byCheckSum = MakeCheckSum(df_RES_ROOM_DELETE, message);
	packetHeader->wPayloadSize = message->GetDataSize();

}


// 대화방 나가기 send request
void SendRoomLeave(Session* session, ChatRoom* chatRoom, bool roomDeleteFlag)
{
	PacketHeader packetHeader;
	CMessage message;

	PackingResRoomLeave(session, &packetHeader, &message);
	SendUnicasting(session, &packetHeader, &message);

	if (roomDeleteFlag)
	{
		memset((char*)&packetHeader, 0, sizeof(PacketHeader));
		message.Clear();
		PackingResRoomDelete(session, chatRoom, &packetHeader, &message);
		SendBroadcasting(&packetHeader, &message);
	}
	else
	{
		SendRoomBroadcasting(chatRoom, nullptr, &packetHeader, &message);
	}
}


void StressTestProcessing(Session* session, CMessage* message)
{
	unsigned short wStrSize;

	*message >> wStrSize;

	WCHAR* wStr = (WCHAR*)malloc(wStrSize);
	
	message->GetData((char*)wStr, wStrSize);

	message->MoveReadPos(wStrSize);

	SendStressTest(session, wStr, wStrSize);

	free(wStr);
}

void PackingStressTest(Session* session, WCHAR* wStr, unsigned short strSize, PacketHeader* packetHeader, CMessage* message)
{
	packetHeader->byCode = dfPACKET_CODE;
	packetHeader->wMsgType = df_RES_STRESS_ECHO;

	*message << strSize;

	message->PutData((char*)wStr, strSize);

	message->MoveWritePos(strSize);

	packetHeader->byCheckSum = MakeCheckSum(df_RES_STRESS_ECHO, message);

	packetHeader->wPayloadSize = message->GetDataSize();

}

void SendStressTest(Session* session, WCHAR* wStr, unsigned short strSize)
{
	PacketHeader packetHeader;

	CMessage message;

	PackingStressTest(session, wStr, strSize,&packetHeader, &message);

	SendUnicasting(session, &packetHeader, &message);
}






void SendUnicasting(Session* session, PacketHeader *packetHeader, CMessage* message)
{
	if (session == nullptr)
	{
		wprintf_s(L"SendUnicasting NULL\n");
		return;
	}

	session->mSendQ.Enqueue((char*)packetHeader, sizeof(PacketHeader));

	int num = message->GetDataSize();

	session->mSendQ.Enqueue(message->GetBufferPtr(), num);


}

void SendBroadcasting(PacketHeader *packetHeader, CMessage* message)
{
	std::map<DWORD, Session*>::iterator iterE = gSessionList.end();

	for (std::map<DWORD, Session*>::iterator iter = gSessionList.begin(); iter != iterE; ++iter)
	{
		SendUnicasting(iter->second, packetHeader, message);
	}
}

void SendRoomBroadcasting(ChatRoom* chatRoom, Session* session ,PacketHeader *packetHeader, CMessage* message) 
{

	std::list<DWORD>::iterator iterE = chatRoom->mUserList.end();

	for (std::list<DWORD>::iterator iter = chatRoom->mUserList.begin(); iter != iterE; ++iter)
	{
		Session* sendClient = FindClient(*iter);
		if (sendClient == nullptr)
		{
			continue;
		}

		if (session != sendClient)
		{
			SendUnicasting(sendClient, packetHeader, message);
		}
	}
}


BYTE MakeCheckSum(WORD msgType, CMessage* message)
{
	BYTE checkSum = 0;

	for (int iCnt = 0; iCnt < sizeof(WORD); ++iCnt)
	{
		checkSum += *(((BYTE*)(&msgType)) + iCnt);
	}

	for (int iCnt = 0; iCnt < message->GetDataSize(); ++iCnt)
	{
		checkSum += *(((BYTE*)(message->GetBufferPtr())) + iCnt);
	}

	return checkSum % 256;
}


Session* FindClient(DWORD clientCode)
{

	auto iterE = gSessionList.end();

	auto clientIter = gSessionList.find(clientCode);

	if (clientIter == iterE)
	{
		return nullptr;
	}

	return clientIter->second;
}


ChatRoom* FindChatRoom(DWORD roomNumber)
{

	auto iterE = gChatRoomList.end();

	auto chatRoomIter = gChatRoomList.find(roomNumber);

	if (chatRoomIter == iterE)
	{
		return nullptr;
	}

	return chatRoomIter->second;
}


bool FindNoLoginClient(DWORD clientCode)
{

	auto iterE = gConnectList.end();

	auto connectIter = gConnectList.find(clientCode);

	if (connectIter == iterE)
	{
		return false;
	}

	return true;
}



void Disconnect(DWORD clientKey)
{
	Session* session = FindClient(clientKey);
	if (session == nullptr)
	{
		return;
	}

	session->mRecvQ.Release();
	session->mSendQ.Release();

	// 0이 아닐 경우 무조건 룸에 들어가 있다. 
	if (session->mEnterRoomNumber != NULL)
	{
		std::list<DWORD> *roomUserList = &gChatRoomList.find(session->mEnterRoomNumber)->second->mUserList;
		
		roomUserList->remove(clientKey);
	}

	gSessionList.erase(session->mUserCode);
}