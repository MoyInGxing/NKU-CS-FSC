#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <iostream>
#include <process.h>
#include <conio.h>
#include <limits>

# define PORT 2023
# define IP "127.0.0.1"
using namespace std;

//��time_t��ʾʱ���Ǵ�1970��1��1��0ʱ0��0�뵽��ʱ������������һ������������str�Ǵ��ʽ�����ʱ���ַ�����
time_t t;
char str[26];

char username[10] = { 0 };

//�����߳�
DWORD WINAPI rdeal(void* data)
{
	char bufferget[128] = { 0 };
	while (true)//һֱ�ȴ�����
	{
		if (recv(*(SOCKET*)data, bufferget, sizeof(bufferget), 0) == SOCKET_ERROR){
			break;
		}
		if (strlen(bufferget) != 0)
		{
			// ���������Ƿ��͹ر��ź�
			if (strcmp(bufferget, "������������ѹر�") == 0) 
			{
				cout << "����������ѹرգ������ý����Զ��˳�" << endl;
				closesocket(*(SOCKET*)data);
				break;
			}
			//'\b'���Ǩ�ƣ������������һ������ĵȴ��û�������Ϣ���ַ��������磺(2023-10-17 20:24:03)  [fsc] :
			for (int i = 0; i <= strlen(username) + 40; i++){
				cout << "\b";
			}
			// ��ӡʱ���Լ����յ�����Ϣ
			time(&t);
			strftime(str, 20, "%Y-%m-%d %X", localtime(&t));
			cout << '(' << str << ")    " << bufferget << endl;
			//�ղŰ�����ȴ��û�������ַ������˻��ˣ�������Ҫ�ٰ�����ַ�����ӡ����
			cout << '(' << str << ")  ["<< username << "] : ";
		}
	}
	Sleep(3000);
	exit(1);
	return 0;
}

int main()
{
	WSADATA wd = { 0 };//����׽�����Ϣ
	SOCKET ClientSocket = INVALID_SOCKET;//�ͻ����׽���

	//���ڳ�ʼ���׽��֣�������2.2winsock�汾�����ط���ֵ��ʾ��ʼ��ʧ�ܣ����Ĵ��������ʼ��������
	if (WSAStartup(MAKEWORD(2, 2), &wd))
	{
		char errMsg[512];
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            WSAGetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            errMsg,
            sizeof(errMsg),
            NULL
        );
		cout << "[����] WSAStartup����ʧ�ܣ�������Ϣ��" <<errMsg<< endl;
		return -1;
	}

	/*����ʵ��Ҫ�������Ҵ�����IP��ַ����ΪAF_INET��IPV-4Э�飩��
	��������Ϊ��ʽ(SOCK_STREAM)��ʹ��TCPЭ����׽���*/
	ClientSocket = socket(AF_INET, SOCK_STREAM, 0);
	//socket�����Ĵ�����	
	if (ClientSocket == INVALID_SOCKET)
	{
		char errMsg[512];
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            WSAGetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            errMsg,
            sizeof(errMsg),
            NULL
        );
		cout << "[����] Socket�������󣬱�����Ϣ��" <<errMsg<< endl;
		return -1;
	}

	/*��ʼ�������÷�������ַ�����趨�����˿ڣ�ʹ��IPv4��ַ�������ַ�������úõ�IP��ַ��*/
	SOCKADDR_IN saddr = { 0 };      
	USHORT port = PORT;           
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.S_un.S_addr = inet_addr(IP);

	//���ӷ�����������������Ϣ
	if (SOCKET_ERROR == connect(ClientSocket, (SOCKADDR*)&saddr, sizeof(saddr)))
	{
		char errMsg[512];
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            WSAGetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            errMsg,
            sizeof(errMsg),
            NULL
        );
		cout << "[����] Connect���Ӵ��󣬱�����Ϣ��" <<errMsg<< endl;
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	cout << "[����] ��������������ɹ�!���������˵�ַ��ϢΪ��" << endl;
	cout << "[����] IP ��" << inet_ntoa(saddr.sin_addr) << "     " << "�˿ں� ��" << htons(saddr.sin_port) << endl;
	cout << "[����] �ɹ��������������ң���˵�����ĳƺţ����ĳƺŲ�Ӧ����all��: ";

	// ��������
	string name;
	cin>>name;
	while (name.length() > 9) {
        cout << "�����û����Ƿ������������룺";
		cin>>name;
    }
	strcpy(username, name.c_str());
	send(ClientSocket, username, sizeof(username), 0);//�����û�����������
	
	cout << "[����] ��������˳��������� quit " << endl;
	cout << "=========================================================" << endl;

	// ���������߳�
	HANDLE recvthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)rdeal, &ClientSocket, 0, 0);

	char bufferSend[128] = { 0 };
	bool start = false;
	while (true)
	{
		if (start){
			//���
			time(&t);
			strftime(str, 20, "%Y-%m-%d %X", localtime(&t));
			cout << "(" << str << ")  "<< "[" << username << "] : ";
		}
		start = true;

		// �û����뷢����Ϣ
		cin.getline(bufferSend, 128);

		if (cin.fail()) // ����Ƿ���Ϊ���볬������
		{
			cin.clear(); // ��������־
			cin.ignore(numeric_limits<streamsize>::max(), '\n'); // �����������е�ʣ���ַ�
			cout << "���볬�ޣ�����������" << endl;
			continue;
		}

		//����û�����quit׼���˳�
		if (strcmp(bufferSend, "quit") == 0)
		{
			cout << "�����뿪������" << endl;
			send(ClientSocket, bufferSend, sizeof(bufferSend), 0);
			break;
		}
		send(ClientSocket, bufferSend, sizeof(bufferSend), 0);
	}
	closesocket(ClientSocket);
	CloseHandle(recvthread);
	WSACleanup();
	system("pause");
	return 0;
}