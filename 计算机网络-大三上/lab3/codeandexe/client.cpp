#include <iostream>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <fstream>
#include "Message.h"

#pragma comment (lib, "ws2_32.lib")

using namespace std;

//����ʵ��pptԭ��ͼ�����ʵ����client��ֱ�ӷ��͵�router���ٽ���ת����server
//���Կ��Լ����Ϊ����client��˵router����˫��ͨ���еġ�server��
const int RouterPORT = 30000; 
const int ClientPORT = 20230;


int initrelseq = 0;

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
	return true;
}

//ʵ�ֵ������ķ���
bool sendMessage(Message& sendMsg, SOCKET clientSocket, SOCKADDR_IN serverAddr)
{
	sendto(clientSocket, (char*)&sendMsg, sizeof(sendMsg), 0, (sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	cout << "client���ͣ�"
	<< "Դ�˿�: " << sendMsg.SrcPort << ", "
	<< "Ŀ�Ķ˿�: " << sendMsg.DestPort << ", "
	<< "���к�: " << sendMsg.SeqNum << ", "
	<< "��־λ: " << "[SYN: " << (sendMsg.flag & SYN ? "SET" : "NOT SET") 
	<< "] [ACK: " << (sendMsg.flag & ACK ? "SET" : "NOT SET") 
	<< "] [FIN: " << (sendMsg.flag & FIN ? "SET" : "NOT SET") 
	<< "] [SFileName: " << (sendMsg.flag & SFileName ? "SET" : "NOT SET") <<"], "
	<< "У���: " << sendMsg.checkNum << endl;
	int msgStart = clock();
	Message recvMsg;
	int AddrLen = sizeof(serverAddr);
	int resendtimes = 0;

	while (1)
	{
		int recvByte = recvfrom(clientSocket, (char*)&recvMsg, sizeof(recvMsg), 0, (sockaddr*)&serverAddr, &AddrLen);
		if (recvByte > 0)
		{
			//�ɹ��յ���Ϣ�����У��͡�ack���������ȷ���������ȴ���ֱ���������ԵĻ��ߵȵ���ʱ���ش�һ����ȥ��ȷ��һ�����ͱ�����һ��ACK����
			//��һ���ǻ���rdt3.0Э��ʵ�ֵģ�ͬʱ��ACK��ackֵ���м��飬���ŷ��Ͱ���Ҫ��Ӧ����ACK��
			if ((recvMsg.flag & ACK) && (recvMsg.AckNum == sendMsg.SeqNum))
			{
				cout << "client�յ���ack = " << recvMsg.AckNum << "��ACK��";
				return true;
			}
		}
		//����rdt3.0Э�飬��ʱ�ش����ƣ����·��Ͳ����¼�ʱ
		if (clock() - msgStart > MAX_WAIT_TIME)
		{
			cout << "seq = "<<sendMsg.SeqNum << "�ı��ģ���" << ++resendtimes << "�γ�ʱ�������ش�......" << endl;
			int sendByte = sendto(clientSocket, (char*)&sendMsg, sizeof(sendMsg), 0, (sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
			msgStart = clock();
			if(sendByte>0){
				cout<<"�����ش��ɹ�"<<endl;
				break;
			}
			else{
				cout<<"�����ش�ʧ��"<<endl;
			}
		}
		if (resendtimes == MAX_SEND_TIMES)
		{
			cout << "��ʱ�ش��ѵ�������������ʧ��" << endl;
			return false;
		}
	}
	return true;
}


//ʵ���ļ�����
void sendFileToServer(string filename, SOCKADDR_IN serverAddr, SOCKET clientSocket)
{
	
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

	//�����ļ������ļ���С���ļ���С����ͷ����size�ֶ��ˣ�������SFileName��־λ
	Message nameMessage;
	nameMessage.SrcPort = ClientPORT;
	nameMessage.DestPort = RouterPORT;
	nameMessage.size = fileSize;
	nameMessage.flag += SFileName;
	nameMessage.SeqNum = ++initrelseq;
	//���ļ�������ǰ�沢��һ��������
	for (int i = 0; i < realname.size(); i++)
		nameMessage.data[i] = realname[i];
	nameMessage.data[realname.size()] = '\0';
	nameMessage.setCheck();
	if (!sendMessage(nameMessage, clientSocket, serverAddr))
	{
		cout << "װ���ļ������ļ���С�ı��ķ���ʧ��" << endl;
		return;
	}
	cout << "װ���ļ������ļ���С�ı��ķ��ͳɹ�" << endl<<endl;

	int batchNum = fileSize / MaxMsgSize;//����װ���ı�������
	int leftNum = fileSize % MaxMsgSize;//ʣ����������С
	//���ص����ݱ��Ķη��ͣ������������ͣ�����Ϊ�˼�㲢û�����ñ�־λ
	//��Ƶ�Э�飺����һ�����ľ���Ҫserver�ظ�һ��ACK����С������
	//���͵����ݱ��Ĳ�����Ҫ���Ǳ�־λ��������Ϊ�˷�����
	for (int i = 0; i < batchNum; i++)
	{
		Message dataMsg;
		dataMsg.SrcPort = ClientPORT;
		dataMsg.DestPort = RouterPORT;
		dataMsg.SeqNum = ++initrelseq;
		for (int j = 0; j < MaxMsgSize; j++)
		{
			dataMsg.data[j] = fileBuffer[i * MaxMsgSize + j];
		}
		dataMsg.setCheck();
		if (!sendMessage(dataMsg, clientSocket, serverAddr))
		{
			cout << "�������ݱ��ķ���ʧ��" << endl;
			return;
		}
		cout << "��" << i+1 << "���������ݱ��ķ��ͳɹ�" << endl<<endl;
	}
	//δ���ص����ݱ��Ķη��ͣ�����������Ͳ��������������
	if (leftNum > 0)
	{
		Message dataMsg;
		dataMsg.SrcPort = ClientPORT;
		dataMsg.DestPort = RouterPORT;
		dataMsg.SeqNum = ++initrelseq;
		for (int j = 0; j < leftNum; j++)
		{
			dataMsg.data[j] = fileBuffer[batchNum * MaxMsgSize + j];
		}
		dataMsg.setCheck();
		if (!sendMessage(dataMsg, clientSocket, serverAddr))
		{
			cout << "δ���ص����ݱ��ķ���ʧ��" << endl;
			return;
		}
		cout << "δ���ص����ݱ��ķ��ͳɹ�" << endl<<endl;
	}

	//���㴫��ʱ���������
	int endtime = clock();
	cout << "\n�ܴ���ʱ��Ϊ:" << (endtime - starttime) << "ms" << endl;
	cout << "ƽ��������:" << ((float)fileSize) / (endtime - starttime)  << "bytes/ms" << endl << endl;
	delete[] fileBuffer;//�ͷ��ڴ�
	return;
}


//ʵ��client���Ĵλ���
bool fourwayhandwave(SOCKET clientSocket, SOCKADDR_IN serverAddr)
{
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
	buffer1.SeqNum = ++initrelseq;
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
				//cout << "client���յ����λ���ʧ��" << endl;
				continue;
			}
		}
	}
	

	
	//���͵��Ĵλ��ֵ���Ϣ��ACK��Ч��ack���ڵ����λ�����Ϣ��seq+1��seq�Զ����µ�����
	buffer4.SrcPort = ClientPORT;
	buffer4.DestPort = RouterPORT;
	buffer4.flag += ACK;
	buffer4.SeqNum=++initrelseq;
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