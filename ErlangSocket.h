#pragma once

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <cstdlib>
#pragma comment(lib,"ws2_32.lib")//���ÿ��ļ�

#include <WS2tcpip.h>
#include <vector>

struct CMDSC
{
	unsigned short int len;
	INT64 id;
	std::string gm;
};

class ErlangSocket
{
public:
	ErlangSocket();
	~ErlangSocket();

private:
	SOCKET m_sc;
	SOCKADDR_IN m_addr;
	CMDSC m_scgm;

public:
	SOCKET Connect(const char*, int);
	void SendGM(INT64, const std::string&);
	void SendGM(const char*);
	void Run();
};
