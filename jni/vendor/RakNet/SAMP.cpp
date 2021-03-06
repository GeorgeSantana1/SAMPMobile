#include "SAMP.h"

uint16_t usNetGameCookie = 0x6969;
uint16_t usNetGameVersion = 0x0FD9;

#define endian_swap8(x) (x)
#define endian_swap16(x) ((x>>8) | (x<<8))
#define endian_swap32(x) ((x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24))
#define endian_swap64(x) ((x>>56) | ((x<<40) & 0x00FF000000000000) | \
		((x<<24) & 0x0000FF0000000000) | ((x<<8)  & 0x000000FF00000000) | \
		((x>>8)  & 0x00000000FF000000) | ((x>>24) & 0x0000000000FF0000) | \
		((x>>40) & 0x000000000000FF00) | (x<<56))

#define ROTL(value, shift) ((value << shift) | (value >> (sizeof(value)*8 - shift)))
#define ROTR(value, shift) ((value >> shift) | (value << (sizeof(value)*8 - shift)))
#define swap(x,y,T) {T tmp = x; x = y; y = tmp;}

unsigned char encrBuffer[4092];
unsigned char sampEncrTable[256] =
{
	0x27, 0x69, 0xFD, 0x87, 0x60, 0x7D, 0x83, 0x02, 0xF2, 0x3F, 0x71, 0x99, 0xA3, 0x7C, 0x1B, 0x9D,
	0x76, 0x30, 0x23, 0x25, 0xC5, 0x82, 0x9B, 0xEB, 0x1E, 0xFA, 0x46, 0x4F, 0x98, 0xC9, 0x37, 0x88,
	0x18, 0xA2, 0x68, 0xD6, 0xD7, 0x22, 0xD1, 0x74, 0x7A, 0x79, 0x2E, 0xD2, 0x6D, 0x48, 0x0F, 0xB1,
	0x62, 0x97, 0xBC, 0x8B, 0x59, 0x7F, 0x29, 0xB6, 0xB9, 0x61, 0xBE, 0xC8, 0xC1, 0xC6, 0x40, 0xEF,
	0x11, 0x6A, 0xA5, 0xC7, 0x3A, 0xF4, 0x4C, 0x13, 0x6C, 0x2B, 0x1C, 0x54, 0x56, 0x55, 0x53, 0xA8,
	0xDC, 0x9C, 0x9A, 0x16, 0xDD, 0xB0, 0xF5, 0x2D, 0xFF, 0xDE, 0x8A, 0x90, 0xFC, 0x95, 0xEC, 0x31,
	0x85, 0xC2, 0x01, 0x06, 0xDB, 0x28, 0xD8, 0xEA, 0xA0, 0xDA, 0x10, 0x0E, 0xF0, 0x2A, 0x6B, 0x21,
	0xF1, 0x86, 0xFB, 0x65, 0xE1, 0x6F, 0xF6, 0x26, 0x33, 0x39, 0xAE, 0xBF, 0xD4, 0xE4, 0xE9, 0x44,
	0x75, 0x3D, 0x63, 0xBD, 0xC0, 0x7B, 0x9E, 0xA6, 0x5C, 0x1F, 0xB2, 0xA4, 0xC4, 0x8D, 0xB3, 0xFE,
	0x8F, 0x19, 0x8C, 0x4D, 0x5E, 0x34, 0xCC, 0xF9, 0xB5, 0xF3, 0xF8, 0xA1, 0x50, 0x04, 0x93, 0x73,
	0xE0, 0xBA, 0xCB, 0x45, 0x35, 0x1A, 0x49, 0x47, 0x6E, 0x2F, 0x51, 0x12, 0xE2, 0x4A, 0x72, 0x05,
	0x66, 0x70, 0xB8, 0xCD, 0x00, 0xE5, 0xBB, 0x24, 0x58, 0xEE, 0xB4, 0x80, 0x81, 0x36, 0xA9, 0x67,
	0x5A, 0x4B, 0xE8, 0xCA, 0xCF, 0x9F, 0xE3, 0xAC, 0xAA, 0x14, 0x5B, 0x5F, 0x0A, 0x3B, 0x77, 0x92,
	0x09, 0x15, 0x4E, 0x94, 0xAD, 0x17, 0x64, 0x52, 0xD3, 0x38, 0x43, 0x0D, 0x0C, 0x07, 0x3C, 0x1D,
	0xAF, 0xED, 0xE7, 0x08, 0xB7, 0x03, 0xE6, 0x8E, 0xAB, 0x91, 0x89, 0x3E, 0x2C, 0x96, 0x42, 0xD9,
	0x78, 0xDF, 0xD0, 0x57, 0x5D, 0x84, 0x41, 0x7E, 0xCE, 0xF7, 0x32, 0xC3, 0xD5, 0x20, 0x0B, 0xA7
};

void SHA1(char *message, unsigned long *out)
{	
	unsigned long h0 = 0x67452301;
	unsigned long h1 = 0xEFCDAB89;
	unsigned long h2 = 0x98BADCFE;
	unsigned long h3 = 0x10325476;
	unsigned long h4 = 0xC3D2E1F0;

	unsigned long len = 0;
	unsigned long long bitlen = 0;

	while (message[len])
	{
		len++;
		bitlen += 8;
	}

	unsigned long complement = (55 - (len%56)) + 8*(((len+8)/64));
	unsigned long newlen = len + complement + 8 + 1;
	char *pMessage = new char[newlen];
	if (!pMessage)
		return;

	memcpy(pMessage, message, len);
	pMessage[len] = -128;
	memset(pMessage+len+1, 0, complement);

	*(unsigned long long*)&pMessage[len + 1 + complement] = endian_swap64(bitlen);

	unsigned long chunks = newlen/64;
	unsigned long w[80];

	for (unsigned long x = 0; x < chunks; x++)
	{
		for (unsigned long i = 0; i < 16; i++)
			w[i] = endian_swap32(*(unsigned long*)(&pMessage[x*64 + i*4]));

		memset(&w[16], 0, 64*4);

		for (unsigned long i = 16; i <= 79; i++)
			w[i] = ROTL((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);

		unsigned long a = h0;
		unsigned long b = h1;
		unsigned long c = h2;
		unsigned long d = h3;
		unsigned long e = h4;

		for (unsigned long i = 0; i <= 79; i++)
		{
			unsigned long f;
			unsigned long k;

			if (0 <= i && i <= 19)
			{
				f = (b & c) | ((~b) & d);
				k = 0x5A827999;
			}
			else if (20 <= i && i <= 39)
			{
				f = b ^ c ^ d;
				k = 0x6ED9EBA1;
			}
			else if (40 <= i && i <= 59)
			{
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDC;
			}
			else if (60 <= i && i <= 79)
			{
				f = b ^ c ^ d;
				k = 0xCA62C1D6;
			}

			unsigned long temp = (ROTL(a, 5) + f + e + k + w[i])&0xFFFFFFFF;
			e = d;
			d = c;
			c = ROTL(b, 30);
			b = a;
			a = temp;
		}

		h0 = (h0 + a)&0xFFFFFFFF;
		h1 = (h1 + b)&0xFFFFFFFF;
		h2 = (h2 + c)&0xFFFFFFFF;
		h3 = (h3 + d)&0xFFFFFFFF;
		h4 = (h4 + e)&0xFFFFFFFF;
	}

	delete [] pMessage;

	out[0] = h0;
	out[1] = h1;
	out[2] = h2;
	out[3] = h3;
	out[4] = h4;
}

void SHA1(char *message, char buf[64])
{
	if (!buf) {
		return;
	}
	
	unsigned long out[5];
	SHA1(message, out);
	sprintf(buf, "%.8X%.8X%.8X%.8X%.8X", out[0], out[1], out[2], out[3], out[4]);
}

const static uint8_t auth_hash_transform_table[100] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x08, 0x06, 0x00, 0x00, 0x00, 0xE4, 0xB5, 0xB7, 0x0A, 0x00, 0x00, 0x00,
	0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0B, 0x13, 0x00, 0x00, 0x0B,
	0x13, 0x01, 0x00, 0x9A, 0x9C, 0x18, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41,
	0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8E, 0x7C, 0xFB, 0x51, 0x93, 0x00, 0x00,
	0x00, 0x20, 0x63, 0x48, 0x52, 0x4D, 0x00, 0x00, 0x7A, 0x25, 0x00, 0x00,
	0x80, 0x83, 0x00, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x80, 0xE9, 0x00, 0x00,
	0x75, 0x30, 0x00, 0x00
};

uint8_t transform_auth_sha1(uint8_t value, uint8_t _xor)
{
	uint8_t result = value;

	for(uint8_t i = 0; i < 100; i++)
	{
		result = result ^ auth_hash_transform_table[i] ^ _xor;
	}

	return result;
}

const static uint8_t code_from_CAnimManager_AddAnimation[20] = {
	0xFF, 0x25, 0x34, 0x39,
	0x4D, 0x00, 0x90, 0x90,
	0x90, 0x90, 0x56, 0x57,
	0x50, 0x8B, 0x44, 0x24,
	0x14, 0x8D, 0x0C, 0x80
};

char doSampDumbShit(uint8_t a1)
{
	char result = a1 + '0';
	if(a1 + '0' > '9') {
		result = a1 + '7';
	}
	return result;
}

void auth_stringify(char *out, uint8_t* hash)
{
	uint8_t i = 0;
	uint8_t* j = hash;

	do {
		out[i] = doSampDumbShit(*j >> 4); i++;
		out[i] = doSampDumbShit(*j & 0xF); i++;

		j++;
	}
	while(i < 40);

	out[i] = '\0';
}

void gen_auth_key(char buf[260], char* auth_in)
{
	char message[260];
	if(!auth_in) return;
	sprintf(message, "%s", auth_in);

	unsigned long out[5];
	uint8_t *pb_out = (uint8_t*)&out;

	SHA1(message, out);

	for(uint8_t i = 0; i < 5; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0x2F); }
	for(uint8_t i = 5; i < 10; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0x45); }
	for(uint8_t i = 10; i < 15; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0x6F); }
	for(uint8_t i = 15; i < 20; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0xDB); }
	for(uint8_t i = 0; i < 20; i++) { pb_out[i] ^= code_from_CAnimManager_AddAnimation[i]; }

	auth_stringify(buf, pb_out);
}

void encryptData(unsigned char *buf, int len, int port, int unk)
{
	unsigned char bChecksum = 0;
	for(int i = 0; i < len; i++)
	{
		unsigned char bData = buf[i];
		bChecksum ^= bData & 0xAA;
	}
	encrBuffer[0] = bChecksum;

	unsigned char *buf_nocrc = &encrBuffer[1];
	memcpy(buf_nocrc, buf, len);

	for(int i = 0; i < len; i++) {
		buf_nocrc[i] = sampEncrTable[buf_nocrc[i]];
		
		if(unk) {
			buf_nocrc[i] ^= (uint8_t)(port ^ 0xCC);
		}
		unk ^= 1u;
	}
}