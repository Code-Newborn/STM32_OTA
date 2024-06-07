#include "leaf_ota_boot.h"
#include "main.h"
#include "stdio.h"




/*======================================================================*/
/*=========================Flash��������(start)=========================*/
/*======================================================================*/
/**
 * @bieaf ����ҳ
 *
 * @param pageaddr  ��ʼ��ַ	
 * @param num       ������ҳ��
 * @return 1
 */
static int Erase_page(unsigned int pageaddr, unsigned int num)
{
	HAL_FLASH_Unlock();
	
	/* ����FLASH*/
	FLASH_EraseInitTypeDef FlashSet;
	FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashSet.PageAddress = pageaddr;
	FlashSet.NbPages = num;
	
	/*����PageError, ���ò�������*/
	unsigned int PageError = 0;
	HAL_FLASHEx_Erase(&FlashSet, &PageError);
	
	HAL_FLASH_Lock();
	return 1;
}


/**
 * @bieaf д���ɸ�����
 *
 * @param addr       д��ĵ�ַ
 * @param buff       д�����ݵ���ʼ��ַ
 * @param word_size  ����
 */
static void WriteFlash(unsigned int addr, unsigned int * buff, int word_size)
{	
	/* 1/4����FLASH*/
	HAL_FLASH_Unlock();
	
	for(int i = 0; i < word_size; i++)	
	{
		/* 3/4��FLASH��д*/
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + 4 * i, buff[i]);	
	}

	/* 4/4��סFLASH*/
	HAL_FLASH_Lock();
}



/**
 * @bieaf �����ɸ�����
 *
 * @param addr       �����ݵĵ�ַ
 * @param buff       �������ݵ�����ָ��
 * @param word_size  ����
 */
static void ReadFlash(unsigned int addr, unsigned int * buff, uint16_t word_size)
{
	for(int i =0; i < word_size; i++)
	{
		buff[i] = *(__IO unsigned int*)(addr + 4 * i);
	}
	return;
}
/*====================================================================*/
/*=========================Flash��������(end)=========================*/
/*====================================================================*/

/**
 * @bieaf ���г���ĸ���
 *
 * @param  ���˵�Դ��ַ
 * @param  ���˵�Ŀ�ĵ�ַ
 * @return ���˵ĳ����С
 */
void MoveCode(unsigned int src_addr, unsigned int des_addr, unsigned int byte_size)
{
	/*1.����Ŀ�ĵ�ַ*/
	printf("> Start erase des flash......\r\n");
	Erase_page(des_addr, (byte_size/PageSize));
	printf("> Erase des flash down......\r\n");
	
	/*2.��ʼ����*/	
	unsigned int temp[256];
	
	printf("> Start copy......\r\n");
	for(int i = 0; i < byte_size/1024; i++)
	{
		ReadFlash((src_addr + i*1024), temp, 256);
		WriteFlash((des_addr + i*1024), temp, 256);
	}
	printf("> Copy down......\r\n");
	
	/*3.����Դ��ַ*/
	printf("> Start erase src flash......\r\n");
	Erase_page(src_addr, (byte_size/PageSize));
	printf("> Erase src flash down......\r\n");
}



/* �޸�����ģʽ */
void Set_Start_Mode(unsigned int Mode)
{ 
	//����һҳ - д����
	Erase_page((Application_1_Addr - PageSize), 1);
	WriteFlash((Application_1_Addr - 4), &Mode, 1);
}



/* ��ȡ����ģʽ */
unsigned int Read_Start_Mode(void)
{
	unsigned int mode = 0;
	ReadFlash((Application_1_Addr - 4), &mode, 1);
	return mode;
}



static unsigned int file_size = 0;		//�ļ���С
static unsigned int addr_now = 0;		//bin�ļ�λ��

static int Read_Flag = 0 ;				//����׼��������־λ
static unsigned int Waite_time = 0 ;	//�Լ�ʱ��



/* ����2�������� */
void Leaf_Uart2_Send(unsigned char * buf, int len)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 0xFFFF);
	HAL_Delay(5);
}



/**
 * @bieaf �����ļ�����
 *
 * @param ����ĵ�ַ
 * @param ����Ĵ�С
 */
void Get_File(unsigned int addr, int size)
{
	static unsigned char tmp_t[10];
	tmp_t[0] = 'D';
	tmp_t[1] = 'A';
	tmp_t[2] = 'T';
	tmp_t[3] = 'A';
	tmp_t[4] = (addr>>24)&0xFF;
	tmp_t[5] = (addr>>16)&0xFF;
	tmp_t[6] = (addr>>8)&0xFF;
	tmp_t[7] = (addr)&0xFF;
	tmp_t[8] = size;
	
	/* ����У��� */	
	tmp_t[9] = tmp_t[0] + tmp_t[1] + tmp_t[2] + tmp_t[3] + tmp_t[4] + tmp_t[5] + tmp_t[6] + tmp_t[7] + tmp_t[8];
	Leaf_Uart2_Send(tmp_t, 10);
}



/**
 * @bieaf �ظ����سɹ�
 *
 */
void Get_File_Ok()
{
	static unsigned char tmp_t[3];
	tmp_t[0] = 'O';
	tmp_t[1] = 'K';
	
	/* ����У��� */
	tmp_t[2] = tmp_t[0] + tmp_t[1];
	Leaf_Uart2_Send(tmp_t, 3);
}



/* ���ݴ����� */
void Leaf_Deal_Frame(unsigned char * buf, int len)
{
	/* У�� */
	unsigned char sum = 0;
	for(int i = 0;i<(len-1);i++)
	{
		sum += buf[i];
	}
	if(sum != buf[len - 1]) return;
	
	
	
	
	if(len == 10)//��ʼ ("START")+(4�ֽڵ�ַ)+(1�ֽ�У��) = 10
	{
		if(buf[0]!='S') return;
		if(buf[1]!='T') return;
		if(buf[2]!='A') return;
		if(buf[3]!='R') return;
		if(buf[4]!='T') return;
		
		// �ļ���С
		file_size = (buf[5]<<24) + (buf[6]<<16) + (buf[7]<<8) + (buf[8]);
		if(file_size>Application_Size) return;

		// ����
		Erase_page(Application_1_Addr, Application_Size/PageSize);
		Read_Flag = 1;
		printf("\n> Status:%d\r\n> Ready to receive file\r\n",file_size);
	}	
	else if(len == 4)//���� ("END")+(1�ֽ�У��) = 4
	{
		if(buf[0]!='E') return;
		if(buf[1]!='N') return;
		if(buf[2]!='D') return;
		
		
		// ����ʧ�� - ��λ
		printf("> File fail!, restart...\r\n");
		HAL_NVIC_SystemReset();
	}
	else if(len==(138))//�����ļ� ("DATA")+(4�ֽڵ�ַ)+(1�ֽڴ�С)+(128�ֽ�����)+(1�ֽ�У��) = 137
	{
		if(buf[0]!='D') return;
		if(buf[1]!='A') return;
		if(buf[2]!='T') return;
		if(buf[3]!='A') return;
		
		unsigned int addr = (buf[4]<<24) + (buf[5]<<16) + (buf[6]<<8) + (buf[7]); 
		if(addr_now != addr) return;
		
		if(buf[8]!=128) return;
		
		/* ������ȷ:��¼ */
		WriteFlash((Application_1_Addr + addr_now), (unsigned int *)(&buf[9]), 32);		
		printf("> File data:%d/%d\r\n",addr_now,file_size);
		
		// �޸���¼λ��
		addr_now += 128;
		
		if(addr_now >= file_size)//��¼���
		{
			//�ɹ�
			Get_File_Ok();
			Set_Start_Mode(Startup_Normol);
			printf("> File ok!, restart...\r\n");
			HAL_NVIC_SystemReset();
		}
	}
}



/**
 * @bieaf  ��ȡ�����ļ��ĺ���
 * @detail ���Լ�ʱ�����յ���������ͳ�������
 *
 * @param  int time_ms,�Լ�ʱ��
 */
void Get_OTA_File(unsigned int time_ms)
{
	Waite_time = time_ms;
	
	printf("> Wait %dms...\r\n",time_ms);
	
	while(1)
	{
		if(Rx_Flag == 1)//�յ�һ֡����
		{
			Rx_Flag = 0;			
			Leaf_Deal_Frame(Rx_Buf, Rx_Len);
		}
		
		/* ��������� */
		if(Read_Flag == 1)
		{
			Get_File(addr_now,128);
			HAL_Delay(10);
		}
		else if(Waite_time > 0)
		{
			Waite_time = Waite_time - 100;
			HAL_Delay(100);
		}
		else
		{
			return;
		}
	}
}



/* ���û������ջ��ֵ */
__asm void MSR_MSP (unsigned int ulAddr) 
{
    MSR MSP, r0 			                   //set Main Stack value
    BX r14
}



/* ������ת���� */
typedef void (*Jump_Fun)(void);
void IAP_ExecuteApp (unsigned int App_Addr)
{
	Jump_Fun JumpToApp; 
    
	if ( ( ( * ( __IO unsigned int * ) App_Addr ) & 0x2FFE0000 ) == 0x20000000 )	//���ջ����ַ�Ƿ�Ϸ�.
	{ 
		JumpToApp = (Jump_Fun) * ( __IO unsigned int *)(App_Addr + 4);				//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
		MSR_MSP( * ( __IO unsigned int * ) App_Addr );								//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
		JumpToApp();															//��ת��APP.
	}
}




/* ����BootLoader������ */
void Start_BootLoader(void)
{
	/*==========��ӡ��Ϣ==========*/  
//	 printf("\r\n");
//	 printf("******************************************\r\n");
//	 printf("*                                        *\r\n");
//	 printf("*    *****     ****     ****   *******   *\r\n");
//	 printf("*    *   **   *    *   *    *     *      *\r\n");
//	 printf("*    *    *   *    *   *    *     *      *\r\n");
//	 printf("*    *   **   *    *   *    *     *      *\r\n");
//	 printf("*    *****    *    *   *    *     *      *\r\n");
//	 printf("*    *   **   *    *   *    *     *      *\r\n");
//	 printf("*    *    *   *    *   *    *     *      *\r\n");
//	 printf("*    *   **   *    *   *    *     *      *\r\n");
//	 printf("*    *****     ****    ****       *      *\r\n");
//	 printf("*                                        *\r\n");
//	 printf("************************ by Leaf_Fruit ***\r\n");
	
	 printf("\r\n");
	 printf("***********************************\r\n");
	 printf("*                                 *\r\n");
	 printf("*           BootLoader            *\r\n");
	 printf("*                                 *\r\n");
	 printf("***************** by Leaf_Fruit ***\r\n");
	
	
	
	Get_OTA_File(1000);	/* �Լ�2�� */
	
	printf("> Choose a startup method......\r\n");	
	switch(Read_Start_Mode())										///< ��ȡ�Ƿ�����Ӧ�ó��� */
	{
		case Startup_Normol:										///< �������� */
		{
			printf("> Normal start......\r\n");
			break;
		}
		case Startup_Update:										///< ���������� */
		{
			printf("> Start update......\r\n");		
			MoveCode(Application_2_Addr, Application_1_Addr, Application_Size);
			printf("> Update down......\r\n");
			break;
		}
		case Startup_OtaNow:										///< ���ھ�����
		{
			printf("> OTA now......\r\n");
			Get_OTA_File(20000);
			break;			
		}
		case Startup_Reset:											///< �ָ��������� Ŀǰûʹ�� */
		{
			printf("> Restore to factory program......\r\n");
			break;			
		}
		default:													///< ����ʧ��
		{
			printf("> Error:%X!!!......\r\n", Read_Start_Mode());
			return;			
		}
	}
	
	/* ��ת��Ӧ�ó��� */
	printf("> Start up......\r\n\r\n");	
	IAP_ExecuteApp(Application_1_Addr);
}




