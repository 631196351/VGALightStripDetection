#pragma once
#include "nvbase.h"
#define ENE_ADDR 0XCE
#define CMD_SETADDR 0
#define CMD_WRIE_BYTE 1
#define CMD_WRIE_WORD 2
#define CMD_WRIE_BLOCK 3
#define CMD_READ_BYTE 0X81
#define CMD_READ_WORD 0X82

#define CMD_ISP_WRITE_PAGE 0X05
#define CMD_ISP_READ_PAGE 0X06

extern unsigned char aura8020[28672];

int eneISPRead(NvU16 addr, NvU8 *pBytes, NvU16 nBytes);
int eneISPWrite(NvU16 addr, NvU8 *pBytes, NvU16 nBytes);
int eneFlashWrite(NvU16 addr, NvU8 *pBytes, NvU16 nBytes);
int eneFlashRead(NvU16 addr, NvU8 *pBytes, NvU16 nBytes);
int enePageErase(NvU16 addr);
void  eneChipErase();
void stop_8051();
int check_FMC_STS();
void   allChkSumCal(NvU8 * buf, NvU16 len);
NvAPI_Status eneWriteRegs(NvU16 reg, NvU8 *data, NvU8 size);
NvAPI_Status eneWriteReg(NvU16 regaddr, NvU8 data);
NvAPI_Status eneReadReg(NvU16 regaddr, NvU8 *data);
NvAPI_Status eneBlockWrite(NvU8 cmd, NvU8 *data, NvU8 size);
NvAPI_Status eneSetRegAddr(NvU16 regAddr);