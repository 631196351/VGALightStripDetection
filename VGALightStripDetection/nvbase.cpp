#include "nvbase.h"
NvPhysicalGpuHandle  hgpu[NVAPI_MAX_PHYSICAL_GPUS];
NvU32 gpuCount;
NV_I2C_INFO i2cInfo = { 0 };
NV_I2C_INFO_EX i2cInfoEx;
NvAPI_Status  nv_status;



NvAPI_Status  nvi2cinit()
{
	NvAPI_Status nv_init_status;
	nv_init_status = NvAPI_Initialize();
	if (nv_init_status == NVAPI_OK)
	{
		nv_init_status = NvAPI_EnumPhysicalGPUs(hgpu, &gpuCount);
	}


	i2cInfo.version = NV_I2C_INFO_VER;
	i2cInfo.displayMask = 0x0;

	i2cInfo.portId = 1;
	i2cInfo.bIsDDCPort = 0;
	i2cInfo.bIsPortIdSet = 1;

	i2cInfo.i2cSpeed = NVAPI_I2C_SPEED_DEPRECATED;
	i2cInfo.i2cSpeedKhz = NVAPI_I2C_SPEED_100KHZ;


	i2cInfoEx.flags = 1;
	i2cInfoEx.encrClientID = 0;

	return nv_init_status;
}

NvAPI_Status nvi2cReadBlock(NvU8 slaveAddr, NvU8 regAddr, NvU8 *readBuf, NvU8 size)
{
	i2cInfo.i2cDevAddress = slaveAddr;
	i2cInfo.regAddrSize = 1;
	i2cInfo.pbI2cRegAddress = &regAddr;
	i2cInfo.pbData = readBuf;
	i2cInfo.cbSize = size;
	return NvAPI_I2CReadEx(hgpu[0], &i2cInfo, &i2cInfoEx);

}
NvAPI_Status nvi2cReadByte(NvU8 slaveAddr, NvU8 regAddr, NvU8 *readBuf)
{
	i2cInfo.i2cDevAddress = slaveAddr;
	i2cInfo.regAddrSize = 1;
	i2cInfo.pbI2cRegAddress = &regAddr;
	i2cInfo.pbData = readBuf;
	i2cInfo.cbSize = 1;
	return NvAPI_I2CReadEx(hgpu[0], &i2cInfo, &i2cInfoEx);

}
NvAPI_Status nvi2cWriteBlock(NvU8 slaveAddr, NvU8 regAddr, NvU8 *writeBuf, NvU8 size)
{
	i2cInfo.i2cDevAddress = slaveAddr;
	i2cInfo.regAddrSize = 1;
	i2cInfo.pbI2cRegAddress = &regAddr;
	i2cInfo.pbData = writeBuf;
	i2cInfo.cbSize = size;
	return NvAPI_I2CWriteEx(hgpu[0], &i2cInfo, &i2cInfoEx);
}
NvAPI_Status nvi2cWriteByte(NvU8 slaveAddr, NvU8 regAddr, NvU8 buf)
{
	NvU8 writebuf[1];
	writebuf[0] = buf;
	i2cInfo.i2cDevAddress = slaveAddr;
	i2cInfo.regAddrSize = 1;
	i2cInfo.pbI2cRegAddress = &regAddr;
	i2cInfo.pbData = writebuf;
	i2cInfo.cbSize = 1;
	return NvAPI_I2CWriteEx(hgpu[0], &i2cInfo, &i2cInfoEx);
}

