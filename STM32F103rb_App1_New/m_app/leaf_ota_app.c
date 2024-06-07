#include "leaf_ota_app.h"
#include "main.h"
#include "stdio.h"



/*======================================================================*/
/*=========================Flash��������(start)=========================*/
/*======================================================================*/
/**
 * @bieaf ����ҳ
 *
 * @param pageaddr  ҳ��ʼ��ַ	
 * @param num       ������ҳ��
 * @return 1
 */
static int Erase_page(uint32_t pageaddr, uint32_t num)
{
	HAL_FLASH_Unlock();
	
	/* ����FLASH*/
	FLASH_EraseInitTypeDef FlashSet;
	FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashSet.PageAddress = pageaddr;
	FlashSet.NbPages = num;
	
	/*����PageError�����ò�������*/
	uint32_t PageError = 0;
	HAL_FLASHEx_Erase(&FlashSet, &PageError);
	
	HAL_FLASH_Lock();
	return 1;
}



/**
 * @bieaf д���ɸ�����
 *
 * @param addr       д��ĵ�ַ
 * @param buff       д�����ݵ�����ָ��
 * @param word_size  ����
 */
static void WriteFlash(uint32_t addr, uint32_t * buff, int word_size)
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


/* �޸�����ģʽ */
void Set_Start_Mode(unsigned int Mode)
{
	// ����1ҳ->д����
	Erase_page((Application_1_Addr - PageSize), 1);
	WriteFlash((Application_1_Addr - 4), &Mode, 1);
}


static unsigned int file_size = 0;		//�ļ���С



/* ����2�������� */
void Leaf_Uart2_Send(unsigned char * buf, int len)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 0xFFFF);
	HAL_Delay(5);
}



/*���ݴ����� */
void Leaf_Deal_Frame(unsigned char * buf, int len)
{
	/* У�� */
	unsigned char sum = 0;
	for(int i = 0;i<(len-1);i++)
	{
		sum += buf[i];
	}
	if(sum != buf[len-1]) return;
	
	
	if(len == 10)//��ʼ ("START")+(4�ֽڵ�ַ)+(1�ֽ�У��) = 10
	{
		if(buf[0]!='S') return;
		if(buf[1]!='T') return;
		if(buf[2]!='A') return;
		if(buf[3]!='R') return;
		if(buf[4]!='T') return;
		
		//�ļ���С
		file_size = (buf[5]<<24) + (buf[6]<<16) + (buf[7]<<8) + (buf[8]);
		if(file_size>Application_Size) return;
		
		//�޸ı�־λ
		Set_Start_Mode(Startup_Normol);
		printf("> File ok!, restart...\r\n");
		HAL_NVIC_SystemReset();
	}
}



/* app�������� */
void leaf_ota_app(void)
{
	if(Rx_Flag == 1)    	// �յ�һ֡����
	{
		Rx_Flag = 0;
		Leaf_Deal_Frame(Rx_Buf, Rx_Len);
	}
}



