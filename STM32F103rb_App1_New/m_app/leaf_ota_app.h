#ifndef _LEAF_OTA_APP_H
#define _LEAF_OTA_APP_H
#ifdef __cplusplus
extern "C" {
#endif



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

void Leaf_Uart2_Send(unsigned char * buf, int len);
void Leaf_Deal_Frame(unsigned char * buf, int len);
void leaf_ota_app(void);



#ifdef __cplusplus
}
#endif

#endif /* __YMODEM_H */
