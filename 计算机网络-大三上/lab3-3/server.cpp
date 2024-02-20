#include <iostream>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fstream>
#include "Message.h"

#pragma comment (lib, "ws2_32.lib")

using namespace std;



//����ʵ��pptԭ��ͼ�����ʵ����client��ֱ�ӷ��͵�router���ٽ���ת����server
//���Կ��Լ����Ϊ����server��˵router����˫��ͨ���еġ�client��
const int ServerPORT = 40460; 
const int RouterPORT = 30000; 


// ���崰�ڴ�СΪ10����client�˵Ĵ��ڴ�Сһ��
#define windowssize 10
//����ʵ��server�˵Ľṹ��
struct MessageState {
    Message msg;
    bool received; // �Ƿ��ѽ��յ�
};
MessageState window[windowssize];

//�������ڵĿ�ʼ
int start = 0;

int initrelseq = 0;

BYTE* fileBuffer;

//ʵ��server����������
bool threewayhandshake(SOCKET serverSocket, SOCKADDR_IN clientAddr)
{
	int AddrLen = sizeof(clientAddr);
	Message buffer1,buffer2,buffer3;
	int resendtimes = 0;

	while (true)
	{
		//���յ�һ�����ֵ���Ϣ
		int recvByte = recvfrom(serverSocket, (char*)&buffer1, sizeof(buffer1), 0, (sockaddr*)&clientAddr, &AddrLen);
		
		if (recvByte > 0)
		{
			//�ɹ��յ���Ϣ�����SYN�������
			if (!(buffer1.flag & SYN) || !buffer1.check())
			{
				cout << "server���յ�һ�����ֳɹ������ʧ��" << endl;
				return false;
			}
			cout << "server���յ�һ�����ֳɹ�"<< endl;

			//���͵ڶ������ֵ���Ϣ��SYN��ACK��Ч��seq=y�����seqҲ��0��ack=x+1��Ҳ����1����
			//�ҵ�Э�����˫�˵�seq�Ǹ���ά���ģ��������໥Ӱ�죬����������y���������seq����0��
			buffer2.SrcPort = ServerPORT;
			buffer2.DestPort = RouterPORT;
			buffer2.SeqNum = initrelseq;
			buffer2.AckNum = buffer1.SeqNum+1;
			buffer2.flag += SYN;
			buffer2.flag += ACK;
			buffer2.setCheck();

			int sendByte = sendto(serverSocket, (char*)&buffer2, sizeof(buffer2), 0, (sockaddr*)&clientAddr, AddrLen);
			clock_t buffer2start = clock();
			
			if (sendByte == 0)
			{
				cout << "server���͵ڶ�������ʧ��" << endl;
				return false;
			}
			cout << "server���͵ڶ������֣�"
			<< "Դ�˿�: " << buffer2.SrcPort << ", "
			<< "Ŀ�Ķ˿�: " << buffer2.DestPort << ", "
			<< "���к�: " << buffer2.SeqNum << ", "
			<< "ȷ�Ϻ�: " << (buffer2.flag & ACK ? to_string(buffer2.AckNum) : "N/A") << ", "
			<< "��־λ: " << "[SYN: " << (buffer2.flag & SYN ? "SET" : "NOT SET") 
			<< "] [ACK: " << (buffer2.flag & ACK ? "SET" : "NOT SET") 
			<< "] [FIN: " << (buffer2.flag & FIN ? "SET" : "NOT SET") << "], "
			<< "У���: " << buffer2.checkNum << endl;

			
			//���յ��������ֵ���Ϣ
			while (true)
			{
				int recvByte = recvfrom(serverSocket, (char*)&buffer3, sizeof(buffer3), 0, (sockaddr*)&clientAddr, &AddrLen);
				if (recvByte > 0)
				{
					//�ɹ��յ���Ϣ�����ACK��У��͡�ack
					if ((buffer3.flag & ACK) && buffer3.check() && (buffer3.AckNum == buffer2.SeqNum+1))
					{
						initrelseq++;
						cout << "server���յ��������ֳɹ�"<< endl;
						cout << "server���ӳɹ���" << endl;
						return true;
					}
					else
					{
						cout << "server���յ��������ֳɹ������ʧ��" << endl;
						return false;
					}
				}

				//server���͵ڶ������ֳ�ʱ�����·��Ͳ����¼�ʱ
				if (clock() - buffer2start > MAX_WAIT_TIME)
				{
					if (++resendtimes > MAX_SEND_TIMES) {
						cout << "server���͵ڶ������ֳ�ʱ�ش��ѵ�������������ʧ��" << endl;
						return false;
					}			
					cout << "server���͵ڶ������֣���"<< resendtimes <<"�γ�ʱ�������ش�......" << endl;
					int sendByte = sendto(serverSocket, (char*)&buffer2, sizeof(buffer2), 0, (sockaddr*)&clientAddr, AddrLen);
					buffer2start = clock();
					if(sendByte>0){
						cout<<"server���͵ڶ��������ش��ɹ�"<<endl;
					}
					else{
						cout<<"server�ڶ��������ش�ʧ��"<<endl;
					}
				}
			}
		}
	}
	return false;
}

void sendAck(int seqNum, SOCKET serverSocket, SOCKADDR_IN clientAddr) {
	Message replyMessage;
	replyMessage.SrcPort = ServerPORT;
	replyMessage.DestPort = RouterPORT;
	replyMessage.flag += ACK;
	replyMessage.SeqNum=initrelseq++;
	replyMessage.AckNum = seqNum;
	replyMessage.setCheck();
	sendto(serverSocket, (char*)&replyMessage, sizeof(replyMessage), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
	cout << "server���ͣ�seq = "<<replyMessage.SeqNum<<"��ack = " << replyMessage.AckNum << " ��ACK����"<<endl;
}

void processReceivedMessage(Message& Msgneedprocess,int batchNum,int leftNum){
	int numofmessage = batchNum+bool(leftNum);
	if(Msgneedprocess.flag & SFileName){
		return;
	}
	if(!bool(leftNum)){
		for (int j = 0; j < MaxMsgSize; j++)
		{
			fileBuffer[(Msgneedprocess.SeqNum-3) * MaxMsgSize + j] = Msgneedprocess.data[j];
		}
	}
	else{
		if(Msgneedprocess.SeqNum < numofmessage+2){
			for (int j = 0; j < MaxMsgSize; j++)
			{
				fileBuffer[(Msgneedprocess.SeqNum-3) * MaxMsgSize + j] = Msgneedprocess.data[j];
			}			
		}
		else{
			for (int j = 0; j < leftNum; j++)
			{
				fileBuffer[batchNum * MaxMsgSize + j] = Msgneedprocess.data[j];
			}			
		}
	}
}

//ʵ���ļ�����
void recvFileFormClient(SOCKET serverSocket, SOCKADDR_IN clientAddr)
{
	int AddrLen = sizeof(clientAddr);
	// ��ʼ������
    for (int i = 0; i < windowssize; i++) {
        window[i].received = false;
    }
	start=initrelseq+1;//��ʼ�����ڣ���client��ͬ����
	Message recvMsg;
	//�����ļ������ļ���С
	unsigned int fileSize;
	char fileName[40] = {0};

	int batchNum =0;//����װ���ı�������
	int leftNum =0;//ʣ����������С
	while (1)
	{
		//һֱ����
		int recvByte = recvfrom(serverSocket, (char*)&recvMsg, sizeof(recvMsg), 0, (sockaddr*)&clientAddr, &AddrLen);
		int windowPos = recvMsg.SeqNum % windowssize;
		
		//��������Ƶ��ǻ���rdt3.0Э�飬��ƽ�ackֵ��Ϊ�������ĵ�seqֵ
		if (recvByte > 0 && recvMsg.check() )
		{
			//����������[start,start+windowssize-1]��������ݱ��ģ��ظ���ӦACK���ġ�
			//����ʧ����飬���򵽴�ķ����һ������������ǰ����
			if( recvMsg.SeqNum >= start && recvMsg.SeqNum < start + windowssize ){
				//�����װ���ļ������ļ���С�����ݱ���
				if (recvMsg.flag & SFileName)
				{
					window[windowPos].msg = recvMsg;
					window[windowPos].received = true;				
					fileSize = recvMsg.size;
					fileBuffer = new BYTE[fileSize];
					for (int i = 0; recvMsg.data[i]; i++)
						fileName[i] = recvMsg.data[i];
					batchNum = fileSize / MaxMsgSize;//����װ���ı�������
					leftNum = fileSize % MaxMsgSize;//ʣ����������С
					cout << "\n�����ļ�����" << fileName << "���ļ���С��" << fileSize <<endl<<endl;
					cout << "server�յ���seq = " << recvMsg.SeqNum << "�����ݱ���"<<endl;
					cout <<"[����������յ����ݱ��ĺ�] ���ڵ�start��"<<start<<endl;
					sendAck(recvMsg.SeqNum, serverSocket, clientAddr);
				}
				//ʣ�����ݱ���
				else{
					window[windowPos].msg = recvMsg;
					window[windowPos].received = true;				
					cout << "server�յ���seq = " << recvMsg.SeqNum << "�����ݱ���"<<endl;
					cout <<"[����������յ����ݱ��ĺ�] ���ڵ�start��"<<start<<endl;
					sendAck(recvMsg.SeqNum, serverSocket, clientAddr);
				}
			}
			//����������[start-windowssize,start-1]���䣬�����ظ�һ����Ӧ��ACK����
			else if(recvMsg.SeqNum < start && recvMsg.SeqNum >= start - windowssize) {
				sendAck(recvMsg.SeqNum, serverSocket, clientAddr);
			}
			//���������������Χ�����ݱ��Ķ�ֱ�Ӷ�����ʲôҲ����Ҫ����
			else{
				continue;
			}
			// ��鲢��������
			while (window[start % windowssize].received) {
				window[start % windowssize].received = false;
				processReceivedMessage(window[start % windowssize].msg,batchNum,leftNum);
				cout<<"seqֵΪ"<<window[start % windowssize].msg.SeqNum<<"�����ݱ����ѽ����ϲ�Ӧ�ý�һ������"<<endl;
				start++;
			}
			cout <<"[������������Ի������ں�] ���ڵ�start��"<<start<<endl<<endl;
			//����Ƿ������
			if(start == batchNum + bool(leftNum) + 3){
				break;
			}
		}
	}

	//д���ļ�
	cout << "\n�ļ�����ɹ�����ʼд���ļ�" << endl;
	FILE* outputfile;
	outputfile = fopen(fileName, "wb");
	if (fileBuffer != 0)
	{
		fwrite(fileBuffer, fileSize, 1, outputfile);
		fclose(outputfile);
	}
	cout << "�ļ�д��ɹ�" << endl<<endl;
	delete[] fileBuffer;//�ͷ��ڴ�
}


//ʵ��server���Ĵλ���
bool fourwayhandwave(SOCKET serverSocket, SOCKADDR_IN clientAddr)
{
	int AddrLen = sizeof(clientAddr);
	Message buffer1;
	Message buffer2;
	Message buffer3;
	Message buffer4;
	while (1)
	{
		//���յ�һ�λ��ֵ���Ϣ
		int recvByte = recvfrom(serverSocket, (char*)&buffer1, sizeof(buffer1), 0, (sockaddr*)&clientAddr, &AddrLen);
		if (recvByte == 0)
		{
			cout << "��һ�λ��ֽ���ʧ�ܣ��˳�" << endl;
			return false;
		}

		else if (recvByte > 0)
		{
			//���FIN��ACK�������
			if (!(buffer1.flag & FIN) || !(buffer1.flag & ACK) || !buffer1.check())
			{
				cout << "��һ�λ��ֽ��ճɹ������ʧ��" << endl;
				return false;
			}
			cout << "server���յ�һ�λ��ֳɹ�" << endl;

			//���͵ڶ��λ��ֵ���Ϣ��ACK��Ч��ack�ǵ�һ�λ�����Ϣ��seq+1,seqֵ�Զ����µ�����
			buffer2.SrcPort = ServerPORT;
			buffer2.DestPort = RouterPORT;
			buffer2.SeqNum = initrelseq++;
			buffer2.AckNum = buffer1.SeqNum+1;
			buffer2.flag += ACK;
			buffer2.setCheck();//����У���
			int sendByte = sendto(serverSocket, (char*)&buffer2, sizeof(buffer2), 0, (sockaddr*)&clientAddr, AddrLen);
			clock_t buffer2start = clock();
			if (sendByte == 0)
			{
				cout << "server���͵ڶ��λ���ʧ��" << endl;
				return false;
			}
			cout << "server���͵ڶ��λ��֣�"
			<< "Դ�˿�: " << buffer2.SrcPort << ", "
			<< "Ŀ�Ķ˿�: " << buffer2.DestPort << ", "
			<< "���к�: " << buffer2.SeqNum << ", "
			<< "ȷ�Ϻ�: " << (buffer2.flag & ACK ? to_string(buffer2.AckNum) : "N/A") << ", "
			<< "��־λ: " << "[SYN: " << (buffer2.flag & SYN ? "SET" : "NOT SET") 
			<< "] [ACK: " << (buffer2.flag & ACK ? "SET" : "NOT SET") 
			<< "] [FIN: " << (buffer2.flag & FIN ? "SET" : "NOT SET")
			<< "] [SFileName: " << (buffer2.flag & SFileName ? "SET" : "NOT SET") <<"], "
			<< "У���: " << buffer2.checkNum << endl;
			break;
		}
	}

	//server���͵����λ��ֵ���Ϣ��FIN��ACK��Ч��seq�Զ����µ�����
	buffer3.SrcPort = ServerPORT;
	buffer3.DestPort = RouterPORT;
	buffer3.flag += FIN;//����FIN
	buffer3.flag += ACK;//����ACK
	buffer3.SeqNum = initrelseq++;//�������seq
	buffer3.setCheck();//����У���
	int sendByte = sendto(serverSocket, (char*)&buffer3, sizeof(buffer3), 0, (sockaddr*)&clientAddr, AddrLen);
	clock_t buffer3start = clock();
	if (sendByte == 0)
	{
		cout << "server���͵����λ���ʧ��" << endl;
		return false;
	}
	cout << "server���͵����λ��֣�"
	<< "Դ�˿�: " << buffer3.SrcPort << ", "
	<< "Ŀ�Ķ˿�: " << buffer3.DestPort << ", "
	<< "���к�: " << buffer3.SeqNum << ", "
	<< "��־λ: " << "[SYN: " << (buffer3.flag & SYN ? "SET" : "NOT SET") 
	<< "] [ACK: " << (buffer3.flag & ACK ? "SET" : "NOT SET") 
	<< "] [FIN: " << (buffer3.flag & FIN ? "SET" : "NOT SET")
	<< "] [SFileName: " << (buffer3.flag & SFileName ? "SET" : "NOT SET") <<"], "
	<< "У���: " << buffer3.checkNum << endl;
	int resendtimes=0;
	
	//���յ��Ĵλ��ֵ���Ϣ
	while (1)
	{
		int recvByte = recvfrom(serverSocket, (char*)&buffer4, sizeof(buffer4), 0, (sockaddr*)&clientAddr, &AddrLen);
		if (recvByte == 0)
		{
			cout << "server���յ��Ĵλ���ʧ��" << endl;
			return false;
		}
		else if (recvByte > 0)
		{
			//�ɹ��յ���Ϣ�����У��͡�ACK��ack
			if ((buffer4.flag & ACK) && buffer4.check() && (buffer4.AckNum == buffer3.SeqNum+1))
			{
				cout << "server���յ��Ĵλ��ֳɹ�" << endl;
				break;
			}
			else
			{
				cout << "server���յ��Ĵλ��ֳɹ������ʧ��" << endl;
				return false;
			}
		}
		//server���͵����λ��ֳ�ʱ�����·��Ͳ����¼�ʱ
		if (clock() - buffer3start > MAX_WAIT_TIME)
		{
			cout << "server���͵����λ��֣���"<< ++resendtimes <<"�γ�ʱ�������ش�......" << endl;
			int sendByte = sendto(serverSocket, (char*)&buffer3, sizeof(buffer3), 0, (sockaddr*)&clientAddr, AddrLen);
			buffer3start = clock();
			if(sendByte>0){
				cout<<"server���͵����λ����ش��ɹ�"<<endl;
				//break;
				continue;
			}
			else{
				cout<<"server���͵����λ����ش�ʧ��"<<endl;
			}			
		}
		if (resendtimes == MAX_SEND_TIMES)
        {
			cout << "server���͵����λ��ֳ�ʱ�ش��ѵ�������������ʧ��" << endl;
			return false;
        }
	}
	cout << "\nserver�ر����ӳɹ���" << endl;
	return true;
}


int main()
{
	//��ʼ��Winsock����
	WSADATA wsaDataStruct;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaDataStruct);
	if (result != 0) {
		cout << "��ʼ��Winsock����ɹ�" << endl;
		return -1;
	}
	if (wsaDataStruct.wVersion != MAKEWORD(2, 2)) {
		cout << "��֧�������Winsock�汾" << endl;
		WSACleanup(); 
		return -1;
	}
	cout << "��ʼ��Winsock����ɹ�" << endl; 

	//����socket����UDP�׽��֣�UDP��һ�������ӵ�Э��
	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket == INVALID_SOCKET)
	{
		cerr << "����socketʧ��" << endl;
		return -1;
	}

	// �����׽���Ϊ������ģʽ�����ǹؼ���һ��
	unsigned long mode = 1;
	if (ioctlsocket(serverSocket, FIONBIO, &mode) != NO_ERROR)
	{
		cerr << "�޷������׽���Ϊ������ģʽ" << endl;
		closesocket(serverSocket); 
		return -1;
	}
	cout << "����socket�ɹ�" << endl;

	//��ʼ����������ַ
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(ServerPORT); 

	//bind
	int tem = bind(serverSocket, (LPSOCKADDR)&serverAddr, sizeof(serverAddr));
	if (tem == SOCKET_ERROR)
	{
		cout << "bindʧ��" << endl;
		return -1;
	}
	cout << "Server��bind�ɹ���׼������" << endl << endl;

	//��ʼ��·������ַ
	SOCKADDR_IN clientAddr;
	clientAddr.sin_family = AF_INET; 
	clientAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	clientAddr.sin_port = htons(RouterPORT); //�˿ں�

	//��������
	bool isConn = threewayhandshake(serverSocket, clientAddr);
	if (isConn == 0){
		cerr << "server����ʧ�ܣ��˳�" << endl;
		return -1;
	}
	//�����ļ�
	recvFileFormClient(serverSocket, clientAddr);

	//�ر�����
	cout << "server���Ͽ�����" << endl;
	bool breaked = fourwayhandwave(serverSocket, clientAddr);
	if (!breaked) {
		cerr << "server�Ͽ�����ʧ�ܣ��˳�" << endl;
		return -1;
	}
	closesocket(serverSocket);
	WSACleanup();
	system("pause");
	return 0;
}
