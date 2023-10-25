#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <iostream>
#include <WinSock2.h>
#include <process.h>
# define PORT 2023
using namespace std;

SOCKET ServerSocket = INVALID_SOCKET;//������һ���׽��ֱ��� ServerSocket ����ʼ����Ϊ INVALID_SOCKET��������������״̬
SOCKADDR_IN caddr = { 0 };	         //�ͻ��˵�ַ
int caddrlen = sizeof(caddr);        //�ͻ��˵�ַ����
//��time_t��ʾʱ���Ǵ�1970��1��1��0ʱ0��0�뵽��ʱ������������һ������������str�Ǵ��ʽ�����ʱ���ַ�����
time_t t;
char str[26];

//����ͻ�����Ϣ�ṹ�壬���������ͻ����׽��֡����ݻ��������û�����IP���ͻ��˱�־��Ա�����ڱ�ʶת���ķ�Χ(����ʵ���ض��û�ת������)
struct Client
{
	SOCKET ClientSocket;    
	char buffer[128];		 
	char username[10];   
	char IP[20];		
	UINT_PTR flag;   
	char targetarea[20];      
} realclient[20];   //����һ��ʵ�������ͬʱ����20���û�ͬʱ����

//�ڷ������ˣ�ÿ���ͻ��˵ķ�����һ���������߳̽��й���ͬʱ��Щ�̹߳���һ���û��б�
HANDLE HandleR[20] = { NULL };				 //������Ϣ�߳̾��
HANDLE Handle;							 //����accept���߳̾��

int i = 0; //����õ���Ϊ�˷��������һ��ȫ�ֱ���

DWORD WINAPI resendthread(void* data);

//���������߳�
DWORD WINAPI acceptthread(void* data)
{
	int flag[20] = { 0 };//��־���飬һһ��Ӧ�ͻ��ˣ�������״̬
	while (true)//������һֱ�ȴ��ͻ��˵�����
	{
		if (realclient[i].flag != 0)   //�ҵ����пͻ���λ��
		{
			i++;
			continue;
		}

		/*����accept�����ȴ��ͻ������ӣ�����������̣�����ͨ��accept�������������׽���
		����Ҫʹ�ù�bind�����󶨣���Ϊ���׽��ֻ��Զ�����accept�����ṩ�ķ�������ַ��Ϣ*/
		if ((realclient[i].ClientSocket = accept(ServerSocket, (SOCKADDR*)&caddr, &caddrlen)) == INVALID_SOCKET)
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
			cout << "[����] Accept���󣬱�����Ϣ��" <<errMsg<< endl;
			closesocket(ServerSocket);
			WSACleanup();
			return -1;
		}

		//�����û����������Ȼ���¼���û�IP����ʹ��socket��������Ϊһ�����صı�ʶ������ǻ����ֲ�ͬ�Ŀͻ���
		recv(realclient[i].ClientSocket, realclient[i].username, sizeof(realclient[i].username), 0);
		cout << "[����] �û� [" << realclient[i].username << "]" << " ���ӳɹ�" << endl;
		memcpy(realclient[i].IP, inet_ntoa(caddr.sin_addr), sizeof(realclient[i].IP));
		realclient[i].flag = realclient[i].ClientSocket;
		i++;

		//���������ͻ��˲��������̣�����״̬�ı�Ŀͻ��ˣ������´���������Ϣ�߳�
		for (int j = 0; j < i; j++)
		{
			if (realclient[j].flag != flag[j])
			{
				if (HandleR[j]) //�����Ч
					CloseHandle(HandleR[j]);
				HandleR[j] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)resendthread, &realclient[j].flag, 0, 0);
			}
		}
		for (int j = 0; j < i; j++)
			flag[j] = realclient[j].flag;//ͬ��flag����ֹ�߳��ظ�����
		Sleep(500);
	}
	return 0;
}


//���ղ��������ݵ��̺߳�����WINAPI��ʾ��������ĵ���Լ���Ǳ�׼����Լ��
DWORD WINAPI resendthread(void* data)
{
	bool first = true;
	SOCKET client = INVALID_SOCKET;
	int flag = 0;
	for (int j = 0; j < i; j++) {
		if (*(int*)data == realclient[j].flag)//�ҵ��Ǹ������̵߳Ŀͻ���
		{
			client = realclient[j].ClientSocket;
			flag = j;
		}
	}
	char temp[128] = { 0 };  //����һ����ʱ�ַ��������Ϣ
	string target, content;//��Ϣ���Է����Σ�����Ŀ���û������ݡ�
	while (true)
	{
		memset(temp, 0, sizeof(temp));//ÿ��ѭ�������temp
		if (recv(client, temp, sizeof(temp), 0) == SOCKET_ERROR){
			continue;
		}
		//��ȡ��һ��Ӣ��ð��ǰ������Ϊ����Ŀ���û���ð�ź�����Ϣ����
		string contents = temp;
		target = contents.substr(0, contents.find(':'));
		content = contents.substr(contents.find(':') + 1 == 0 ? contents.length() : contents.find(':') + 1);
		if (content.length() == 0)//���ʾcontents����Ӣ��ð�ţ���ʱtarget�д���������contents
		{
			strcpy(realclient[flag].targetarea, "all");
			strcpy(temp, target.c_str());
		}
		else
		{
			strcpy(realclient[flag].targetarea, target.c_str());
			strcpy(temp, content.c_str());
		}

		memcpy(realclient[flag].buffer, temp, sizeof(realclient[flag].buffer));//�����ݴ浽buffer��Ա��

		if (strcmp(temp, "quit") == 0)   //������Ϊ��ʵ���û����������˳������ҵĹ��ܣ������⵽�û�����quit������ôֱ�ӹر��̣߳�����ת���߳�
		{
			//���ιر��׽��֡��߳̾��������λ�ÿճ������Ժ������߳�ʹ��
			closesocket(realclient[flag].ClientSocket);
			CloseHandle(HandleR[flag]);   
			realclient[flag].ClientSocket = 0;  
			HandleR[flag]=NULL;
			cout << "[����] �û� [" << realclient[flag].username << "] " << "�뿪���������� " << endl;
		}
		else if (first == true)//ȷ��һ���û���һ�����ӵ�ʱ����Ϣ���ᱻת������Ϊ��������Ƿ����û���
		{
			first = false;
			continue;
		}
		else
		{
			//��ʽ�����ʱ�䣬�Լ��û����ͷ��͵���Ϣ����
			time(&t);
			strftime(str, 20, "%Y-%m-%d %X", localtime(&t));
			cout << '(' << str << ")  [" << realclient[flag].username << "] :" << temp << endl;

			char temp[128] = { 0 };		   
			memcpy(temp, realclient[flag].buffer, sizeof(temp));//��������
			sprintf(realclient[flag].buffer, "%s: %s", realclient[flag].username, temp); //�ѷ�����Ϣ���û�����ӽ�ת������Ϣ��
			if (strlen(temp) != 0) //������ݲ�Ϊ����ת��
			{
				// �������û�����(Ⱥ��)��������Լ�֮������пͻ��˷�����Ϣ
				if (strcmp(realclient[flag].targetarea, "all") == 0)
				{
					for (int j = 0; j < i; j++){
						if (j != flag){
							if (send(realclient[j].ClientSocket, realclient[flag].buffer, sizeof(realclient[j].buffer), 0) == SOCKET_ERROR){
								return -1;
							}
						}
					}
				}
				// ���ض��û�����(˽��)
				else{
					for (int j = 0; j < i; j++){
						if (strcmp(realclient[flag].targetarea, realclient[j].username) == 0){
							if (send(realclient[j].ClientSocket, realclient[flag].buffer, sizeof(realclient[j].buffer), 0) == SOCKET_ERROR){
								return -1;	
							}
						}
					}
				}
			}
		}
	}
	return 0;
}





int main()
{
	//��ʼ��WSADATA�ṹ�壬���ڽ���WSAStartup������ϸ��Ϣ
	WSADATA wd = { 0 };
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
		cout << "[����] WSAStartup�������󣬱�����Ϣ��" <<errMsg<< endl;
		return -1;
	}

	/*����ʵ��Ҫ�������Ҵ�����IP��ַ����ΪAF_INET��IPV-4Э�飩��
	��������Ϊ��ʽ(SOCK_STREAM)��ʹ��TCPЭ����׽���*/
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//socket�����Ĵ�����
	if (ServerSocket == INVALID_SOCKET)
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
		WSACleanup();
		return -1;
	}
    /*��ʼ�������÷�������ַ�����趨�����˿ڣ�ʹ��IPv4��ַ��ֵ��ע�����
	INADDR_ANY ��һ������꣬��ֵΪ0.0.0.0����ʾ������������������ӿ��Ͻ��м�����*/
	SOCKADDR_IN saddr = { 0 };				
	unsigned short port = PORT;					
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	//bind�׶Σ����������׽������ַ�˿ڰ󶨣����İ���������
	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&saddr, sizeof(saddr)))
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
		cout << "[����] Bind�󶨳��ִ��󣬱�����Ϣ��" <<errMsg<< endl;
		closesocket(ServerSocket);//���﷢��������Ҫ����ر��׽���
		WSACleanup();
		return -1;
	}

	//bind�׶���ɺ����listen�׶Σ����ô����������������е���󳤶�Ϊ3�����İ���������
	if (listen(ServerSocket, 3) == SOCKET_ERROR)
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
		cout << "Listen�������ִ��󣬱�����Ϣ��" <<errMsg<< endl;
		closesocket(ServerSocket);//���﷢��������Ҫ����ر��׽���
		WSACleanup();
		return -1;
	}

	//�������̾�������˵���������Ѿ�׼�������������߳̾��
	cout << "[����] ����!" << endl;
	cout << "[����] ��ܰ��ʾ������ر����������ҷ������������� quit " << endl;
	cout << "=========================================================" << endl;
	Handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)acceptthread, NULL, 0, 0);
	
	//ʵ��ʵ��Ҫ�󣺷��������������˳�������quit���ɹرշ�����
	char ssignal[8];
	cin.getline(ssignal, 8);
	if (strcmp(ssignal, "quit") == 0)
	{
		cout << "[����] ���������׼���ر�" << endl;
		const char* cshutsignal = "������������ѹر�";
		for (int j = 0; j <= i; j++)
		{
			if (realclient[j].ClientSocket != INVALID_SOCKET){
				//����ÿ����Ч�ͻ��ˣ����ͷ������ر���Ϣ��ر�����
				send(realclient[j].ClientSocket, cshutsignal, strlen(cshutsignal), 0);
				closesocket(realclient[j].ClientSocket);
			}
		}
		CloseHandle(Handle);
		closesocket(ServerSocket);
		WSACleanup();
		cout << "[����] ����������Ѿ��ر�" << endl;
	}
	else{
		cout<<"ָ����Ч���Զ��˳��������Ϳͻ���"<<endl;
	}
}