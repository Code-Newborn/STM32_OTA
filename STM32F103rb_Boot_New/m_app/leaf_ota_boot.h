#ifndef _LEAF_OTA_BOOT_H_
#define _LEAF_OTA_BOOT_H_



/*=====�û�����(�����Լ��ķ�����������)=====*/

#define PageSize		FLASH_PAGE_SIZE			//1K

#define BootLoader_addr			0x08000000U		///< BootLoader���׵�ַ
#define Application_1_Addr		0x08005000U		///< Ӧ�ó���1���׵�ַ
#define Application_2_Addr		0x0800F000U		///< Ӧ�ó���2���׵�ַ

#define BootLoader_Size 		0x5000U			///< BootLoader�Ĵ�С 20K
#define Application_Size		0xA000U			///< Ӧ�ó���Ĵ�С 40K


/* �����Ĳ��� */
#define Startup_Normol 0xFFFFFFFF	///< ��������
#define Startup_Update 0xAAAAAAAA	///< ����������
#define Startup_OtaNow 0x55555555	///< ��������
#define Startup_Reset  0x5555AAAA	///< ***�ָ����� Ŀǰûʹ��***

/*==========================================*/




int  Erase_page(unsigned int pageaddr, unsigned int num);
void WriteFlash(unsigned int addr, unsigned int * buff, int word_size);

void Leaf_Uart2_Send(unsigned char * buf, int len);
void Leaf_Deal_Frame(unsigned char * buf, int len);

void Start_BootLoader(void);

#endif

