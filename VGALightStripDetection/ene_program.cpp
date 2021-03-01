#include "ene_program.h"
#include <stdlib.h>
#include <stdio.h>

#include<iostream>
#include  <windows.h>
NvU8 XOR_P_CHK;
NvU8 SUM_P_CHK;
NvU16 SUM_P;
NvAPI_Status eneSetRegAddr(NvU16 regAddr)
{
	NvU8 buf[2];
	buf[0] = (regAddr >> 8) & 0xff;
	buf[1] = regAddr & 0xff;
	return nvi2cWriteBlock(ENE_ADDR, CMD_SETADDR, buf, 2);

}

NvAPI_Status eneWriteReg(NvU16 regaddr,NvU8 data)
{
	eneSetRegAddr(regaddr);
	return nvi2cWriteByte(ENE_ADDR, CMD_WRIE_BYTE, data);
}

NvAPI_Status eneReadReg(NvU16 regaddr, NvU8 *data)
{
	eneSetRegAddr(regaddr);
	return nvi2cReadByte(ENE_ADDR, CMD_READ_BYTE, data);
}
NvAPI_Status WriteReg(NvU16 reg, NvU8 data)
{
	//OK
	//I2C
	//Set ADDR_H
	//[0xCE][0x09][Register & 0xFF]
	//	//Set ADDR_L
	//	[0xCE][0x09][Register & 0xFF00 >> 8]
	//	//Set Data
	//	[0xCE][0x01][data]
	
	eneSetRegAddr(reg);
	return nvi2cWriteByte(ENE_ADDR, CMD_WRIE_BYTE, data);
}



NvAPI_Status eneWriteRegs(NvU16 reg, NvU8 *data,NvU8 size)
{
	int  status = 0;
	NvU8 *p = (NvU8 *)malloc(size + 1);
	p[0] = size;
	for (size_t i = 1; i <= size; i++)
	{
		p[i] = data[i - 1];
	}

	status+=eneSetRegAddr(reg);
	
	status += nvi2cWriteBlock(ENE_ADDR, CMD_WRIE_BLOCK, p,size+1);
	free(p);
	return (NvAPI_Status)status;
}
NvAPI_Status eneBlockWrite(NvU8 cmd, NvU8 *data, NvU8 size)
{
	int  status = 0;
	NvU8 *p = (NvU8 *)malloc(size + 1);
	p[0] = size;
	for (size_t i = 1; i <= size; i++)
	{
		p[i] = data[i - 1];
	}

	status += nvi2cWriteBlock(ENE_ADDR, cmd, p, size + 1);
	free(p);
	return (NvAPI_Status)status;
}
NvAPI_Status eneReadRegs(NvU16 regaddr, NvU8 *data,NvU8  size)
{
	eneSetRegAddr(regaddr);
	return nvi2cReadByte(ENE_ADDR, 0x80+size, data);
}

int check_FMC_STS()
{
	NvU8 atmp = 0;
	NvU8 to = 0;

	
	eneReadReg(0xF800, &atmp);
	while (atmp & 0x80 == 0)
	{
			to++;	
			if (to >= 20)
			{
				break;
			}
	}
	
	if (to>20)
	{
		return 0;
	}
	return 1;
}
void stop_8051()
{
	
	NvU8 tmp;
	WriteReg(0xF100, 0x00);  //Stop WDT
	WriteReg(0xF010, 0x05);  //Stop 8051

							 //OSC32M Fill
	WriteReg(0xF808, 0xF0);
	WriteReg(0xF809, 0x01);
	WriteReg(0xF807, 0x90);

	if (check_FMC_STS() == 0)
	{
		printf("stop8051 fail");
	}
	eneReadReg(0xf80b, &tmp);
	WriteReg(0xF807, tmp);
	//Flash Timing Program
	WriteReg(0xF815, 0x10);
	WriteReg(0xF816, 0x11);
	WriteReg(0xF817, 0x06);
	WriteReg(0xF818, 0x07);


}
int eneSetFlashAddr(NvU16 addr)
{
	int res = 0;
	res+=WriteReg(0xF808, (addr & 0xFF));
	res+=WriteReg(0xF809, (addr>>8)&0xff);
	return res;

}
int enePageErase(NvU16 addr)	//OK : Sam
{
	NvU8 atmp;
	int res = 0;
	WriteReg(0xf808, (NvU8)(addr & 0xff));
	WriteReg(0xf809, (NvU8)((addr >> 8)) & 0xff);
	WriteReg(0xF807, 0x20);
	if (check_FMC_STS() == 0)
		printf("F800 wait Fail");
		

	return 1;

}
void  eneChipErase()
{
	
	for (size_t i = 0; i < 0x7000; i+=0x80)
	{
		enePageErase(i);
		printf("ereaseing addr %x\n",i);
	}
}
int eneFlashWrite(NvU16 addr, NvU8 *pBytes, NvU16 nBytes)
{
	if (eneSetFlashAddr(addr))
			return 0;
	WriteReg(0xF807, 0x80);
	//4.	Wait 0xF800[7]=1							//wait xbi idel
	if (check_FMC_STS()==0)
	{
		return 0;
	}
	
	for (size_t i = 0; i < nBytes; i++)
	{

		WriteReg(0xf0a, pBytes[i]);
		WriteReg(0xf807, 2);
		if (eneSetFlashAddr == 0)
			return 0;
		addr++;
		//eneSetFlashAddr(addr);
		WriteReg(0xf808, addr & 0xff);

	}
	WriteReg(0xf807, 0x70);
	if (check_FMC_STS() == 0)
		return 0;
	return 1;
}

int eneISPWrite(NvU16 addr, NvU8 *pBytes, NvU16 nBytes)
{
	

	int  status = NVAPI_OK;
	
	status = WriteReg(0xF807, 0x80);
	//4.	Wait 0xF800[7]=1							//wait xbi idel
	if (check_FMC_STS()==0)
		return 0;
	status = eneSetFlashAddr(addr);
	status = eneSetRegAddr(0xF80A);	
	Sleep(1);
	status = nvi2cWriteBlock(ENE_ADDR, 0x5, pBytes, nBytes);
	
	//status = WriteReg(0xF807, 0x70);
	eneSetRegAddr(0xf807);
	nvi2cWriteByte(ENE_ADDR, 0x05, 0x70);
	if (check_FMC_STS() == 0)
		printf("check fmc status fail");

	return 1;
}

int eneISPRead(NvU16 addr, NvU8 *pBytes, NvU16 nBytes)
{
	int bOK;
	int i;
	NvU8 dattmp;
	int re=0;
	//Set Address = 0x0000
	re += WriteReg(0xF808, (addr & 0x00FF));
	re += WriteReg(0xF809, (addr & 0xFF00) >> 8);
	re += eneSetRegAddr(0xf80b);
	re += nvi2cReadBlock(ENE_ADDR, 0x6, pBytes, nBytes);
	return re;

}

int  eneFlashRead(NvU16 addr, NvU8 *pBytes, NvU16 nBytes)
{
	int re=0;
	NvU8 temp;
	re+=eneSetFlashAddr(addr);
	for (size_t i = 0; i < nBytes; i++)
	{
		re += WriteReg(0xf807, 0x03); //read data cmd
		if (check_FMC_STS() == 0)
			return 0;

		re += eneReadReg(0xf80b, &temp);
		pBytes[i] = temp;
		addr++;
		re += WriteReg(0xf808, addr & 0xff);
	}
	
	return re;

}

//void   eneGetCheckSum(NvU8 * buf, NvU16 len)
//{
//	NvU16 sum, wordsum = 0;
//	NvU16 xor = 0;
//	for (size_t i = 0; i < len; i++)
//	{
//		wordsum += buf[i];
//		xor ^= buf[i];
//	}
//	printf("check sum: %x\nxor %x\n", wordsum, xor);
//}

void Program_chksum(NvU16 filesize)
{
	//BYTE check_info[16];
	//for (BYTE i = 0; i<16; i++)
	//	check_info[i] = 0;
	//ISPPageErase(0x6F80);   //0x6F80~0x6FFF
	//check_info[0] = 0x00FF & filesize;
	//check_info[1] = ((0xFF00 & filesize) >> 8);
	//check_info[6] = 0x55;   //Partial Checksum
	//check_info[7] = SUM_P_CHK;   //Partial Checksum
	//check_info[8] = XOR_P_CHK;   //Partial Checksum
	//check_info[10] = SUM_P & 0xFF;
	//check_info[11] = (0xFF00 & SUM_P) >> 8;
	//ISPWrite(0x6FF0, check_info, 16);
}


void  eneflash( unsigned char * aura8020)
{


	NvU8  readBuf[0x80] = { 0 };
	NvU8  temp = 0;
	stop_8051();
	//erease
	for (size_t i = 0; i < 0x7000; i += 0x80)
	{
		enePageErase(i);
		printf("erase %x\n", i);
	}
	Sleep(100);
	//programe
	for (size_t i = 0; i < 0x6E00; i += 0x80)
	{
		eneISPWrite(i, &(aura8020[i]), 0x80);
		printf("isp write addr %x\n", i);
	}

	eneISPWrite(0x6f80, &aura8020[0x6f80], 0x80);
	Sleep(100);

	NvU16 checkAddr = 0;
	NvU16 checkFail = 0;
	NvU16 failTimes = 0;
	for (size_t i = 0; i < 0x3800; i += 0x80)
	{

		if (eneISPRead(i, readBuf, 0x80) == 0)
		{
			checkFail = 0;
			for (size_t t = 0; t < 0x80; t++)
			{
				if (aura8020[checkAddr++] != readBuf[t])
				{
					printf("%x check fail\n", checkAddr);
					checkFail = 1;
					failTimes++;
					//break;
					printf("%x check fail,reflash %x\n", i, i);
					eneISPWrite(i, &aura8020[i], 0x80);

					if (failTimes > 30)
					{
						printf("fail times over 10,please reset the 6k582\n");
						system("pause");
					}
					i -= 0x80;
					checkAddr = i;
					break;
					//checkFail = 0;
				}
				else
				{
					printf("%x verify ok\n", i);
				}


			}
		}
		else
		{
			printf("read addr %x fail\n", i);
			system("pause");
		}
	}

	//reset 
	eneWriteReg(0xf010, 4);
	Sleep(100);

	Sleep(2000);


	eneReadReg(0x8021, &temp);
	if (temp == 0x5)
	{
		printf("program ok\n");
		eneWriteReg(0x8021, 0xb);
		eneWriteReg(0x802f, 0x1);
	}
	else
	{
		printf("program ok but  ic  didn't work\n");
	}
}


//void ereaseChip()
//{
//	stop_8051();
//	for (size_t i = 0; i < 0x7000; i += 0x80)
//	{
//		enePageErase(i);
//		printf("erase %x\n", i);
//		Sleep(5);
//	}
//
//	eneWriteReg(0xf010, 4);
//}