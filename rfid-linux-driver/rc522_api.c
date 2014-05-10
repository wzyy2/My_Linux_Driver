#include "rc522_api.h"

#include <linux/delay.h>
#include <linux/spi/spi.h>

void ClearBitMask(unsigned char reg,unsigned char mask);
void WriteRawRC(unsigned char Address, unsigned char value);
void SetBitMask(unsigned char reg,unsigned char mask);
char PcdComMF522(unsigned char Command, 
		unsigned char *pInData, 
		unsigned char InLenByte,
		unsigned char *pOutData, 
		unsigned int  *pOutLenBit);
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData);
unsigned char ReadRawRC(unsigned char Address);
void PcdAntennaOn(void);


void delay_ns(unsigned int ns)
{
	ndelay(ns);
}


char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
	char status;  
	unsigned int unLen;
	unsigned char ucComMF522Buf[MAXRLEN]; 

	ClearBitMask(Status2Reg,0x08);
	WriteRawRC(BitFramingReg,0x07);
	SetBitMask(TxControlReg,0x03);

	ucComMF522Buf[0] = req_code;

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

	if ((status == MI_OK) && (unLen == 0x10))
	{    
		*pTagType     = ucComMF522Buf[0];
		*(pTagType+1) = ucComMF522Buf[1];
	}
	else
	{   status = MI_ERR;   }

	return status;
}


char PcdAnticoll(unsigned char *pSnr)
{
	char status;
	unsigned char i,snr_check=0;
	unsigned int unLen;
	unsigned char ucComMF522Buf[MAXRLEN]; 


	ClearBitMask(Status2Reg,0x08);
	WriteRawRC(BitFramingReg,0x00);
	ClearBitMask(CollReg,0x80);

	ucComMF522Buf[0] = PICC_ANTICOLL1;
	ucComMF522Buf[1] = 0x20;

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

	if (status == MI_OK)
	{
		for (i=0; i<4; i++)
		{   
			*(pSnr+i)  = ucComMF522Buf[i];
			snr_check ^= ucComMF522Buf[i];
		}
		if (snr_check != ucComMF522Buf[i])
		{   status = MI_ERR;    }
	}

	SetBitMask(CollReg,0x80);
	return status;
}


char PcdSelect(unsigned char *pSnr)
{
	char status;
	unsigned char i;
	unsigned int unLen;
	unsigned char ucComMF522Buf[MAXRLEN]; 

	ucComMF522Buf[0] = PICC_ANTICOLL1;
	ucComMF522Buf[1] = 0x70;
	ucComMF522Buf[6] = 0;
	for (i=0; i<4; i++)
	{
		ucComMF522Buf[i+2] = *(pSnr+i);
		ucComMF522Buf[6]  ^= *(pSnr+i);
	}
	CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);

	ClearBitMask(Status2Reg,0x08);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);

	if ((status == MI_OK) && (unLen == 0x18))
	{   status = MI_OK;  }
	else
	{   status = MI_ERR;    }

	return status;
}


char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr)
{
	char status;
	unsigned int unLen;
	unsigned char i,ucComMF522Buf[MAXRLEN]; 

	ucComMF522Buf[0] = auth_mode;
	ucComMF522Buf[1] = addr;
	for (i=0; i<6; i++)
	{    ucComMF522Buf[i+2] = *(pKey+i);   }
	for (i=0; i<6; i++)
	{    ucComMF522Buf[i+8] = *(pSnr+i);   }
	//   memcpy(&ucComMF522Buf[2], pKey, 6); 
	//   memcpy(&ucComMF522Buf[8], pSnr, 4); 

	status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
	if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
	{   status = MI_ERR;   }

	return status;
}


char PcdRead(unsigned char addr,unsigned char *pData)
{
	char status;
	unsigned int unLen;
	unsigned char i,ucComMF522Buf[MAXRLEN]; 

	ucComMF522Buf[0] = PICC_READ;
	ucComMF522Buf[1] = addr;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
	if ((status == MI_OK) && (unLen == 0x90))
		//   {   memcpy(pData, ucComMF522Buf, 16);   }
	{
		for (i=0; i<16; i++)
		{    *(pData+i) = ucComMF522Buf[i];   }
	}
	else
	{   status = MI_ERR;   }

	return status;
}

char PcdWrite(unsigned char addr,unsigned char *pData)
{
	char status;
	unsigned int unLen;
	unsigned char i,ucComMF522Buf[MAXRLEN]; 

	ucComMF522Buf[0] = PICC_WRITE;
	ucComMF522Buf[1] = addr;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

	if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
	{   status = MI_ERR;   }

	if (status == MI_OK)
	{
		//memcpy(ucComMF522Buf, pData, 16);
		for (i=0; i<16; i++)
		{    
			ucComMF522Buf[i] = *(pData+i);   
		}
		CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

		status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
		if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
		{   status = MI_ERR;   }
	}

	return status;
}


char PcdHalt(void)
{
	char status;
	unsigned int unLen;
	unsigned char ucComMF522Buf[MAXRLEN]; 

	ucComMF522Buf[0] = PICC_HALT;
	ucComMF522Buf[1] = 0;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

	return MI_OK;
}


void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
	unsigned char i,n;
	ClearBitMask(DivIrqReg,0x04);
	WriteRawRC(CommandReg,PCD_IDLE);
	SetBitMask(FIFOLevelReg,0x80);
	for (i=0; i<len; i++)
	{   WriteRawRC(FIFODataReg, *(pIndata+i));   }
	WriteRawRC(CommandReg, PCD_CALCCRC);
	i = 0xFF;
	do 
	{
		n = ReadRawRC(DivIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x04));
	pOutData[0] = ReadRawRC(CRCResultRegL);
	pOutData[1] = ReadRawRC(CRCResultRegM);
}


char PcdReset(void)
{
	SET_RC522RST;
	delay_ns(10);
	CLR_RC522RST;
	delay_ns(10);
	SET_RC522RST;
	delay_ns(10);
	WriteRawRC(CommandReg,PCD_RESETPHASE);
	delay_ns(10);

	WriteRawRC(ModeReg,0x3D);            //和Mifare卡通讯，CRC初始值0x6363
	WriteRawRC(TReloadRegL,30);           
	WriteRawRC(TReloadRegH,0);
	WriteRawRC(TModeReg,0x8D);
	WriteRawRC(TPrescalerReg,0x3E);

	WriteRawRC(TxAutoReg,0x40);//必须要

	return MI_OK;
}

char M500PcdConfigISOType(unsigned char type)
{
	if (type == 'A')                     //ISO14443_A
	{ 
		ClearBitMask(Status2Reg,0x08);
		WriteRawRC(ModeReg,0x3D);//3F
		WriteRawRC(RxSelReg,0x86);//84
		WriteRawRC(RFCfgReg,0x7F);   //4F
		WriteRawRC(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
		WriteRawRC(TReloadRegH,0);
		WriteRawRC(TModeReg,0x8D);
		WriteRawRC(TPrescalerReg,0x3E);
		delay_ns(1000);
		PcdAntennaOn();
	}
	else{ return -1; }

	return MI_OK;
}

unsigned char ReadRawRC(unsigned char Address)
{
	unsigned char ucAddr;
	unsigned char ucResult=0;
	CLR_SPI_CS;
	ucAddr = ((Address<<1)&0x7E)|0x80;

	//	SPIWriteByte(ucAddr);
	//	ucResult=SPIReadByte();
	int ret = spi_write_then_read(rc522_spi, &ucAddr, 1, &ucResult, 1);
	if(ret != 0) {
		printk("spi_write_then_read err = %d\n", ret);
	}
	SET_SPI_CS;
	return ucResult;
}


void WriteRawRC(unsigned char Address, unsigned char value)
{  
	unsigned char ucAddr;

	CLR_SPI_CS;
	ucAddr = ((Address<<1)&0x7E);

	//	SPIWriteByte(ucAddr);
	//	SPIWriteByte(value);
	struct spi_transfer st[2];  
	struct spi_message  msg; 
	spi_message_init( &msg );
	memset( st, 0, sizeof(st) );

	st[ 0 ].tx_buf = &ucAddr;  
	st[ 0 ].len = 1;  
	spi_message_add_tail( &st[0], &msg );  

	st[ 1 ].tx_buf = &value;  
	st[ 1 ].len = 1;  
	spi_message_add_tail( &st[1], &msg );  
	spi_sync( rc522_spi, &msg );

	SET_SPI_CS;
}

void SetBitMask(unsigned char reg,unsigned char mask)  
{
	char tmp = 0x0;
	tmp = ReadRawRC(reg);
	WriteRawRC(reg,tmp | mask);  // set bit mask
}


void ClearBitMask(unsigned char reg,unsigned char mask)  
{
	char tmp = 0x0;
	tmp = ReadRawRC(reg);
	WriteRawRC(reg, tmp & ~mask);  // clear bit mask
} 


char PcdComMF522(unsigned char Command, 
		unsigned char *pInData, 
		unsigned char InLenByte,
		unsigned char *pOutData, 
		unsigned int *pOutLenBit)
{
	char status = MI_ERR;
	unsigned char irqEn   = 0x00;
	unsigned char waitFor = 0x00;
	unsigned char lastBits;
	unsigned char n;
	unsigned int i;
	switch (Command)
	{
		case PCD_AUTHENT:
			irqEn   = 0x12;
			waitFor = 0x10;
			break;
		case PCD_TRANSCEIVE:
			irqEn   = 0x77;
			waitFor = 0x30;
			break;
		default:
			break;
	}

	WriteRawRC(ComIEnReg,irqEn|0x80);
	ClearBitMask(ComIrqReg,0x80);
	WriteRawRC(CommandReg,PCD_IDLE);
	SetBitMask(FIFOLevelReg,0x80);

	for (i=0; i<InLenByte; i++)
	{   WriteRawRC(FIFODataReg, pInData[i]);    }
	WriteRawRC(CommandReg, Command);


	if (Command == PCD_TRANSCEIVE)
	{    SetBitMask(BitFramingReg,0x80);  }


	i = 2000;
	do 
	{
		n = ReadRawRC(ComIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x01) && !(n&waitFor));
	ClearBitMask(BitFramingReg,0x80);

	if (i!=0)
	{    
		if(!(ReadRawRC(ErrorReg)&0x1B))
		{
			status = MI_OK;
			if (n & irqEn & 0x01)
			{   status = MI_NOTAGERR;   }
			if (Command == PCD_TRANSCEIVE)
			{
				n = ReadRawRC(FIFOLevelReg);
				lastBits = ReadRawRC(ControlReg) & 0x07;
				if (lastBits)
				{   *pOutLenBit = (n-1)*8 + lastBits;   }
				else
				{   *pOutLenBit = n*8;   }
				if (n == 0)
				{   n = 1;    }
				if (n > MAXRLEN)
				{   n = MAXRLEN;   }
				for (i=0; i<n; i++)
				{   pOutData[i] = ReadRawRC(FIFODataReg);    }
			}
		}
		else
		{   status = MI_ERR;   }

	}


	SetBitMask(ControlReg,0x80);           // stop timer now
	WriteRawRC(CommandReg,PCD_IDLE); 
	return status;
}


void PcdAntennaOn(void)
{
	unsigned char i;
	i = ReadRawRC(TxControlReg);
	if (!(i & 0x03))
	{
		SetBitMask(TxControlReg, 0x03);
	}
}


void PcdAntennaOff(void)
{
	ClearBitMask(TxControlReg, 0x03);
}


