#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "../Driver/_command.h"
#include "../Driver/_can_database.h"
#include "../Driver/_can.h"
#include "../Driver/_io_status.h"
#define CAN1_BAUDRATE 1000

//�Ƿ�ʹ����Ӧcan���ߣ�����FIFO
#define USE_CAN1_R0	1
#define USE_CAN1_R1	1

void Can_NVIC_Config(void)
{
	NVIC_InitTypeDef  NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
#if  USE_CAN1_R0
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);
#endif
#if  USE_CAN1_R1
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
#endif
}

void Can_GPIO_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* Configure CAN1 IOs **********************************************/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1,ENABLE);	
	GPIO_PinRemapConfig(GPIO_Remap1_CAN1,ENABLE);
#if USE_CAN1_R0 || USE_CAN1_R1
	/* Configure CAN RX and TX pins */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;	         //PB8:CAN-RX  �ض��幦�� 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;			   //��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;					 //PB9:CAN-TX  �ض��幦�� 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;			 //����ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
#endif
}

void CAN_CAN_Config(void)
{
	CAN_InitTypeDef        CAN_InitStructure;
	
	/* Enable CAN clock */
#if USE_CAN1_R0 || USE_CAN1_R1
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1,ENABLE);
	CAN_DeInit(CAN1);
#endif

	/* CAN cell init */
	CAN_StructInit(&CAN_InitStructure);

	CAN_InitStructure.CAN_TTCM=DISABLE;//ʧ��ʱ�䴥��ͨѶģʽ
	CAN_InitStructure.CAN_ABOM=DISABLE;//ʧ���Զ����߹���
	CAN_InitStructure.CAN_AWUM=DISABLE;//ʧ���Զ�����ģʽ
	CAN_InitStructure.CAN_NART=DISABLE;//ʧ�ܷ��Զ��ش���ģʽ,���Զ��ش�
	CAN_InitStructure.CAN_RFLM=DISABLE;//ʧ�ܽ���FIFO����ģʽ,��FIFO���ʱ�����ݸ���
	CAN_InitStructure.CAN_TXFP=ENABLE;//ʧ�ܷ���FIFO���ȼ�,����˳���ɷ���˳����� DISABLE:ID����
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;//����ģʽ 

	CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;		   //BTR-SJW ����ͬ����Ծ���� 1��ʱ�䵥Ԫ
	CAN_InitStructure.CAN_BS1=CAN_BS1_2tq;		   //BTR-TS1 ʱ���1 ռ����2��ʱ�䵥Ԫ
	CAN_InitStructure.CAN_BS2=CAN_BS2_3tq;		   //BTR-TS1 ʱ���2 ռ����3��ʱ�䵥Ԫ
	
	//���߲�����,�����ʼ�����μ� F1�ֲᡰ22.7.7 Bit timing��
	//�˴���APB1ʱ��Ϊ72M״̬����Ȼ�ֲ���˵��APB1ֻ�����Ϊ36M
#if CAN1_BAUDRATE == 1000 /* 1MBps */
  CAN_InitStructure.CAN_Prescaler =12;			   //BTR-BRP �����ʷ�Ƶ��  ������ʱ�䵥Ԫ��ʱ�䳤�� 72/(1+2+3)/12=1Mbps
#elif CAN1_BAUDRATE == 500 /* 500KBps */
  CAN_InitStructure.CAN_Prescaler =24;
#elif CAN1_BAUDRATE == 250 /* 250KBps */
  CAN_InitStructure.CAN_Prescaler =48;
#elif CAN1_BAUDRATE == 125 /* 125KBps */
  CAN_InitStructure.CAN_Prescaler =96;
#endif 

#if USE_CAN1_R0 || USE_CAN1_R1
	CAN_Init(CAN1, &CAN_InitStructure);	
#endif
}

void Can_FILTER_Config(void)
{
	uint8_t i=0;
	uint8_t can1_filter = 0;
	uint16_t id_temp;
	
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	
	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdList;
	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_16bit;
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
	for(i=0;i<Can_Data_Num;i++)
	{
		if(Can_Database[i].Data_type==READ_ONLY)
		{
			CAN_FilterInitStructure.CAN_FilterNumber = can1_filter;
			can1_filter++;
			id_temp = (uint16_t)(Can_Database[i].Data_ID)<<5;
			CAN_FilterInitStructure.CAN_FilterIdHigh = id_temp;
			CAN_FilterInitStructure.CAN_FilterIdLow = id_temp;
			CAN_FilterInitStructure.CAN_FilterMaskIdHigh = id_temp;
			CAN_FilterInitStructure.CAN_FilterMaskIdLow = id_temp;
			CAN_FilterInitStructure.CAN_FilterFIFOAssignment = Can_Database[i].Fifo_num;
			
			CAN_FilterInit(&CAN_FilterInitStructure);
		}
	}
}

void Can_IT_Config(void)
{
#if  USE_CAN1_R0
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
#endif
#if  USE_CAN1_R1
	CAN_ITConfig(CAN1, CAN_IT_FMP1, ENABLE);
#endif
}

void Can1_Init(void)
{
	Hash_Table_init();
	Can_NVIC_Config();
	Can_GPIO_Config();
	CAN_CAN_Config();
	Can_FILTER_Config();
	Can_IT_Config();
//   printf("can init ok");
}

/*can���߽���ʱ�ж��е�࣬����Ҽ���
����can���߹����豸�϶࣬ÿ����Ӷ��и��Եĳ������а��Ӷ�����
����ĸ��ʽ�С��Can_Database���ݿ�������������ͨ�ŵĹ�����Ϣ����
�������ص����ݿ������ȫͳһ�����򣬾ͻ����ͨ�Ź��ϡ��ڽ���ʱ��
�����жϿ��Կ��ٷ�������֮�����Ϣ��ͳһ���⡣
*/
	
//������GPS�巢�͵�����
void USB_LP_CAN1_RX0_IRQHandler(void)   //�ض����������жϷ�����
{
	uint8_t i;
	CanRxMsg   RxMessage;
// 	printf("\r\ncan ok");
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
	if(HASH_TABLE[RxMessage.StdId] != 255)
	{
		if(Can_Database[HASH_TABLE[RxMessage.StdId]].Data_length == RxMessage.DLC)
			if(Can_Database[HASH_TABLE[RxMessage.StdId]].Data_type == WRITE_ONLY)
				if(Can_Database[HASH_TABLE[RxMessage.StdId]].Channel == 1)
					if(Can_Database[HASH_TABLE[RxMessage.StdId]].Fifo_num == CAN_Filter_FIFO0)
					{
						for(i=0;i<RxMessage.DLC;i++)
							(*(Can_Database[HASH_TABLE[RxMessage.StdId]].Data_ptr+i)) = RxMessage.Data[i];
						Can_Database[HASH_TABLE[RxMessage.StdId]].MenuFunc();
					}
						
	}
}

//������GPS�巢�͵�����
void CAN1_RX1_IRQHandler(void)  
{
	uint8_t i;
	CanRxMsg   RxMessage;
	

	
	CAN_Receive(CAN1, CAN_FIFO1, &RxMessage);
	if(HASH_TABLE[RxMessage.StdId] != 255)
	{
		if(Can_Database[HASH_TABLE[RxMessage.StdId]].Data_length == RxMessage.DLC)
			if(Can_Database[HASH_TABLE[RxMessage.StdId]].Data_type == READ_ONLY)
				if(Can_Database[HASH_TABLE[RxMessage.StdId]].Channel == 1)
					if(Can_Database[HASH_TABLE[RxMessage.StdId]].Fifo_num == CAN_Filter_FIFO1)
					{
						for(i=0;i<RxMessage.DLC;i++)
							(*(Can_Database[HASH_TABLE[RxMessage.StdId]].Data_ptr+i)) = RxMessage.Data[i];
						Can_Database[HASH_TABLE[RxMessage.StdId]].MenuFunc();
					}
	}
}

void Can_SendData(CanTxMsg* TxM)
{
	while(CAN_Transmit(CAN1,TxM)==CAN_NO_MB);
}
/******************* (C) COPYRIGHT 2016 Heils *****END OF FILE****/