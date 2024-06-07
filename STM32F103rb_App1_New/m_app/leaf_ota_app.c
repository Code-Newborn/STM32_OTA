#include "leaf_ota_app.h"
#include "main.h"
#include "stdio.h"



/*======================================================================*/
/*=========================Flash基本函数(start)=========================*/
/*======================================================================*/
/**
 * @bieaf 擦除页
 *
 * @param pageaddr  页起始地址	
 * @param num       擦除的页数
 * @return 1
 */
static int Erase_page(uint32_t pageaddr, uint32_t num)
{
	HAL_FLASH_Unlock();
	
	/* 擦除FLASH*/
	FLASH_EraseInitTypeDef FlashSet;
	FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashSet.PageAddress = pageaddr;
	FlashSet.NbPages = num;
	
	/*设置PageError，调用擦除函数*/
	uint32_t PageError = 0;
	HAL_FLASHEx_Erase(&FlashSet, &PageError);
	
	HAL_FLASH_Lock();
	return 1;
}



/**
 * @bieaf 写若干个数据
 *
 * @param addr       写入的地址
 * @param buff       写入数据的数组指针
 * @param word_size  长度
 */
static void WriteFlash(uint32_t addr, uint32_t * buff, int word_size)
{	
	/* 1/4解锁FLASH*/
	HAL_FLASH_Unlock();
	
	for(int i = 0; i < word_size; i++)	
	{
		/* 3/4对FLASH烧写*/
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + 4 * i, buff[i]);	
	}

	/* 4/4锁住FLASH*/
	HAL_FLASH_Lock();
}



/**
 * @bieaf 读若干个数据
 *
 * @param addr       读数据的地址
 * @param buff       读出数据的数组指针
 * @param word_size  长度
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
/*=========================Flash基本函数(end)=========================*/
/*====================================================================*/


/* 修改启动模式 */
void Set_Start_Mode(unsigned int Mode)
{
	// 擦除1页->写数据
	Erase_page((Application_1_Addr - PageSize), 1);
	WriteFlash((Application_1_Addr - 4), &Mode, 1);
}


static unsigned int file_size = 0;		//文件大小



/* 串口2发送数据 */
void Leaf_Uart2_Send(unsigned char * buf, int len)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 0xFFFF);
	HAL_Delay(5);
}



/*数据处理函数 */
void Leaf_Deal_Frame(unsigned char * buf, int len)
{
	/* 校验 */
	unsigned char sum = 0;
	for(int i = 0;i<(len-1);i++)
	{
		sum += buf[i];
	}
	if(sum != buf[len-1]) return;
	
	
	if(len == 10)//开始 ("START")+(4字节地址)+(1字节校验) = 10
	{
		if(buf[0]!='S') return;
		if(buf[1]!='T') return;
		if(buf[2]!='A') return;
		if(buf[3]!='R') return;
		if(buf[4]!='T') return;
		
		//文件大小
		file_size = (buf[5]<<24) + (buf[6]<<16) + (buf[7]<<8) + (buf[8]);
		if(file_size>Application_Size) return;
		
		//修改标志位
		Set_Start_Mode(Startup_Normol);
		printf("> File ok!, restart...\r\n");
		HAL_NVIC_SystemReset();
	}
}



/* app升级函数 */
void leaf_ota_app(void)
{
	if(Rx_Flag == 1)    	// 收到一帧数据
	{
		Rx_Flag = 0;
		Leaf_Deal_Frame(Rx_Buf, Rx_Len);
	}
}



