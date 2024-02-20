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




int initrelseq = 0;


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

//ʵ�ֵ������Ľ���
bool recvMessage(Message& recvMsg, SOCKET serverSocket, SOCKADDR_IN clientAddr)
{
	int AddrLen = sizeof(clientAddr);
	while (1)
	{
		int recvByte = recvfrom(serverSocket, (char*)&recvMsg, sizeof(recvMsg), 0, (sockaddr*)&clientAddr, &AddrLen);
		if (recvByte > 0)
		{
			//�ɹ��յ���Ϣ���ظ�ACK����
			if (recvMsg.check() && (recvMsg.SeqNum == initrelseq + 1))
			{
				Message replyMessage;
				replyMessage.SrcPort = ServerPORT;
				replyMessage.DestPort = RouterPORT;
				replyMessage.flag += ACK;
				replyMessage.SeqNum=initrelseq++;
				replyMessage.AckNum = recvMsg.SeqNum;
				replyMessage.setCheck();
				sendto(serverSocket, (char*)&replyMessage, sizeof(replyMessage), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "server�յ���seq = " << recvMsg.SeqNum << "�����ݱ���"<< endl;
				cout << "server���ͣ�seq = "<<replyMessage.SeqNum<<"��ack = " << replyMessage.AckNum << "��ACK���ģ������"<<replyMessage.checkNum << endl;
				return true;
			}

			//���seqֵ����ȷ���������ģ������ۼ�ȷ�ϵ�ACK���ģ�ack=initrelseq-1��
			else if (recvMsg.check() && (recvMsg.SeqNum != initrelseq + 1))
			{
				Message replyMessage;
				replyMessage.SrcPort = ServerPORT;
				replyMessage.DestPort = RouterPORT;
				replyMessage.flag += ACK;
				replyMessage.SeqNum = initrelseq;
				replyMessage.AckNum = initrelseq;
				replyMessage.setCheck();
				sendto(serverSocket, (char*)&replyMessage, sizeof(replyMessage), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "[�ۼ�ȷ�ϣ�����seqֵ��] server�յ� seq = " << recvMsg.SeqNum << "�����ݱ��ģ������� ack = " << replyMessage.AckNum << " ���ۼ�ȷ�ϵ�ACK����" << endl;
			}
		}
		else if (recvByte == 0)
		{
			return false;
		}
	}
	return true;
}

//ʵ���ļ�����
void recvFileFormClient(SOCKET serverSocket, SOCKADDR_IN clientAddr)
{
	int AddrLen = sizeof(clientAddr);
	//�����ļ������ļ���С
	Message nameMessage;
	unsigned int fileSize;
	char fileName[40] = {0};
	while (1)
	{
		int recvByte = recvfrom(serverSocket, (char*)&nameMessage, sizeof(nameMessage), 0, (sockaddr*)&clientAddr, &AddrLen);
		if (recvByte > 0)
		{
			//����ɹ��յ�װ���ļ������ļ���С����Ϣ�������һ��������ظ���Ӧ��ACK����
			//��������Ƶ��ǻ���rdt3.0Э�飬��ackֵ��Ϊ�������ĵ�seqֵ
			if (nameMessage.check() && (nameMessage.SeqNum == initrelseq + 1) && (nameMessage.flag & SFileName))
			{
				fileSize = nameMessage.size;
				for (int i = 0; nameMessage.data[i]; i++)
					fileName[i] = nameMessage.data[i];
				cout << "\n�����ļ�����" << fileName << "���ļ���С��" << fileSize << endl<<endl;
				
				Message replyMessage;
				replyMessage.SrcPort = ServerPORT;
				replyMessage.DestPort = RouterPORT;
				replyMessage.flag += ACK;
				replyMessage.SeqNum=initrelseq++;
				replyMessage.AckNum = nameMessage.SeqNum;
				replyMessage.setCheck();
				sendto(serverSocket, (char*)&replyMessage, sizeof(replyMessage), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "server�յ���seq = " << nameMessage.SeqNum << "�����ݱ���"<<endl;
				cout << "server���ͣ�seq = "<<replyMessage.SeqNum<<"��ack = " << replyMessage.AckNum << " ��ACK���ģ��������" <<replyMessage.checkNum<< endl<<"װ���ļ������ļ���С�ı��Ľ��ճɹ������濪ʼ��ʽ�������ݶ�"<< endl<< endl;
				break;
			}

			//���seqֵ����ȷ���������ģ������ۼ�ȷ�ϵ�ACK���ģ�ack=initrelseq-1��
			else if (nameMessage.check() && (nameMessage.SeqNum != initrelseq + 1) && (nameMessage.flag & SFileName))
			{
				Message replyMessage;
				replyMessage.SrcPort = ServerPORT;
				replyMessage.DestPort = RouterPORT;
				replyMessage.flag += ACK;
				replyMessage.SeqNum=initrelseq+1;
				replyMessage.AckNum = initrelseq;
				replyMessage.setCheck();
				sendto(serverSocket, (char*)&replyMessage, sizeof(replyMessage), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "[�ۼ�ȷ�ϣ�����seqֵ��]server�յ� seq = " << nameMessage.SeqNum << "�����ݱ��ģ������� ack = " << replyMessage.AckNum << "��ACK����" << endl;
			}
		}
	}


	int batchNum = fileSize / MaxMsgSize;//����װ���ı�������
	int leftNum = fileSize % MaxMsgSize;//ʣ����������С
	BYTE* fileBuffer = new BYTE[fileSize];

	//���ص����ݱ��Ķν���
	for (int i = 0; i < batchNum; i++)
	{
		Message dataMsg;
		if (recvMessage(dataMsg, serverSocket, clientAddr))
		{
			cout << "��" << i+1 << "���������ݱ��Ľ��ճɹ�" << endl<< endl;
		}
		else
		{
			cout << "��" << i+1 << "���������ݱ��Ľ���ʧ��" << endl<< endl;
			return;
		}
		//��ȡ����
		for (int j = 0; j < MaxMsgSize; j++)
		{
			fileBuffer[i * MaxMsgSize + j] = dataMsg.data[j];
		}
	}

	//δ���ص����ݱ��Ķν��գ�����������Ͳ��������������
	if (leftNum > 0)
	{
		Message dataMsg;
		if (recvMessage(dataMsg, serverSocket, clientAddr))
		{
			cout << "δ���ص����ݱ��Ľ��ճɹ�" << endl<< endl;
		}
		else
		{
			cout << "δ���ص����ݱ��Ľ���ʧ��" << endl<< endl;
			return;
		}
		//��ȡ����
		for (int j = 0; j < leftNum; j++)
		{
			fileBuffer[batchNum * MaxMsgSize + j] = dataMsg.data[j];
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
