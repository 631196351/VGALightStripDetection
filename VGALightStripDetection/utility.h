#pragma once

int getVGAInfo(char* ppid, size_t size);


typedef unsigned long Ul32;
typedef unsigned char BYTE;

#if 0
void setColor(unsigned char red, unsigned char blue, unsigned char green)
{
	unsigned char buf[90] = { 0 };
	for (size_t i = 0; i < 90; i += 3)
	{
		buf[i] = red;
		buf[i + 1] = blue;
		buf[i + 2] = green;
	}
	for (size_t j = 0; j < 90; j += 30)
	{
		eneWriteRegs(0x8160 + j, &buf[j], 30);
	}
	eneWriteReg(0x8021, 1);
	eneWriteReg(0x802f, 1);
}

void setSingleColor(u8 ledNumlight, u8 ledNumDelight, u8 r, u8 g, u8 b)
{
	u8 bufLight[3] = { r,g,b };	// Ҫ�򿪵ĵ���ɫ
	u8 bufDelight[3] = { 0,0,0 };// Ҫ�ص��ĵ�

	eneWriteRegs(0x8160 + ledNumlight * 3, bufLight, 3);

	eneWriteRegs(0x8160 + ledNumDelight * 3, bufDelight, 3);

	eneWriteReg(0x8021, 1);	// ����Ϊ��̬
	eneWriteReg(0x802f, 1);	// ��������
}
#else
void initVGA();

// ����Ӳ��ƽ̨, ����led�ƹ�
void setSignleColor(int led, BYTE r, BYTE g, BYTE b);

// ����Ϊ�ض���ɫ
void resetColor(int count, BYTE r, BYTE g, BYTE b);
#endif