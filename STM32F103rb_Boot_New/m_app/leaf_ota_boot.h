#ifndef _LEAF_OTA_BOOT_H_
#define _LEAF_OTA_BOOT_H_


/*=====用户配置(根据自己的分区进行配置)=====*/

#define PageSize FLASH_PAGE_SIZE  // 1K

#define BootLoader_addr    0x08000000U  ///< BootLoader的首地址
#define Application_1_Addr 0x08005000U  ///< 应用程序1的首地址
#define Application_2_Addr 0x0800F000U  ///< 应用程序2的首地址

#define BootLoader_Size  0x5000U  ///< BootLoader的大小 20K
#define Application_Size 0xA000U  ///< 应用程序的大小 40K


/* 启动的步骤 */
#define Startup_Normol 0xFFFFFFFF  ///< 正常启动
#define Startup_Update 0xAAAAAAAA  ///< 升级再启动
#define Startup_OtaNow 0x55555555  ///< 现在升级
#define Startup_Reset  0x5555AAAA  ///< ***恢复出厂 目前没使用***

/*==========================================*/


int  Erase_page( unsigned int pageaddr, unsigned int num );
void WriteFlash( unsigned int addr, unsigned int* buff, int word_size );

void Leaf_Uart2_Send( unsigned char* buf, int len );
void Leaf_Deal_Frame( unsigned char* buf, int len );

void Start_BootLoader( void );

#endif
