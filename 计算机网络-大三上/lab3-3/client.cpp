#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <fstream>
#include <queue>
#include <vector>
#include "Message.h"
#include <limits>
#pragma comment (lib, "ws2_32.lib")

using namespace std;



//����ʵ��pptԭ��ͼ�����ʵ����client��ֱ�ӷ��͵�router���ٽ���ת����server
//���Կ��Լ����Ϊ����client��˵router����˫��ͨ���еġ�server��
const int RouterPORT = 30000; 
const int ClientPORT = 20230;



//���崴��ACK�����߳�ʱ�򴫹�ȥ�Ĳ��������ڴ����߳�ֻ�ܴ�һ���������������ｫ��Ҫ����ȥ����Ϣ�󶨳ɽṹ��
struct parameters {
	SOCKET clientSocket;
	SOCKADDR_IN serverAddr;
	int nummessage;
};

// ���崰�ڴ�СΪ10
#define windowssize 10

//��Ϣ״̬�ṹ�壬���е�message���˻�����������
struct MessageState {
    Message msg;
    bool acked; // �Ƿ����յ�ACK
    clock_t sendTime; // ��Ϣ����ʱ��
};
MessageState window[windowssize];

//��������
HANDLE mutex;
//client��ά�������к�
int initrelseq = 0;
//�������ڵĿ�ʼ�͵���
int start = 0;
int arrive = 0;

//���������־λ
bool finish = false;


//ʵ��client����������
bool threewayhandshake(SOCKET clientSocket, SOCKADDR_IN serverAddr)
{
	int AddrLen = sizeof(serverAddr);
	Message buffer1,buffer2,buffer3;

	//���͵�һ�����ֵ���Ϣ��SYN��Ч��seq=x�����seq����0����
	buffer1.SrcPort = ClientPORT;
	buffer1.DestPort = RouterPORT;
	buffer1.flag += SYN;
	buffer1.SeqNum = initrelseq;
	buffer1.setCheck();

	int sendByte = sendto(clientSocket, (char*)&buffer1, sizeof(buffer1), 0, (sockaddr*)&serverAddr, AddrLen);
	clock_t buffer1start = clock();
	if (sendByte > 0)
	{
		cout << "client���͵�һ�����֣�"
		<< "Դ�˿�: " << buffer1.SrcPort << ", "
		<< "Ŀ�Ķ˿�: " << buffer1.DestPort << ", "
		<< "���к�: " << buffer1.SeqNum << ", "
		<< "��־λ: " << "[SYN: " << (buffer1.flag & SYN ? "SET" : "NOT SET") 
		<< "] [ACK: " << (buffer1.flag & ACK ? "SET" : "NOT SET") 
		<< "] [FIN: " << (buffer1.flag & FIN ? "SET" : "NOT SET") << "], "
		<< "У���: " << buffer1.checkNum << endl;
	}

	int resendtimes = 0;
	//���յڶ������ֵ���Ϣ
	while (true)
	{
		int recvByte = recvfrom(clientSocket, (char*)&buffer2, sizeof(buffer2), 0, (sockaddr*)&serverAddr, &AddrLen);
		if (recvByte > 0)
		{
			//�ɹ��յ���Ϣ�����У��͡�ACK��SYN��ack
			if ((buffer2.flag & ACK) && (buffer2.flag & SYN) && buffer2.check() && (buffer2.AckNum == buffer1.SeqNum+1))
			{
				cout << "client���յڶ������ֳɹ�" << endl;
				break;
			}
			else{
				cout<<"���յڶ���������Ϣ���ʧ��"<<endl;
			}
		}

		//client���͵�һ�λ��ֳ�ʱ�����·��Ͳ����¼�ʱ
		if (clock() - buffer1start > MAX_WAIT_TIME)
		{
            if (++resendtimes > MAX_SEND_TIMES) {
                cout << "client���͵�һ�����ֳ�ʱ�ش��ѵ�������������ʧ��" << endl;
                return false;
            }
			cout << "client���͵�һ�����֣���"<< resendtimes <<"�γ�ʱ�������ش�......" << endl;
			int sendBtye = sendto(clientSocket, (char*)&buffer1, sizeof(buffer1), 0, (sockaddr*)&serverAddr, AddrLen);
			buffer1start = clock();
            if (sendByte <= 0) {
                cout << "client��һ�������ش�ʧ��" << endl;
				return false;
            } 
		}
	}

	//���͵��������ֵ���Ϣ��ACK��Ч��seq=x+1�����seq����1��ack=y+1��Ҳ����1����
	buffer3.SrcPort = ClientPORT;
	buffer3.DestPort = RouterPORT;
	buffer3.flag += ACK;
	buffer3.SeqNum = ++initrelseq;
	buffer3.AckNum = buffer2.SeqNum+1;
	buffer3.setCheck();

	sendByte = sendto(clientSocket, (char*)&buffer3, sizeof(buffer3), 0, (sockaddr*)&serverAddr, AddrLen);
	clock_t buffer3start = clock();
	if (sendByte == 0)
	{
		cout << "client���͵���������ʧ��" << endl;
		return false;
	}
	cout << "client���͵���������:"
	<< "Դ�˿�: " << buffer3.SrcPort << ", "
	<< "Ŀ�Ķ˿�: " << buffer3.DestPort << ", "
	<< "���к�: " << buffer3.SeqNum << ", "
	<< "ȷ�Ϻ�: " << (buffer3.flag & ACK ? to_string(buffer3.AckNum) : "0") << ", "
	<< "��־λ: " << "[SYN: " << (buffer3.flag & SYN ? "SET" : "NOT SET") 
	<< "] [ACK: " << (buffer3.flag & ACK ? "SET" : "NOT SET") 
	<< "] [FIN: " << (buffer3.flag & FIN ? "SET" : "NOT SET") << "], "
	<< "У���: " << buffer3.checkNum << endl;
	cout << "client���ӳɹ���" << endl;
	start=initrelseq+1;
	arrive=initrelseq+1;
	return true;
}




//����ack���߳�
DWORD WINAPI recvackthread(PVOID useparameter)
{
	mutex = CreateMutex(NULL, FALSE, NULL); // ����������
	parameters* p = (parameters*)useparameter;
	SOCKADDR_IN serverAddr = p->serverAddr;
	SOCKET clientSocket = p->clientSocket;
	int nummessage = p->nummessage;
	int AddrLen = sizeof(serverAddr);

	while (1)
	{
		Message recvMsg;
		int recvByte = recvfrom(clientSocket, (char*)&recvMsg, sizeof(recvMsg), 0, (sockaddr*)&serverAddr, &AddrLen);
		int windowPos2 = recvMsg.AckNum % windowssize;//����һ��ack���Ķ�Ӧ���ݱ����ڴ����е�λ��
		//�ɹ��յ���Ϣ�����У���
		if (recvByte > 0 && recvMsg.check())
		{
			//����յ���һ�����ڷ�Χ�ڵ�ACK���ģ����Ƚ�������Ϊȷ��
            if (recvMsg.AckNum >= start && recvMsg.AckNum < start + windowssize)
            {
				WaitForSingleObject(mutex, INFINITE);  // �ȴ�����ȡ������
                window[windowPos2].acked = true;
				cout << "client�յ�: ack = " << recvMsg.AckNum << "��ACK���ģ���Ӧ���ݱ��ı�ȷ��" << endl;
                //�Ӵ���start��ʼ���������ݱ����Ƿ���ȷ�ϣ�����Ӧ�ػ������ڣ������������ǿ���һֱ���������ܻ���Ϊֹ
				while (start < arrive && window[start % windowssize].acked)
				{
					start++;
				}
				cout <<"[������������յ�ACK���ĺ�] ���ڵ�start��"<<start<<"���ڵ�arrive��"<<arrive<<endl<<endl;
				ReleaseMutex(mutex);                   // �ͷŻ�����
            }
			//�жϽ��������
			if (start == nummessage + 2)
			{
				cout << "\n�ļ��������" << endl;
				finish = true;
				return 0;
			}
		}
		//��û�յ�ACK���Ļ�У��ʧ�ܲ��ԣ����Բ������ȴ�
	}
	CloseHandle(mutex); // ��������
	return 0;
}

//ʵ���ļ�����
void sendFileToServer(string filename, SOCKADDR_IN serverAddr, SOCKET clientSocket)
{
	mutex = CreateMutex(NULL, FALSE, NULL); // ����������
	int starttime = clock();
	string realname = filename;
	filename="�����ļ�\\"+filename;
	//���ļ������ֽ���
	ifstream fin(filename.c_str(), ifstream::binary);
	if (!fin) {
		printf("�޷����ļ���\n");
		return;
	}
	
	//�ļ���ȡ��fileBuffer
	BYTE* fileBuffer = new BYTE[MaxFileSize];
	unsigned int fileSize = 0;
	BYTE byte = fin.get();
	while (fin) {
		fileBuffer[fileSize++] = byte;
		byte = fin.get();
	}
	fin.close();


	int batchNum = fileSize / MaxMsgSize;//����װ���ı�������
	int leftNum = fileSize % MaxMsgSize;//ʣ����������С
	int nummessage;
    //���һ��Ҫ���͵����ݱ�������
	if(leftNum!=0){
		nummessage=batchNum+2;
	}
	else{
		nummessage=batchNum+1;
	}
	
	parameters useparameter;
	useparameter.serverAddr = serverAddr;
	useparameter.clientSocket = clientSocket;
	useparameter.nummessage = nummessage;
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)recvackthread, &useparameter, 0, 0);
	
	int windowPos1 = 0;//���㴰�ڵ�ǰװ�ص���λ��

	int count=0;
    while(1){
		if (arrive < start + windowssize && arrive < nummessage + 2){
			Message datamessage;
			if (arrive == 2){
				//�����ļ������ļ���С���ļ���С����ͷ����size�ֶ��ˣ�������SFileName��־λdatamessage
				datamessage.SrcPort = ClientPORT;
				datamessage.DestPort = RouterPORT;
				datamessage.size = fileSize;
				datamessage.flag += SFileName;
				datamessage.SeqNum = arrive;
				//���ļ�������ǰ�沢��һ��������
				for (int i = 0; i < realname.size(); i++)
					datamessage.data[i] = realname[i];
				datamessage.data[realname.size()] = '\0';
				datamessage.setCheck();
			}
			else if (arrive == batchNum + 3 && leftNum > 0){
				//δ���ص����ݱ��Ķη��ͣ�����������Ͳ��������������
				datamessage.SrcPort = ClientPORT;
				datamessage.DestPort = RouterPORT;
				datamessage.SeqNum = arrive;
				for (int j = 0; j < leftNum; j++)
				{
					datamessage.data[j] = fileBuffer[batchNum * MaxMsgSize + j];
				}
				datamessage.setCheck();
			}
			else{
				//���ص����ݱ��Ķη��ͣ������������ͣ�����Ϊ�˼�㲢û�����ñ�־λ
				//���͵����ݱ��Ĳ�����Ҫ���ñ�־λ��������Ϊ�˷�����
				datamessage.SrcPort = ClientPORT;
				datamessage.DestPort = RouterPORT;
				datamessage.SeqNum = arrive;
				for (int j = 0; j < MaxMsgSize; j++)
				{
					datamessage.data[j] = fileBuffer[count * MaxMsgSize + j];
				}
				datamessage.setCheck();
				count++;
			}
			{
			WaitForSingleObject(mutex, INFINITE);  // �ȴ�����ȡ������
			sendto(clientSocket, (char*)&datamessage, sizeof(datamessage), 0, (sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
			// ���´���
			windowPos1 = datamessage.SeqNum % windowssize;
			window[windowPos1].msg = datamessage;
			window[windowPos1].acked = false;
			window[windowPos1].sendTime = clock();
			arrive++;
			cout << "client����: seq = " << datamessage.SeqNum << ", У��� = " << datamessage.checkNum<<"�����ݱ���"<<endl;
			cout <<"[���������������Ϣ��] ���ڵ�start��"<<start<<"���ڵ�arrive��"<<arrive<<endl<<endl;
			ReleaseMutex(mutex);    // �ͷŻ�����
			}               	
		}

		//��ʱ�ش������鵱ǰ���ڵ����б��ģ��ش�δȷ�ϵı���
		for (int i = 0; i < arrive - start; i++) {
			windowPos1 = (start+i) % windowssize;
			if (!window[windowPos1].acked && (clock() - window[windowPos1].sendTime > MAX_WAIT_TIME)) {
				//�ش�δȷ�ϵ���Ϣ
				WaitForSingleObject(mutex, INFINITE);  // �ȴ�����ȡ������
				Message resendMsg = window[windowPos1].msg;
				sendto(clientSocket, (char*)&resendMsg, sizeof(resendMsg), 0, (sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
				cout << "�����ش�: seq = " << resendMsg.SeqNum << " �����ݱ���" << endl;
				window[windowPos1].sendTime = clock(); // ���·���ʱ��
				ReleaseMutex(mutex);    // �ͷŻ�����
			}
		}
		//����������˳�
		if(finish == true){
			break;
		}
		}
		CloseHandle(hThread);

	//���㴫��ʱ���������
	int endtime = clock();
	cout << "\n�ܴ���ʱ��Ϊ:" << (endtime - starttime) << "ms" << endl;
	cout << "ƽ��������:" << ((float)fileSize) / (endtime - starttime)  << "bytes/ms" << endl << endl;
	delete[] fileBuffer;//�ͷ��ڴ�
	CloseHandle(mutex); // ��������
	return;
}


//ʵ��client���Ĵλ���
bool fourwayhandwave(SOCKET clientSocket, SOCKADDR_IN serverAddr)
{
	//���������initrelseq
	initrelseq=arrive;
	int AddrLen = sizeof(serverAddr);
	Message buffer1;
	Message buffer2;
	Message buffer3;
	Message buffer4;

	//���͵�һ�λ��֣�FIN��ACK��Ч��seq��֮ǰ�ķ��������ݰ�������кţ�֮ǰ�����к�ÿ��+1
	//������0��ʱ��0������1.jpg�ļ��ķ�����˵��seq�����Ѿ�����189��
	buffer1.SrcPort = ClientPORT;
	buffer1.DestPort = RouterPORT;
	buffer1.flag += FIN;
	buffer1.flag += ACK;
	buffer1.SeqNum = initrelseq++;
	buffer1.setCheck();
	int sendByte = sendto(clientSocket, (char*)&buffer1, sizeof(buffer1), 0, (sockaddr*)&serverAddr, AddrLen);
	clock_t buffer1start = clock();
	if (sendByte == 0)
	{
		cout << "client���͵�һ�λ���ʧ�ܣ��˳�" << endl;
		return false;
	}
	cout << "client���͵�һ�λ��֣�"
	<< "Դ�˿�: " << buffer1.SrcPort << ", "
	<< "Ŀ�Ķ˿�: " << buffer1.DestPort << ", "
	<< "���к�: " << buffer1.SeqNum << ", "
	<< "��־λ: " << "[SYN: " << (buffer1.flag & SYN ? "SET" : "NOT SET") 
	<< "] [ACK: " << (buffer1.flag & ACK ? "SET" : "NOT SET") 
	<< "] [FIN: " << (buffer1.flag & FIN ? "SET" : "NOT SET") 
	<< "] [SFileName: " << (buffer1.flag & SFileName ? "SET" : "NOT SET") <<"], "
	<< "У���: " << buffer1.checkNum << endl;
	int resendtimes=0;
	//���յڶ��λ��ֵ���Ϣ
	while (1)
	{
		int recvByte = recvfrom(clientSocket, (char*)&buffer2, sizeof(buffer2), 0, (sockaddr*)&serverAddr, &AddrLen);
		if (recvByte == 0)
		{
			cout << "client�ڶ��λ��ֽ���ʧ��" << endl;
			return false;
		}
		else if (recvByte > 0)
		{
			//�ɹ��յ���Ϣ�����У��͡�ACK��ack
			if ((buffer2.flag & ACK) && buffer2.check() && (buffer2.AckNum == buffer1.SeqNum+1))
			{
				cout << "client���յڶ��λ��ֳɹ�" << endl;
				break;
			}
			else
			{
				//cout << "client�ڶ��λ��ֽ��ճɹ������ʧ��" << endl;
				continue;
			}
		}
		//client���͵�һ�λ��ֳ�ʱ�����·��Ͳ����¼�ʱ
		if (clock() - buffer1start > MAX_WAIT_TIME)
		{
			cout << "client���͵�һ�λ��֣���"<< ++resendtimes <<"�γ�ʱ�������ش�......" << endl;
			int sendByte = sendto(clientSocket, (char*)&buffer1, sizeof(buffer1), 0, (sockaddr*)&serverAddr, AddrLen);
			buffer1start = clock();
			if(sendByte>0){
				cout<<"client���͵�һ�λ����ش��ɹ�"<<endl;
				break;
			}
			else{
				cout<<"client���͵�һ�λ����ش�ʧ��"<<endl;
			}			
		}
		if (resendtimes == MAX_SEND_TIMES)
        {
			cout << "client���͵�һ�λ��ֳ�ʱ�ش��ѵ�������������ʧ��" << endl;
			return false;
        }
	}

	//���յ����λ��ֵ���Ϣ
	while (1)
	{
		int recvByte = recvfrom(clientSocket, (char*)&buffer3, sizeof(buffer3), 0, (sockaddr*)&serverAddr, &AddrLen);
		if (recvByte == 0)
		{
			cout << "client���յ����λ���ʧ��" << endl;
			return false;
		}
		else if (recvByte > 0)
		{
			//�յ���Ϣ�����У��͡�FIN��ACK
			if ((buffer3.flag & ACK)&& (buffer3.flag & FIN) && buffer3.check())
			{
				cout << "client���յ����λ��ֳɹ�" << endl;
				break;
			}
			else
			{
				continue;
			}
		}
	}
	
	//���͵��Ĵλ��ֵ���Ϣ��ACK��Ч��ack���ڵ����λ�����Ϣ��seq+1��seq�Զ����µ�����
	buffer4.SrcPort = ClientPORT;
	buffer4.DestPort = RouterPORT;
	buffer4.flag += ACK;
	buffer4.SeqNum=initrelseq;
	buffer4.AckNum = buffer3.SeqNum+1;
	buffer4.setCheck();
	sendByte = sendto(clientSocket, (char*)&buffer4, sizeof(buffer4), 0, (sockaddr*)&serverAddr, AddrLen);
	if (sendByte == 0)
	{
		cout << "client���͵��Ĵλ���ʧ��" << endl;
		return false;
	}
	
	cout << "client���͵��Ĵλ��֣�"
	<< "Դ�˿�: " << buffer4.SrcPort << ", "
	<< "Ŀ�Ķ˿�: " << buffer4.DestPort << ", "
	<< "���к�: " << buffer4.SeqNum << ", "
	<< "ȷ�Ϻ�: " << (buffer4.flag & ACK ? to_string(buffer4.AckNum) : "N/A") << ", "
	<< "��־λ: " << "[SYN: " << (buffer4.flag & SYN ? "SET" : "NOT SET") 
	<< "] [ACK: " << (buffer4.flag & ACK ? "SET" : "NOT SET") 
	<< "] [FIN: " << (buffer4.flag & FIN ? "SET" : "NOT SET")
	<< "] [SFileName: " << (buffer4.flag & SFileName ? "SET" : "NOT SET") <<"], "
	<< "У���: " << buffer4.checkNum << endl;
	
	//���Ĵλ���֮����ȴ�2MSL����ֹ���һ��ACK��ʧ
	//��ʱclient����TIME_WAIT״̬
	int tempclock = clock();
	cout << "client������2MSL�ĵȴ�ʱ��" << endl;
	Message tmp;
	while (clock() - tempclock < 2 * MAX_WAIT_TIME)
	{
		int recvByte = recvfrom(clientSocket, (char*)&tmp, sizeof(tmp), 0, (sockaddr*)&serverAddr, &AddrLen);
		if (recvByte == 0)
		{
			cout << "TIME_WAIT״̬ʱ�յ�������Ϣ���˳�" << endl;
			return false;
		}
		else if (recvByte > 0)
		{
			sendByte = sendto(clientSocket, (char*)&buffer4, sizeof(buffer4), 0, (sockaddr*)&serverAddr, AddrLen);
			cout << "TIME_WAIT״̬ʱ�������һ��ACK��ʧ���ط�" << endl;
		}
	}
	cout << "\nclient�ر����ӳɹ���" << endl;
	return true;
}


int main()
{
	
	//��ʼ��Winsock����
	WSADATA wsaDataStruct;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaDataStruct);
	if (result != 0) {
		cout << "��ʼ��Winsock����ʧ��" << endl;
		return -1;
	}
	if (wsaDataStruct.wVersion != MAKEWORD(2, 2)) {
		cout << "��֧�������Winsock�汾" << endl;
		WSACleanup(); 
		return -1;
	}
	cout << "��ʼ��Winsock����ɹ�" << endl; 

	//����socket����UDP�׽��֣�UDP��һ�������ӵ�Э��
	SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "����socketʧ��" << endl;
		return -1;
	}

	// �����׽���Ϊ������ģʽ�����ǹؼ���һ��
	unsigned long mode = 1;
	if (ioctlsocket(clientSocket, FIONBIO, &mode) != NO_ERROR)
	{
		cerr << "�޷������׽���Ϊ������ģʽ��\n" << endl;
		closesocket(clientSocket); 
		return -1;
	}
	cout << "����socket�ɹ�" << endl;

	//��ʼ��·������ַ
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;//ʹ��IPv4��ַ
	serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); 
	serverAddr.sin_port = htons(RouterPORT); 

	//��ʼ���ͻ��˵�ַ
	SOCKADDR_IN clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); 
	clientAddr.sin_port = htons(ClientPORT); 
	
	//bind
	bind(clientSocket, (LPSOCKADDR)&clientAddr, sizeof(clientAddr));
	
	//��������
	bool connected = threewayhandshake(clientSocket, serverAddr);
	if (!connected) {
		cerr << "client����ʧ�ܣ��˳�" << endl;
		return -1;
	}

	//��ʼ��һ�´����������
	for (int i = 0; i < windowssize; i++) {
        window[i].acked = false;
        window[i].sendTime = std::numeric_limits<clock_t>::max();
    }
	//������Ƶ��Ƿ���һ���ļ��ͻ��˳�
	string filename;
	cout << "������Ҫ���͵��ļ�����" << endl;
	cin >> filename;
	cout << endl;
	sendFileToServer(filename, serverAddr, clientSocket);


	//�Ͽ�����
	cout << "client���Ͽ�����" << endl;
	bool breaked = fourwayhandwave(clientSocket, serverAddr);
	if (!breaked) {
		cerr << "client�Ͽ�����ʧ�ܣ��˳�" << endl;
		return -1;
	}
	
	closesocket(clientSocket);
	WSACleanup();
	system("pause");
	return 0;
}