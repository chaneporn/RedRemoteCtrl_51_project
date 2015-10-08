/********************************* �����к�̫�������޹�˾ *******************************
* ʵ �� �� �������ֵ��¼�ͷ���
* ʵ��˵�� ����¼������38K�ز����͵ĺ����źŴ洢���ⲿ�洢оƬ����ӳ�䵽������̣����°���ʱ������֮ǰ��¼�ĺ����ź�
* ʵ��ƽ̨ ����̫51��Ƭ�������� V1.1
* ���ӷ�ʽ ������ñCN4 ����2,4��� ����ñCN20 ����2,4��ӣ�����ñCN3 ����1,3���
* ע    �� ������ͨ���ضϺʹ�TR1��ʵ�ַ��͵͵�ƽ�͸ߵ�ƽ����TR1ʱ������38k�ߵ�ƽ�ز������ն˾����������յ��͵�ƽ��
*            �ض�TR1ʱ�����Ͷ˲������ز������ն˾��������յ��ߵ�ƽ
* ��    �� ����̫���Ӳ�Ʒ�з���    QQ ��1909197536
* ��    �� ��http://shop120013844.taobao.com/
****************************************************************************************/
#include <reg52.h> 
#include "AT24C02.h"
#include <stdio.h>
#include <string.h>

#define FOSC 11059200L //�������ã�Ĭ��ʹ��11.0592M Hz
#define KEY_BLOCK (sizeof(struct KEYMSG)) //�洢һ����ֵ����Ϣ��Ҫ�Ŀռ��С
//IO�ӿڶ���
sbit IR_OUT=P1^6 ;
//sbit dula=P2^6;
//sbit wela=P2^7;
sbit IRIN = P3^2;         //���������������
sbit BEEP = P1^5;         //������������
sbit LED_read = P1^0;
sbit LED_ok = P1^1;
sbit LED_run = P1^2;

#define KEY_PORT P3
//ϵͳ״̬����
#define SEND_RED 0  //Ĭ�ϵķ���״̬
#define READ_RED 1  //��ȡ�����ź�״̬
#define READ_OK  2  // ��ȡ�ɹ����ȴ�����ƥ��״̬

struct KEYMSG{
	unsigned char key_value;//��Ӧ��������̼�ֵ
	unsigned int count_bit;//�ú����źŸߵ͵�ƽ�ܸ�����һ����bit*2
	unsigned char value_msg[40];//��¼�ߵ͵�ƽ
	unsigned char  time[640];//��¼ÿһ����ƽ����ʱ��,ÿ����һ�飬�ֱ���TH0��TL0
};

volatile struct KEYMSG key_info;
data volatile unsigned char sys_status = SEND_RED;//ϵͳ��ǰ״̬

unsigned char KeyScan();
void beep();
void ReadKeyMsg(struct KEYMSG *key_msg);
void WriteKeyMsg(struct KEYMSG *key_msg);
void SendKeyMsg(struct KEYMSG *key_msg);
void PrintDebugMsg();

/*******************************************************************************
* �� �� �� ��Delayms
* �������� ��ʵ�� ms������ʱ
* ��    �� ��ms
* ��    �� ����
*******************************************************************************/
void Delayms(unsigned int ms)
{
	unsigned int i,j;
	for(i=0;i<ms;i++)
	#if FOSC == 11059200L
		for(j=0;j<114;j++);
	#elif FOSC == 12000000L
	  for(j=0;j<123;j++);
	#elif FOSC == 24000000L
		for(j=0;j<249;j++);
	#else
		for(j=0;j<114;j++);
	#endif
}

/*******************************************************************************
* �� �� �� ��Init_Timer
* �������� ����ʱ��1��ʼ�� 13us����һ���ж� ���ڲ���38K�ز� 
* ��    �� ����
* ��    �� ����
*******************************************************************************/
void Init_Timer(void) 
{ 
	TMOD=0x21;	 //T0 mode 1      T1 mode 2  	
	TH1=256-(1000*11.02/38.0/12)/2+0.5;						
	//����
	TL1=TH1;
	ET1=1; 
	TR1=0;
//	EA=1; 
} 

/*******************************************************************************
* �� �� �� ��Exit0Init
* �������� �����ж�0��ʼ������
* ��    �� ����
* ��    �� ����
*******************************************************************************/
void Exit0Init()
{
	EX0 = 0;	//���� INT1 �ⲿ�ж�

    IT0 = 1;	// ������ʽΪ���帺���ش���
 //   EA = 1;//���ж�
}   

void UartInit()
{
	SCON |= 0x50;
	TL2 = RCAP2L = (65536-(FOSC/32/9600));
	TH2 = RCAP2H = (65536-(FOSC/32/9600))>>8;
	T2CON = 0x34;
	//ES = 1;
}

void PutSring(char *buf)
{
	int i;
	for(i=0;i<strlen(buf);i++)
	{
		SBUF = buf[i];//д��SBUF����ʼ���ͣ�������Զ������жϷ���
		while(!TI);		  //�ȴ������������
		TI=0;			  //���������ɱ�־λ
	}
}

/*******************************************************************************
* �� �� �� ��putchar
* �������� ������ϵͳ�Դ���putchar������ʵ��printf����
* ��    �� ��Ҫ���͵Ĳ���
* ��    �� �����ͳɹ��Ĳ���
*******************************************************************************/
char putchar(char ch)
{ 
	/* Place your implementation of fputc here */ 
	SBUF=(unsigned char)ch; //�����յ������ݷ��뵽���ͼĴ���
	while(!TI);		  //�ȴ������������
	TI=0;		 //���������ɱ�־λ	
	return ch;
}

/*******************************************************************************
* �� �� �� ��main
* �������� ��������
* ��    �� ����
* ��    �� ����
*******************************************************************************/
void main(void) 
{ 
  unsigned char date = 0;
//	xdata unsigned int aa;
	IR_OUT=1;
	UartInit();
	Init_Timer(); 
	X24c02Init();
	Exit0Init();
	LED_read = 1;
	LED_ok = 1;
	EA = 1;//�����ж�
//	printf("system init OK\r\n");
	
//			TH0 = 0;
//		TL0 = 0;
//		TR0 = 1;
//	Delayms(20);
//	TR0 = 0;
//	printf("test TH0 %d\r\n",(int)TH0);
//	printf("test TL0 %d\r\n",(int)TL0);
	//aa = TL0;
	//aa = TH0*256;
//	aa = (unsigned int)(TH0*256 + TL0);
//	printf("test value %u\r\n",aa);
//	aa = aa;



//key_info.key_value = 1;
//key_info.count_bit = 67;
//key_info.value_msg[0] = 85;key_info.value_msg[1] = 85;key_info.value_msg[2] = 85;key_info.value_msg[3] = 85;
//key_info.time[0] = 8315;key_info.time[1] = 8315;key_info.time[2] = 8315;key_info.time[3] = 8315;key_info.time[4] = 8315;key_info.time[5] = 8315;
//SendKeyMsg(&key_info);

	while(1) 
	{	
		date = 0xff;
		LED_run = !LED_run;
		Delayms(20);
		if(sys_status != READ_RED)
		{
			date = KeyScan();
		}
		if(date != 0xff)
		{
			if(date == 16)//�����ģʽ
			{
				memset(&key_info.key_value , 0 , sizeof(struct KEYMSG));//���buf
				sys_status = READ_RED;//��������ȡ״̬
				LED_read = 0;				
				IE0 = 0;
				EX0 = 1;//���ⲿ�ж�
			}
			
			if(sys_status == SEND_RED)//�Ӵ洢����ȡ��������ͨ�����ⷢ�ͷ��ͼ�ֵ
			{
				if(date <=10)//������10��ֵ
				{
					key_info.key_value = date;
					ReadKeyMsg(&key_info);
					//PrintDebugMsg();
					SendKeyMsg(&key_info);
					beep();
				} 
			}
			
			if(sys_status == READ_OK)//ƥ����յ��ļ�ֵ
			{
				if(date <=10) //������10��ֵ
				{
					key_info.key_value = date;
					PrintDebugMsg();
					WriteKeyMsg(&key_info);					
					LED_read = 1;
					LED_ok = 1;//���
					sys_status = SEND_RED;
				} 
			}
		}
	} 
} 

/*******************************************************************************
* �� �� �� ��KeyScan
* �������� ��4*4����ɨ��
* ��    �� ����
* ��    �� ��num ��ȡ�ļ�ֵ����û�м�ֵ�򷵻� 0xff
*******************************************************************************/
unsigned char KeyScan()
{
	unsigned char temp,num;
	num = 0xff;
	KEY_PORT=0xfe;
	temp=KEY_PORT;
	temp=temp&0xf0;
	while(temp!=0xf0)
	{
		Delayms(5); //��ʱ����
		temp=KEY_PORT;
		temp=temp&0xf0;
		while(temp!=0xf0)
		{
			temp=KEY_PORT;
		switch(temp)
			{
				case 0xee:num=1;
					break;
				case 0xde:num=2;
					break;
				case 0xbe:num=3;
					break;
				case 0x7e:num=4;
					break;
			}
		while(temp!=0xf0)
			{
				temp=KEY_PORT;
				temp=temp&0xf0;
			}
		}
		Delayms(5); //��ʱ����
	}

	KEY_PORT=0xfd;
	temp=KEY_PORT;
	temp=temp&0xf0;
	while(temp!=0xf0)
	{
		Delayms(5); //��ʱ����
		temp=KEY_PORT;
		temp=temp&0xf0;
		while(temp!=0xf0)
		{
			temp=KEY_PORT;
		switch(temp)
			{
				case 0xed:num=5;
					break;
				case 0xdd:num=6;
					break;
				case 0xbd:num=7;
					break;
				case 0x7d:num=8;
					break;
			}
		while(temp!=0xf0)
			{
				temp=KEY_PORT;
				temp=temp&0xf0;
			}
		}
		Delayms(5); //��ʱ����
	}

	KEY_PORT=0xfb;
	temp=KEY_PORT;
	temp=temp&0xf0;
	while(temp!=0xf0)
	{
		Delayms(5); //��ʱ����
		temp=KEY_PORT;
		temp=temp&0xf0;
		while(temp!=0xf0)
		{
			temp=KEY_PORT;
		switch(temp)
			{
				case 0xeb:num=9;
					break;
				case 0xdb:num=10;
					break;
				case 0xbb:num=11;
					break;
				case 0x7b:num=12;
					break;
			}
		while(temp!=0xf0)
			{
				temp=KEY_PORT;
				temp=temp&0xf0;
			}
		}
		Delayms(5); //��ʱ����
	}

	KEY_PORT=0xf7;
	temp=KEY_PORT;
	temp=temp&0xf0;
	while(temp!=0xf0)
	{
		Delayms(5); //��ʱ����
		temp=KEY_PORT;
		temp=temp&0xf0;
		while(temp!=0xf0)
		{
			temp=KEY_PORT;
		switch(temp)
			{
				case 0xe7:num=13;
					break;
				case 0xd7:num=14;
					break;
				case 0xb7:num=15;
					break;
				case 0x77:num=16;
					break;
			}
		while(temp!=0xf0)
			{
				temp=KEY_PORT;
				temp=temp&0xf0;
			}
		}
		Delayms(5); //��ʱ����
	}
return num;
}

/*******************************************************************************
* �� �� �� ��T1_ISR
* �������� ����ʱ��1�жϷ����������ڲ���38k��Ƶ
* ��    �� ����
* ��    �� ����
*******************************************************************************/
void T1_ISR(void) interrupt 3 
{ 
	IR_OUT=!IR_OUT; 
}

/*******************************************************************************
* �� �� �� ��Exit0Int
* �������� ���ⲿ�ж�0 ISR
* ��    �� ����
* ��    �� ����
*******************************************************************************/
void Exit0Int() interrupt 0 
{
	EX0 = 0; 
	if(sys_status != READ_RED) return;
	
	TH0 = 0;
	TL0 = 0;
	TR0 = 1;   
	
	while(1)
	{
		if(IRIN == 1)
		{
			while(IRIN)
			{
				if(TH0 > 200)//��ƽ����ʱ�䳬��Լ20ms�����ɷ���
				{
					if(key_info.count_bit >= 16)
					{
						LED_ok = 0;
						sys_status = READ_OK;
						TR0 = 0;
						return;
					}
					else
					{
						memset(&key_info.key_value , 0 , sizeof(struct KEYMSG));//���buf
						printf("key_info.count_bit < 16\r\n");
						TR0 = 0;
						EX0 = 1; 
						return;
					}
				}
			}
		}
		else if(IRIN == 0)		
		{
			while(!IRIN);
		}
		TR0 = 0;
		key_info.time[key_info.count_bit*2] = TH0;
		key_info.time[key_info.count_bit*2+1] = TL0;
		
		TH0 = 0;
		TL0 = 0;
		TR0 = 1;//�ȿ�ʼ����ļ�����ͬʱִ������ļ��㣬����������С
		
		key_info.value_msg[key_info.count_bit/8] = key_info.value_msg[key_info.count_bit/8] | ((unsigned char)(!IRIN) << (7-key_info.count_bit%8));		
		key_info.count_bit++;
		if(key_info.count_bit > 320) 
		{
				memset(&key_info.key_value , 0 , sizeof(struct KEYMSG));//���buf
				TR0 = 0;
			printf("key_info.count_bit > 160\r\n");
			EX0 = 1; 
				return;			
		}
	}
//     EX0 = 1; 
}

/*******************************************************************************
* �� �� �� ��beep
* �������� ����������һ��
* ��    �� ����
* ��    �� ����
*******************************************************************************/
void beep()
{
  unsigned char i;
  for (i=0;i<180;i++)
   {
     Delayms(1);
     BEEP=!BEEP;                 //BEEPȡ��
   } 
  BEEP=1;                      //�رշ�����
}

/*******************************************************************************
* �� �� �� ��ReadKeyMsg
* �������� ����ȡһ����ֵ����Ϣ����ֵ�Ѿ��ŵ�key_msg->key_value
* ��    �� ��struct KEYMSG *key_msg ��Ҫ������Ϣ�Ľṹ��
* ��    �� ����
*******************************************************************************/
void ReadKeyMsg(struct KEYMSG *key_msg)
{
	unsigned int index,length;
	unsigned int start_add = (key_msg->key_value-1)*KEY_BLOCK;
	unsigned char *ptemp = (unsigned char *)key_msg;
	
	for(index = 0;index < 43;index++)
	{
		ptemp[index] = X24c16ReadAdd(start_add+index);
		Delayms(1);
	}
	length = key_msg->count_bit*2;//���ٶ�ȡʱ��
	for(index = 43;index < length + 43;index++)
	{
		ptemp[index] = X24c16ReadAdd(start_add+index);
		Delayms(1);
	}	
}

/*******************************************************************************
* �� �� �� ��WriteKeyMsg
* �������� ��д���ֵ��Ϣ���ⲿ�洢��
* ��    �� ��struct KEYMSG *key_msg �����Ϣ�Ľṹ��
* ��    �� ����
*******************************************************************************/
void WriteKeyMsg(struct KEYMSG *key_msg)
{
	unsigned int index,length;
	unsigned int start_add = (key_msg->key_value-1)*KEY_BLOCK;
	unsigned char *ptemp = (unsigned char *)key_msg;
	
	length = key_msg->count_bit*2 + 43;//����д��ʱ��
	for(index = 0;index < length;index++)
	{
		X24c16WriteAdd(start_add+index,ptemp[index]);
		Delayms(1);
	}
}	

/*******************************************************************************
* �� �� �� ��SendKeyMsg
* �������� �����ͺ����ֵ,ע�ⷢ�ͺͽ����Ƿ���ģ�Ҳ���ǽ���Ϊ1������0
* ��    �� ��key_msg ��¼�����ֵ��Ϣ�Ľṹ��
* ��    �� ����
*******************************************************************************/
void SendKeyMsg(struct KEYMSG *key_msg)
{
	unsigned int count;
	unsigned char flag;
//	unsigned char TH0_temp;
//	unsigned char TL0_temp;
	flag = key_msg->value_msg[0] & (0x01<<7);//�ȼ��㣬�Խ�Լ����ʱ��
	for(count = 0;count < key_msg->count_bit ; count++)
	{
		key_msg->time[count*2] = 256 - key_msg->time[count*2];
		key_msg->time[count*2+1] = 256 - key_msg->time[count*2+1];
	}
	for(count = 0;count < key_msg->count_bit ; count++)
	{
		if(flag == 0)
		{
			TH0 = key_msg->time[count*2];
			TL0 = key_msg->time[count*2+1];
			TR0=1;
			TR1=1; //�򿪶�ʱ��1�����ж��з����ز���������1
			
			flag = key_msg->value_msg[(count+1)/8] & (0x01<<(7-((count+1)%8)));//�ȼ��㣬�Խ�Լ����ʱ��		
			
			while(!TF0); 
			
			TR1=0; 
			TF0=0;
			TR0=0; 	
			IR_OUT=1;			
		}
		else
		{
			TH0 = key_msg->time[count*2];
			TL0 = key_msg->time[count*2+1];
			TR0=1; 
	//		TR1=0; //�رն�ʱ��1������0
					
			flag = key_msg->value_msg[(count+1)/8] & (0x01<<(7-((count+1)%8)));//�ȼ��㣬�Խ�Լ����ʱ��
			
			while(!TF0); 
			
			TR0=0; 
			TF0=0;	
			IR_OUT=1;
		}
	}
}

void PrintDebugMsg()
{
	int i;
	printf("key_value:%d\r\n",(int)key_info.key_value);
	printf("count_bit:%d\r\n",(int)key_info.count_bit);
	printf("value_msg:");
	for(i=0;i<10;i++)
	{
		printf("%d ",(int)key_info.value_msg[i]);
	}
	printf("\r\n");
	
		printf("time:");
	for(i=0;i<160;i++)
	{
		printf("%d ",(int)key_info.time[i]);
	}
	printf("\r\n");
	printf("\r\n");
}
