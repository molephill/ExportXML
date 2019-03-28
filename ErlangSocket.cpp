#include "stdafx.h"
#include "ErlangSocket.h"

struct data {
	unsigned short int len;
	unsigned short int cmd;
	char content[5];
};

ErlangSocket::ErlangSocket()
{
	//WSADATA wsaData;
	//WSAStartup(MAKEWORD(2, 2), &wsaData);
	//m_sc = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, NULL);
	////设置该套接字使之可以重新绑定端口
	///*int optval = 1;
	//setsockopt(m_sc, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(int));*/
	//SOCKADDR_IN addr;
	//int len = sizeof(addr);
	//addr.sin_family = AF_INET;
	////addr.sin_addr.s_addr = inet_addr("192.168.1.127");
	//inet_pton(AF_INET, "192.168.1.127", &addr.sin_addr);
	//addr.sin_port = htons(8101);
	//connect(m_sc, (struct sockaddr *)&addr, len);
	//char buff[1024];
	//memset(buff, 0, 1024);

	//struct data pdata = { 4, 1001, "test" };

	////发送数据  
	//printf("send data: %d %d %s \n", pdata.len, pdata.cmd, pdata.content);
	//send(m_sc, (char *)&pdata, sizeof(pdata), 0);

	//接收数据  
	//recv(m_sc, buff, 1024, 0);
	//printf("recv data: %s \n", buff);
}

SOCKET ErlangSocket::Connect(const char* ip, int port)
{
	if (m_sc) 
	{
		closesocket(m_sc);
		WSACleanup();
	}

	//加载套接字  
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("Failed to load Winsock");
		return 0;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addrSrv.sin_addr);

	//创建套接字  
	m_sc = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == m_sc) {
		printf("Socket() error:%d", WSAGetLastError());
		return 0;
	}

	//向服务器发出连接请求  
	if (connect(m_sc, (struct  sockaddr*)&addrSrv, sizeof(addrSrv)) == INVALID_SOCKET) {
		printf("Connect failed:%d", WSAGetLastError());
		return 0;
	}

	//发送数据  
	/*char buff[] = "roles";
	send(m_sc, buff, sizeof(buff), 0);*/

	return m_sc;
	
	//CloseHandle(hThread);
}

void ErlangSocket::SendGM(INT64 id, const std::string& gm)
{
	/*m_scgm.len = gm.size() * sizeof(char);
	m_scgm.id = id;
	m_scgm.gm = gm;
	send(m_sc, (char *)&m_scgm, sizeof(m_scgm), 0);*/
	//send(m_sc, data, sizeof(data), 0);
}

void ErlangSocket::SendGM(const char* gm)
{
	send(m_sc, gm, strlen(gm), 0);
}

void ErlangSocket::Run()
{
	/*bool run = true;
	while (run)
	{
		if (!m_sc) run = false;
		char buff[1024];
		memset(buff, 0, 1024);
		recv(m_sc, buff, 1024, 0);
		printf("recv data: %s \n", buff);
	}*/
}

ErlangSocket::~ErlangSocket()
{
	closesocket(m_sc);
	WSACleanup();
	getchar();
}
