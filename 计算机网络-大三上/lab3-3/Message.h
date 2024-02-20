#include <iostream>

using namespace std;


#define MAX_WAIT_TIME  3000 //��ʱʱ�ޣ���λms��
#define MAX_SEND_TIMES  10 //����ش�����
#define MaxFileSize 15000000 //����ļ���С���鿴��Ҫ���Ե��ļ���ȷ��
#define MaxMsgSize 10000 //������ݶδ�С

//���ò�ͬ�ı�־λ���õ��İ���SYN��ACK��FIN��SFileName��ʾ�������ļ�����
const unsigned short SYN = 0x1;//0001
const unsigned short ACK = 0x2;//0010
const unsigned short FIN = 0x4;//0100
const unsigned short SFileName = 0x8;//1000

//�޸ı�����Ĭ�ϵ����ݳ�Ա���뷽ʽ��Ϊ1�ֽڣ�ȷ�����еĳ�Ա�����������
#pragma pack(1)
struct Message
{
	//ͷ����һ��28�ֽڣ�
	//ԴIP��Ŀ��IP
	unsigned int SrcIP, DestIP;//4�ֽڡ�4�ֽ�
	//Դ�˿ںš�Ŀ�Ķ˿ں�
	unsigned short SrcPort, DestPort;//2�ֽڡ�2�ֽ�
	//��ţ������ʾ������к�
	unsigned int SeqNum;//4�ֽ�
	//ȷ�Ϻ�
	unsigned int AckNum;//4�ֽ�
	//���ݶδ�С
	unsigned int size;//4�ֽ�
	//��־λ
	unsigned short flag;//2�ֽ�
	//У���
	unsigned short checkNum;//2�ֽ�
	//�������ݶΣ�ʵ��Ҫ��һ�����ܳ���15000�ֽڣ��������趨������10000�ֽ�
	BYTE data[MaxMsgSize];
	//������ʵ�ֵķ���
	Message();
	bool check();
	void setCheck();
};


#pragma pack()//�ָ���������Ĭ�ϵĶ��뷽ʽ
//���캯������ʼ��ȫΪ0
Message::Message()
{
	SrcIP = 0;
	DestIP = 0;
	//����ʵ��ʹ�õ��Ǳ��ػػ���ַ��127.0.0.1��������IP�����ֶ�û����
	SeqNum = 0;
	AckNum = 0;
	size = 0;
	flag = 0;
	memset(&data, 0, sizeof(data));
}
/*
��֤Message�ṹ���У����Ƿ���ȷ��
������õ�����ͬ����2�ֽڣ�16λ��
�ú���ͨ���������ṹ�����У����ֶν��ж˽�λ�ӷ�������֤���ݵ������ԡ�
����ڽ�����16λ������֮�󣬼���õ���32λ�͵ĵ�16λȫΪ1��
��˵��У�������ȷ�ģ������ڴ����洢������δ�����仯������true��
�����16λ��ȫΪ1����У�����֤ʧ�ܣ�����false��
*/
bool Message::check()
{

	unsigned int sum = 0;
	unsigned short* msgStream = (unsigned short*)this;

	for (int i = 0; i < sizeof(*this) / 2; i++)
	{
		sum += *msgStream++;
		if (sum & 0xFFFF0000)
		{
			sum &= 0xFFFF;
			sum++;
		}
	}
	if ((sum & 0xFFFF) == 0xFFFF)
	{
		return true;
	}
	return false;

}
/*
���㲢����Message�ṹ���У��͡�
������õ�������2�ֽڣ�16λ��
У��ͼ�����ѭ������У��͵ı�׼���㷽����ͨ���Խṹ����ÿ16λ���ж˽�λ�ӷ���
���Ƚ�У����ֶ����㣬Ȼ��Խṹ������ಿ��ִ�ж˽�λ�ӷ���
��󽫼���õ���32λ�͵ĵ�16λȡ�����洢ΪУ��͡�
��ȷ�����ڴ����洢���������ݰ��������Կ��Ա����շ���֤��
 */
void Message::setCheck()
{
	this->checkNum = 0;
	int sum = 0;
	unsigned short* msgStream = (unsigned short*)this;


	for (int i = 0; i < sizeof(*this) / 2; i++)
	{
		sum += *msgStream++;
		if (sum & 0xFFFF0000)
		{
			sum &= 0xFFFF;
			sum++;
		}
	}
	this->checkNum = ~(sum & 0xFFFF);

}


