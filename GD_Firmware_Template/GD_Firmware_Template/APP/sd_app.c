#include "mcu_cmic_gd32f470vet6.h"

FATFS fs;
FIL fdst;
uint16_t i = 0, count, result = 0;
UINT br, bw;

sd_card_info_struct sd_cardinfo;

BYTE buffer[128];
BYTE filebuffer[128];

ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length --){
        if(*src++ != *dst++)
            return ERROR;
    }
    return SUCCESS;
}

void sd_fatfs_init(void)
{
    nvic_irq_enable(SDIO_IRQn, 0, 0);					// ʹ��SDIO�жϣ����ȼ�Ϊ0
}

/**
 * @brief       ͨ�����ڴ�ӡSD�������Ϣ
 * @param       ��
 * @retval      ��
 */
void card_info_get(void)
{
    sd_card_info_struct sd_cardinfo;      // SD����Ϣ�ṹ��
    sd_error_enum status;                 // SD������״̬
    uint32_t block_count, block_size;
    
    // ��ȡSD����Ϣ
    status = sd_card_information_get(&sd_cardinfo);
    
    if(SD_OK == status)
    {
        my_printf(DEBUG_USART, "\r\n*** SD Card Info ***\r\n");
        
        // ��ӡ������
        switch(sd_cardinfo.card_type)
        {
            case SDIO_STD_CAPACITY_SD_CARD_V1_1:
                my_printf(DEBUG_USART, "Card Type: Standard Capacity SD Card V1.1\r\n");
                break;
            case SDIO_STD_CAPACITY_SD_CARD_V2_0:
                my_printf(DEBUG_USART, "Card Type: Standard Capacity SD Card V2.0\r\n");
                break;
            case SDIO_HIGH_CAPACITY_SD_CARD:
                my_printf(DEBUG_USART, "Card Type: High Capacity SD Card\r\n");
                break;
            case SDIO_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: Multimedia Card\r\n");
                break;
            case SDIO_HIGH_CAPACITY_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: High Capacity Multimedia Card\r\n");
                break;
            case SDIO_HIGH_SPEED_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: High Speed Multimedia Card\r\n");
                break;
            default:
                my_printf(DEBUG_USART, "Card Type: Unknown\r\n");
                break;
        }
        
        // ��ӡ�������Ϳ��С
        block_count = (sd_cardinfo.card_csd.c_size + 1) * 1024;
        block_size = 512;
        my_printf(DEBUG_USART,"\r\n## Device size is %dKB (%.2fGB)##", sd_card_capacity_get(), sd_card_capacity_get() / 1024.0f / 1024.0f);
        my_printf(DEBUG_USART,"\r\n## Block size is %dB ##", block_size);
        my_printf(DEBUG_USART,"\r\n## Block count is %d ##", block_count);
        
        // ��ӡ������ID�Ͳ�Ʒ����
        my_printf(DEBUG_USART, "Manufacturer ID: 0x%X\r\n", sd_cardinfo.card_cid.mid);
        my_printf(DEBUG_USART, "OEM/Application ID: 0x%X\r\n", sd_cardinfo.card_cid.oid);
        
        // ��ӡ��Ʒ���� (PNM)
        uint8_t pnm[6];
        pnm[0] = (sd_cardinfo.card_cid.pnm0 >> 24) & 0xFF;
        pnm[1] = (sd_cardinfo.card_cid.pnm0 >> 16) & 0xFF;
        pnm[2] = (sd_cardinfo.card_cid.pnm0 >> 8) & 0xFF;
        pnm[3] = sd_cardinfo.card_cid.pnm0 & 0xFF;
        pnm[4] = sd_cardinfo.card_cid.pnm1 & 0xFF;
        pnm[5] = '\0';
        my_printf(DEBUG_USART, "Product Name: %s\r\n", pnm);
        
        // ��ӡ��Ʒ�汾�����к�
        my_printf(DEBUG_USART, "Product Revision: %d.%d\r\n", (sd_cardinfo.card_cid.prv >> 4) & 0x0F, sd_cardinfo.card_cid.prv & 0x0F);
        // ���к����޷��ŷ�ʽ��ʾ�����⸺��
        my_printf(DEBUG_USART, "Product Serial Number: 0x%08X\r\n", sd_cardinfo.card_cid.psn);
        
        // ��ӡCSD�汾������CSD��Ϣ
        my_printf(DEBUG_USART, "CSD Version: %d.0\r\n", sd_cardinfo.card_csd.csd_struct + 1);
        
    }
    else
    {
        my_printf(DEBUG_USART, "\r\nFailed to get SD card information, error code: %d\r\n", status);
    }
}

void sd_fatfs_test(void)
{
    uint16_t k = 5;
    DSTATUS stat = 0;
    do
    {
        stat = disk_initialize(0); 			//��ʼ��SD�����豸��0��,�������������,ÿ����������������Ӳ�̡�U �̵ȣ�ͨ����������һ��Ψһ�ı�š�
    }while((stat != 0) && (--k));			//�����ʼ��ʧ�ܣ��������k�Ρ�
    
    card_info_get();
    
    my_printf(DEBUG_USART, "SD Card disk_initialize:%d\r\n",stat);
    f_mount(0, &fs);						 //����SD�����ļ�ϵͳ���豸��0����
    my_printf(DEBUG_USART, "SD Card f_mount:%d\r\n",stat);
    
    if(RES_OK == stat)						 //���ع��ؽ����FR_OK ��ʾ�ɹ�����
    {        
        my_printf(DEBUG_USART, "\r\nSD Card Initialize Success!\r\n");
     
        result = f_open(&fdst, "0:/FATFS.TXT", FA_CREATE_ALWAYS | FA_WRITE);		//��SD���ϴ����ļ�FATFS.TXT��
     
        sprintf((char *)filebuffer, "HELLO MCUSTUDIO");

        //result = f_write(&fdst, textfilebuffer, sizeof(textfilebuffer), &bw); 	//��textfilebuffer�е�����д���ļ���
        result = f_write(&fdst, filebuffer, sizeof(filebuffer), &bw);				//��filebuffer�е�����д���ļ���
        
        /**********���д���� begin****************/
        if(FR_OK == result)		
            my_printf(DEBUG_USART, "FATFS FILE write Success!\r\n");
        else
        {
            my_printf(DEBUG_USART, "FATFS FILE write failed!\r\n");
        }
        /**********���д���� end****************/
        
        f_close(&fdst);//�ر��ļ�
        
        
        f_open(&fdst, "0:/FATFS.TXT", FA_OPEN_EXISTING | FA_READ);	//��ֻ����ʽ���´��ļ�
        br = 1;
        
        /**********ѭ����ȡ�ļ����� begin****************/
        for(;;)
        {
            // ��ջ�����
            for (count=0; count<128; count++)
            {
                buffer[count]=0;
            }
            // ��ȡ�ļ����ݵ�buffer
            result = f_read(&fdst, buffer, sizeof(buffer), &br);
            if ((0 == result)|| (0 == br))
            {
                break;
            }
        }
        /**********ѭ����ȡ�ļ����� end****************/
        
        // �Ƚ϶�ȡ��������д��������Ƿ�һ��
        if(SUCCESS == memory_compare(buffer, filebuffer, 128))
        {
            my_printf(DEBUG_USART, "FATFS Read File Success!\r\nThe content is:%s\r\n",buffer);
        }
        else
        {
            my_printf(DEBUG_USART, "FATFS FILE read failed!\n");            
        }
         f_close(&fdst);//�ر��ļ�
    }
}
