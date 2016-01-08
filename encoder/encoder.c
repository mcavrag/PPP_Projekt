////////////////////////////////////////////////////////////////////////////////
//
//      Custom JPEG encoder
//          for FER PPP course
//
//      - Assumptions:
//          - maximum image size known
//          - image received as a RGB value list via stdin
//          - image width and height multiples of 8
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#pragma pack(1)

#define MAX 4096

struct BitMap
{
	unsigned short int Type;
	unsigned int Size;
	unsigned short int Reserved1, Reserved2;
	unsigned int Offset;
} Header;

struct BitMapInfo
{
	unsigned int Size;
	int Width, Height;
	unsigned short int Planes;
	unsigned short int Bits;
	unsigned int Compression;
	unsigned int ImageSize;
	int xResolution, yResolution;
	unsigned int Colors;
	unsigned int ImportantColors;
} InfoHeader;

struct Pixels
{
	unsigned char Blue, Green, Red;
};

const float RGB_to_YUV_factors[3][4] = {
	{ 0.257, 0.504, 0.098, 16 },
	{ -0.148, -0.291, 0.439, 128 },
	{ 0.439, -0.368, -0.071, 128 }
};
const float DCT_coefficients[8][8] = {
	{ .3536, .3536, .3536, .3536, .3536, .3536, .3536, .3536 },
	{ .4904, .4157, .2778, .0975,-.0975,-.2778,-.4157,-.4094 },
	{ .4619, .1913,-.1913,-.4619,-.4619,-.1913, .1913, .4619 },
	{ .4157,-.0975,-.4904,-.2778, .2778, .4904, .0975,-.4157 },
	{ .3536,-.3536,-.3536, .3536, .3536,-.3536,-.3536, .3536 },
	{ .2778,-.4904, .0975, .4157,-.4157,-.0975, .4904,-.2778 },
	{ .1913,-.4619, .4619,-.1913,-.1913, .4619,-.4619, .1913 },
	{ .0975,-.2778, .4157,-.4904, .4904,-.4157, .2778,-.0975 }
};
const float DCT_coefficients_transp[8][8] = {
	{ .3536, .4904, .4619, .4157, .3536, .2778, .1913, .0975 },
	{ .3536, .4157, .1913,-.0975,-.3536,-.4904,-.4619,-.2778 },
	{ .3536, .2778,-.1913,-.4904,-.3536, .0975, .4619, .4157 },
	{ .3536, .0975,-.4619,-.2778, .3536, .4157,-.1913,-.4904 },
	{ .3536,-.0975,-.4619, .2778, .3536,-.4157,-.1913, .4904 },
	{ .3536,-.2778,-.1913, .4904,-.3536,-.0975, .4619,-.4157 },
	{ .3536,-.4157, .1913, .0975,-.3536, .4904,-.4619, .2778 },
	{ .3536,-.4094, .4619,-.4157, .3536,-.2778, .1913,-.0975 }
};
const short quantization_table_luminance[8][8] = {
	{ 16, 11, 10, 16, 24, 40, 51, 61 },
	{ 12, 12, 14, 19, 26, 58, 60, 55 },
	{ 14, 13, 16, 24, 40, 57, 69, 56 },
	{ 14, 17, 22, 29, 51, 87, 80, 62 },
	{ 18, 22, 37, 56, 68, 109, 103, 77 },
	{ 24, 35, 55, 64, 81, 104, 113, 92 },
	{ 49, 64, 78, 87, 103, 121, 120, 101 },
	{ 72, 92, 95, 98, 112, 100, 103, 99 }
};

const short quantization_table_chrominance[8][8] = {
	{ 17, 18, 24, 47, 99, 99, 99, 99 },
	{ 18, 21, 26, 66, 99, 99, 99, 99 },
	{ 24, 26, 56, 99, 99, 99, 99, 99 },
	{ 47, 66, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 },
	{ 99, 99, 99, 99, 99, 99, 99, 99 }
};

short RGB_image[MAX][MAX][3];
short YUV_image[MAX][MAX][3];
short DCT_image[MAX][MAX][3];
int N, M;

void generate_YUV_image();
void quantize(int, int);
void perform_DCT(int, int);
void read_input();
void shift_values(int, int);

void shift_values(int x, int y) {
	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < 8; ++j)
			for (int k = 0; k < 3; ++k)
				YUV_image[x + i][y + j][k] -= 128;
}

void quantize(int x, int y) {
	int i, j, k;
	for (k = 0; k < 3; k++)
		for (i = 0; i < 8; ++i)
			for (j = 0; j < 8; ++j)
				DCT_image[x + i][y + j][k] /= (k == 0 ? quantization_table_luminance[i][j] :
					quantization_table_chrominance[i][j]);
}

void zig_zag(int x, int y, short ZZ[64], int yuv) {
	int i = 0, j = 0, k = 0, d = 0;
	// do dijagonale
	while (k<36) {
		ZZ[k++] = DCT_image[i + x][j + y][yuv];
		if ((i == 0) && (j % 2 == 0)) {
			j++;
			d = 1;
		}
		else if ((j == 0) && (i % 2 == 1)) {
			i++;
			d = 0;
		}
		else if (d == 0) {
			i--;
			j++;
		}
		else {
			i++;
			j--;
		}
	}
	// poslije dijagonale
	i = 7 + x;
	j = 1 + y;
	d = 0;
	while (k<64) {
		ZZ[k++] = DCT_image[i + x][j + y][yuv];
		if ((i == 7) && (j % 2 == 0)) {
			j++;
			d = 0;
		}
		else if ((j == 7) && (i % 2 == 1)) {
			i++;
			d = 1;
		}
		else if (d == 0) {
			i--;
			j++;
		}
		else {
			i++;
			j--;
		}
	}
}

int RLE(int ZZ[64], int RL[64])
{
	int rl = 1;
	int i = 1;
	int k = 0;
	RL[0] = ZZ[0];
	while (i<64)
	{
		k = 0;
		while ((i<64) && (ZZ[i] == 0) && (k<15))
		{
			i++;
			k++;
		}
		if (i == 64 && rl < 64)
		{

			RL[rl++] = 0;
			RL[rl++] = 0;
		}
		else if(rl < 64)
		{
			RL[rl++] = k;
			RL[rl++] = ZZ[i++];
		} else {
			i++;
		}
	}
	if(rl >= 65) {
		while(rl != 64)
			rl--;
	}

	if (!(RL[rl - 1] == 0 && RL[rl - 2] == 0))
	{
		rl-=2;
		RL[rl++] = 0;
		RL[rl++] = 0;
	}
	if ((RL[rl - 4] == 15) && (RL[rl - 3] == 0))
	{
		RL[rl - 4] = 0;
		rl -= 2;
	}

	return rl;
}
int getCat(int a)
{
	if (a == 0)
		return 0;
	else if (abs(a) <= 1)
		return 1;
	else if (abs(a) <= 3)
		return 2;
	else if (abs(a) <= 7)
		return 3;
	else if (abs(a) <= 15)
		return 4;
	else if (abs(a) <= 31)
		return 5;
	else if (abs(a) <= 63)
		return 6;
	else if (abs(a) <= 127)
		return 7;
	else if (abs(a) <= 255)
		return 8;
	else if (abs(a) <= 511)
		return 9;
	else if (abs(a) <= 1023)
		return 10;
	else if (abs(a) <= 2047)
		return 11;
	else if (abs(a) <= 4095)
		return 12;
	else if (abs(a) <= 8191)
		return 13;
	else if (abs(a) <= 16383)
		return 14;
	else
		return 15;
}

int getDCcode(int a, int lenb, char *b)
{
	int codeLen[12] = { 3,4,5,5,7,8,10,12,14,16,18,20 };
	char* code[12] = { "010","011","100","00","101","110","1110","11110","111110","1111110","11111110","111111110" };
	int cat = getCat(a);
	lenb = codeLen[cat];
	strcpy(b, code[cat]);
	int j;
	int c = a;
	if (a<0)
		c += (int)pow(2, cat) - 1;
	for (j = lenb - 1;j>lenb - cat - 1;j--)
	{
		if (c % 2 == 1)
			b[j] = '1';
		else
			b[j] = '0';
		c /= 2;
	}
	b[lenb] = '\0';
	return lenb;
}

int getACcode(int n, int a, int lenb, char* b)
{
	int codeLen[16][11] = {
		4 ,3 ,4 ,6 ,8 ,10,12,14,18,25,26,
		0 ,5 ,8 ,10,13,16,22,23,24,25,26,
		0 ,6 ,10,13,20,21,22,23,24,25,26,
		0 ,7 ,11,14,20,21,22,23,24,25,26,
		0 ,7 ,12,19,20,21,22,23,24,25,26,
		0 ,8 ,12,19,20,21,22,23,24,25,26,
		0 ,8 ,13,19,20,21,22,23,24,25,26,
		0 ,9 ,13,19,20,21,22,23,24,25,26,
		0 ,9 ,17,19,20,21,22,23,24,25,26,
		0 ,10,18,19,20,21,22,23,24,25,26,
		0 ,10,18,19,20,21,22,23,24,25,26,
		0 ,10,18,19,20,21,22,23,24,25,26,
		0 ,11,18,19,20,21,22,23,24,25,26,
		0 ,12,18,19,20,21,22,23,24,25,26,
		0 ,13,18,19,20,21,22,23,24,25,26,
		12,17,18,19,20,21,22,23,24,25,26
	};
	char* code[16][11] = {
		"1010",  "00",  "01",  "100",  "1011",  "11010",  "111000",  "1111000",  "1111110110",  "1111111110000010",  "1111111110000011",
		"","1100","111001","1111001","111110110","11111110110","1111111110000100","1111111110000101","1111111110000110","1111111110000111","1111111110001000",
		"","11011","11111000","1111110111","1111111110001001","1111111110001010","1111111110001011","1111111110001100","1111111110001101","1111111110001110","1111111110001111",
		"","111010","111110111","11111110111","1111111110010000","1111111110010001","1111111110010010","1111111110010011","1111111110010100","1111111110010101","1111111110010110",
		"","111011","1111111000","1111111110010111","1111111110011000","1111111110011001","1111111110011010","1111111110011011","1111111110011100","1111111110011101","1111111110011110",
		"","1111010","1111111001","1111111110011111","1111111110100000","1111111110100001","1111111110100010","1111111110100011","1111111110100100","1111111110100101","1111111110100110",
		"","1111011","11111111000","1111111110100111","1111111110101000","1111111110101001","1111111110101010","1111111110101011","1111111110101100","1111111110101101","1111111110101110",
		"","11111001","11111111001","1111111110101111","1111111110110000","1111111110110001","1111111110110010","1111111110110011","1111111110110100","1111111110110101","1111111110110110",
		"","11111010","111111111000000","1111111110110111","1111111110111000","1111111110111001","1111111110111010","1111111110111011","1111111110111100","1111111110111101","1111111110111110",
		"","111111000","1111111110111111","1111111111000000","1111111111000001","1111111111000010","1111111111000011","1111111111000100","1111111111000101","1111111111000110","1111111111000111",
		"","111111001","1111111111001000","1111111111001001","1111111111001010","1111111111001011","1111111111001100","1111111111001101","1111111111001110","1111111111001111","1111111111010000",
		"","111111010","1111111111010001","1111111111010010","1111111111010011","1111111111010100","1111111111010101","1111111111010110","1111111111010111","1111111111011000","1111111111011001",
		"","1111111010","1111111111011010","1111111111011011","1111111111011100","1111111111011101","1111111111011110","1111111111011111","1111111111100000","1111111111100001","1111111111100010",
		"","11111111010","1111111111100011","1111111111100100","1111111111100101","1111111111100110","1111111111100111","1111111111101000", "1111111111101001","1111111111101010","1111111111101011",
		"","111111110110","1111111111101100","1111111111101101","1111111111101110","1111111111101111","1111111111110000","1111111111110001","1111111111110010","1111111111110011","1111111111110100",
		"111111110111","1111111111110101","1111111111110110","1111111111110111","1111111111111000","1111111111111001","1111111111111010","1111111111111011","1111111111111100","1111111111111101","1111111111111110"
	};

	int cat = getCat(a);
	lenb = codeLen[n][cat];
	strcpy(b, code[n][cat]);
	int j;
	int c = a;
	if (a<0)
		c += (int)pow(2, cat) - 1;
	for (j = lenb - 1;j>lenb - cat - 1;j--)
	{
		if(j < 0) {
			break;
		}
		if (c % 2 == 1)
			b[j] = '1';
		else
			b[j] = '0';
		c /= 2;
	}
	b[lenb] = '\0';
	return lenb;
}

int Encode(short* RL, int rl, char* output, int prev_dc)
{
	int dc = RL[0];
	RL[0] -= prev_dc;
	
	//   char output[33*26];
	char b[32];
	int bLen = 0;
	bLen = getDCcode(RL[0], bLen, b);
	//   cout<<"Code : "<<RL[0]<<" "<<b;
	strcpy(output, b);
	int i;
	for (i = 1;i<rl;i += 2)
	{
		if(i < 63) {
			if(RL[i] < 0) {
				RL[i]*=-1;
			}
			if(RL[i+1] < 0) {
				RL[i+1]*=-1;
			}
			if(RL[i] > 15) {
				RL[i] = 15;
			}
			bLen = getACcode(RL[i], RL[i + 1], bLen, b);
		//	cout<<" , "<<RL[i]<<" "<<RL[i+1]<<" "<<b;
			strcat(output, b);
		}
	}
	//   writeToFile(output);
	return dc;
}




void read_input1(void) {

	int i = 0, j = 0;
	int size_spix;
	int padding = 0;
	char temp[4];
	struct Pixels **pixel_arrayp;

	FILE *BMP_in = fopen("test.bmp", "rb");
	if (BMP_in == NULL) {

		printf("Soap test.bmp ne moze se otvoriti");
		exit(1);
	}


	fread(&Header.Type, sizeof(Header.Type), 1, BMP_in);
	fread(&Header.Size, sizeof(Header.Size), 1, BMP_in);
	fread(&Header.Reserved1, sizeof(Header.Reserved1), 1, BMP_in);
	fread(&Header.Reserved2, sizeof(Header.Reserved2), 1, BMP_in);
	fread(&Header.Offset, sizeof(Header.Offset), 1, BMP_in);
	fread(&InfoHeader, sizeof(InfoHeader), 1, BMP_in);


	padding = InfoHeader.Width % 4;
	if (padding != 0) {
		padding = 4 - padding;
	}

	size_spix = sizeof(struct Pixels);


	pixel_arrayp = (struct Pixels **)calloc(InfoHeader.Height, sizeof(struct Pixel*));


	for (i = 0; i<InfoHeader.Height; i++) {
		pixel_arrayp[i] = (struct Pixels *)calloc(InfoHeader.Width, size_spix);
	}

	for (i = 0; i < InfoHeader.Height; i++) {
		for (j = 0; j < InfoHeader.Width; j++) {


			fread(&pixel_arrayp[i][j], 3, 1, BMP_in);
			for (int i1 = 0;i1 < 3;i1++)
				for (int j1 = 0;j1 < 3;j1++){
					RGB_image[i][j][0] = pixel_arrayp[i][j].Red;
					RGB_image[i][j][1] = pixel_arrayp[i][j].Green;
					RGB_image[1][j][2] = pixel_arrayp[i][j].Blue;
				}

		}
		if (padding != 0) {
			fread(&temp, padding, 1, BMP_in);
		}
	}



	fclose(BMP_in);
}


int main(void) {
	short ZigZag_Y[64] = { 0 };
	short ZigZag_U[64] = { 0 };
	short ZigZag_V[64] = { 0 };
	char writecode[100000];
	writecode[0] = '\0';

	read_input1();
	N = InfoHeader.Height;
	M = InfoHeader.Width;


	generate_YUV_image();
	int prev_DC_Y = 0;
	int prev_DC_U = 0;
	int prev_DC_V = 0;
	char code[1000];


	FILE *BMP_out = fopen("cross_izlaz.bmp", "wb");

	if (BMP_out == NULL) {
      printf("\nCannot open file\n");
      exit(1);
   	}

	for (int i = 0; i < N; i += 8)
		for (int j = 0; j < M; j += 8) {
			//printf("Block (%d, %d):\n", i / 8, j / 8);
			shift_values(i, j);
			perform_DCT(i, j);
			quantize(i, j);
			prev_DC_Y = DCT_image[0][0][0];
			prev_DC_U = DCT_image[0][0][1];
			prev_DC_V = DCT_image[0][0][2];
			zig_zag(i, j, ZigZag_Y, 0);
			zig_zag(i, j, ZigZag_U, 1);
			zig_zag(i, j, ZigZag_V, 2);	
			short RL[64] = {0};
			int rl = 0;
			rl = RLE(ZigZag_Y, RL);
			prev_DC_Y = Encode(RL, rl, code, prev_DC_Y);
			rl = RLE(ZigZag_U, RL);
			prev_DC_U = Encode(RL, rl, code, prev_DC_U);
			rl = RLE(ZigZag_V, RL);
			prev_DC_V = Encode(RL, rl, code, prev_DC_V);
			strcat(writecode, code);
		}	

	fwrite(&Header, sizeof(Header), 1,  BMP_out);
    fwrite(&InfoHeader, sizeof(InfoHeader), 1, BMP_out);	

    fwrite(&writecode, sizeof(writecode), 1, BMP_out);

    fclose(BMP_out);

	return 0;
}

void generate_YUV_image() {
	float temp;
	for (int i = 0; i < N; ++i)
		for (int j = 0; j < M; ++j)
			for (int k = 0; k < 3; ++k) {
				temp = RGB_to_YUV_factors[k][3];

				for (int l = 0; l < 3; ++l)
					temp += RGB_image[i][j][l] * RGB_to_YUV_factors[k][l];

				YUV_image[i][j][k] = temp;
			}
}

void perform_DCT(int x, int y) {
	float temp[8][8];
	for (int l = 0; l < 3; ++l) {
		memset(temp, 0, sizeof(temp));
		for (int i = 0; i < 8; ++i)
			for (int j = 0; j < 8; ++j)
				for (int k = 0; k < 8; ++k)
					temp[i][j] += DCT_coefficients[i][k] * YUV_image[x + k][y + j][l];
		for (int i = 0; i < 8; ++i)
			for (int j = 0; j < 8; ++j)
				for (int k = 0; k < 8; ++k)
					DCT_image[x + i][y + j][l] += temp[i][k] * DCT_coefficients_transp[k][j];
	}
}

void read_input() {
	scanf("%d%d", &N, &M);
	for (int i = 0; i < N; ++i)
		for (int j = 0; j < M; ++j)
			for (int k = 0; k < 3; ++k)
				scanf("%d", &RGB_image[i][j][k]);
}



