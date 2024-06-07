#include "leaf_ota_boot.h"
#include "main.h"
#include "stdio.h"




/*======================================================================*/
/*=========================Flash基本函数(start)=========================*/
/*======================================================================*/
/**
 * @bieaf 擦除页
 *
 * @param pageaddr  起始地址	
 * @param num       擦除的页数
 * @return 1
 */
static int Erase_page(unsigned int pageaddr, unsigned int num)
{
	HAL_FLASH_Unlock();
	
	/* 擦除FLASH*/
	FLASH_EraseInitTypeDef FlashSet;
	FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashSet.PageAddress = pageaddr;
	FlashSet.NbPages = num;
	
	/*设置PageError, 调用擦除函数*/
	unsigned int PageError = 0;
	HAL_FLASHEx_Erase(&FlashSet, &PageError);
	
	HAL_FLASH_Lock();
	return 1;
}


/**
 * @bieaf 写若干个数据
 *
 * @param addr       写入的地址
 * @param buff       写入数据的起始地址
 * @param word_size  长度
 */
static void WriteFlash(unsigned int addr, unsigned int * buff, int word_size)
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

/**
 * @bieaf 进行程序的覆盖
 *
 * @param  搬运的源地址
 * @param  搬运的目的地址
 * @return 搬运的程序大小
 */
void MoveCode(unsigned int src_addr, unsigned int des_addr, unsigned int byte_size)
{
	/*1.擦除目的地址*/
	printf("> Start erase des flash......\r\n");
	Erase_page(des_addr, (byte_size/PageSize));
	printf("> Erase des flash down......\r\n");
	
	/*2.开始拷贝*/	
	unsigned int temp[256];
	
	printf("> Start copy......\r\n");
	for(int i = 0; i < byte_size/1024; i++)
	{
		ReadFlash((src_addr + i*1024), temp, 256);
		WriteFlash((des_addr + i*1024), temp, 256);
	}
	printf("> Copy down......\r\n");
	
	/*3.擦除源地址*/
	printf("> Start erase src flash......\r\n");
	Erase_page(src_addr, (byte_size/PageSize));
	printf("> Erase src flash down......\r\n");
}



/* 修改启动模式 */
void Set_Start_Mode(unsigned int Mode)
{ 
	//擦除一页 - 写数据
	Erase_page((Application_1_Addr - PageSize), 1);
	WriteFlash((Application_1_Addr - 4), &Mode, 1);
}



/* 读取启动模式 */
unsigned int Read_Start_Mode(void)
{
	unsigned int mode = 0;
	ReadFlash((Application_1_Addr - 4), &mode, 1);
	return mode;
}



static unsigned int file_size = 0;		//文件大小
static unsigned int addr_now = 0;		//bin文件位置

static int Read_Flag = 0 ;				//升级准备就绪标志位
static unsigned int Waite_time = 0 ;	//自检时间



/* 串口2发送数据 */
void Leaf_Uart2_Send(unsigned char * buf, int len)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 0xFFFF);
	HAL_Delay(5);
}



/**
 * @bieaf 请求文件数据
 *
 * @param 请求的地址
 * @param 请求的大小
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
	
	/* 计算校验和 */	
	tmp_t[9] = tmp_t[0] + tmp_t[1] + tmp_t[2] + tmp_t[3] + tmp_t[4] + tmp_t[5] + tmp_t[6] + tmp_t[7] + tmp_t[8];
	Leaf_Uart2_Send(tmp_t, 10);
}



/**
 * @bieaf 回复下载成功
 *
 */
void Get_File_Ok()
{
	static unsigned char tmp_t[3];
	tmp_t[0] = 'O';
	tmp_t[1] = 'K';
	
	/* 计算校验和 */
	tmp_t[2] = tmp_t[0] + tmp_t[1];
	Leaf_Uart2_Send(tmp_t, 3);
}



/* 数据处理函数 */
void Leaf_Deal_Frame(unsigned char * buf, int len)
{
	/* 校验 */
	unsigned char sum = 0;
	for(int i = 0;i<(len-1);i++)
	{
		sum += buf[i];
	}
	if(sum != buf[len - 1]) return;
	
	
	
	
	if(len == 10)//开始 ("START")+(4字节地址)+(1字节校验) = 10
	{
		if(buf[0]!='S') return;
		if(buf[1]!='T') return;
		if(buf[2]!='A') return;
		if(buf[3]!='R') return;
		if(buf[4]!='T') return;
		
		// 文件大小
		file_size = (buf[5]<<24) + (buf[6]<<16) + (buf[7]<<8) + (buf[8]);
		if(file_size>Application_Size) return;

		// 擦除
		Erase_page(Application_1_Addr, Application_Size/PageSize);
		Read_Flag = 1;
		printf("\n> Status:%d\r\n> Ready to receive file\r\n",file_size);
	}	
	else if(len == 4)//结束 ("END")+(1字节校验) = 4
	{
		if(buf[0]!='E') return;
		if(buf[1]!='N') return;
		if(buf[2]!='D') return;
		
		
		// 升级失败 - 复位
		printf("> File fail!, restart...\r\n");
		HAL_NVIC_SystemReset();
	}
	else if(len==(138))//数据文件 ("DATA")+(4字节地址)+(1字节大小)+(128字节数据)+(1字节校验) = 137
	{
		if(buf[0]!='D') return;
		if(buf[1]!='A') return;
		if(buf[2]!='T') return;
		if(buf[3]!='A') return;
		
		unsigned int addr = (buf[4]<<24) + (buf[5]<<16) + (buf[6]<<8) + (buf[7]); 
		if(addr_now != addr) return;
		
		if(buf[8]!=128) return;
		
		/* 数据正确:烧录 */
		WriteFlash((Application_1_Addr + addr_now), (unsigned int *)(&buf[9]), 32);		
		printf("> File data:%d/%d\r\n",addr_now,file_size);
		
		// 修改烧录位置
		addr_now += 128;
		
		if(addr_now >= file_size)//烧录完成
		{
			//成功
			Get_File_Ok();
			Set_Start_Mode(Startup_Normol);
			printf("> File ok!, restart...\r\n");
			HAL_NVIC_SystemReset();
		}
	}
}



/**
 * @bieaf  获取升级文件的函数
 * @detail 在自检时间内收到升级命令就程序升级
 *
 * @param  int time_ms,自检时间
 */
void Get_OTA_File(unsigned int time_ms)
{
	Waite_time = time_ms;
	
	printf("> Wait %dms...\r\n",time_ms);
	
	while(1)
	{
		if(Rx_Flag == 1)//收到一帧数据
		{
			Rx_Flag = 0;			
			Leaf_Deal_Frame(Rx_Buf, Rx_Len);
		}
		
		/* 发送命令包 */
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



/* 采用汇编设置栈的值 */
__asm void MSR_MSP (unsigned int ulAddr) 
{
    MSR MSP, r0 			                   //set Main Stack value
    BX r14
}



/* 程序跳转函数 */
typedef void (*Jump_Fun)(void);
void IAP_ExecuteApp (unsigned int App_Addr)
{
	Jump_Fun JumpToApp; 
    
	if ( ( ( * ( __IO unsigned int * ) App_Addr ) & 0x2FFE0000 ) == 0x20000000 )	//检查栈顶地址是否合法.
	{ 
		JumpToApp = (Jump_Fun) * ( __IO unsigned int *)(App_Addr + 4);				//用户代码区第二个字为程序开始地址(复位地址)		
		MSR_MSP( * ( __IO unsigned int * ) App_Addr );								//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
		JumpToApp();															//跳转到APP.
	}
}




/* 进行BootLoader的启动 */
void Start_BootLoader(void)
{
	/*==========打印消息==========*/  
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
	
	
	
	Get_OTA_File(1000);	/* 自检2秒 */
	
	printf("> Choose a startup method......\r\n");	
	switch(Read_Start_Mode())										///< 读取是否启动应用程序 */
	{
		case Startup_Normol:										///< 正常启动 */
		{
			printf("> Normal start......\r\n");
			break;
		}
		case Startup_Update:										///< 升级再启动 */
		{
			printf("> Start update......\r\n");		
			MoveCode(Application_2_Addr, Application_1_Addr, Application_Size);
			printf("> Update down......\r\n");
			break;
		}
		case Startup_OtaNow:										///< 现在就升级
		{
			printf("> OTA now......\r\n");
			Get_OTA_File(20000);
			break;			
		}
		case Startup_Reset:											///< 恢复出厂设置 目前没使用 */
		{
			printf("> Restore to factory program......\r\n");
			break;			
		}
		default:													///< 启动失败
		{
			printf("> Error:%X!!!......\r\n", Read_Start_Mode());
			return;			
		}
	}
	
	/* 跳转到应用程序 */
	printf("> Start up......\r\n\r\n");	
	IAP_ExecuteApp(Application_1_Addr);
}




