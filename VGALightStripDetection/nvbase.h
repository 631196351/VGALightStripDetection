#pragma once
#include "nvapi.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

NvAPI_Status  nvi2cinit();
NvAPI_Status nvi2cReadBlock(NvU8 slaveAddr, NvU8 regAddr, NvU8 *readBuf, NvU8 size);
NvAPI_Status nvi2cWriteBlock(NvU8 slaveAddr, NvU8 regAddr, NvU8 *writeBuf, NvU8 size);
NvAPI_Status nvi2cWriteByte(NvU8 slaveAddr, NvU8 regAddr, NvU8 buf);
NvAPI_Status nvi2cReadByte(NvU8 slaveAddr, NvU8 regAddr, NvU8 *readBuf);