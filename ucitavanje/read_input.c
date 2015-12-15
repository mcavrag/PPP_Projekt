#include <stdio.h>
#include <stdlib.h>
#pragma pack(1)

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
   unsigned char Blue,Green,Red;
};

void read_input(void) {

   int i=0, j=0;
   int size_spix;
   int padding = 0;
   char temp[4];
   struct Pixels **pixel_arrayp;

   FILE *BMP_out = fopen("cross_izlaz.bmp", "wb");
   FILE *BMP_in = fopen ("test.bmp", "rb");
   if (BMP_in == NULL){

        printf ("Soap test.bmp ne moze se otvoriti");
        exit (1);
        }


   fread(&Header.Type, sizeof(Header.Type), 1, BMP_in);
   fread(&Header.Size, sizeof(Header.Size), 1, BMP_in);
   fread(&Header.Reserved1, sizeof(Header.Reserved1), 1, BMP_in);
   fread(&Header.Reserved2, sizeof(Header.Reserved2), 1, BMP_in);
   fread(&Header.Offset, sizeof(Header.Offset), 1, BMP_in);
   fread(&InfoHeader, sizeof(InfoHeader), 1, BMP_in);


   padding = InfoHeader.Width % 4;
   if(padding != 0 ) {
      padding = 4 - padding;
   }

   size_spix = sizeof(struct Pixels);


   pixel_arrayp = (struct Pixels **)calloc(InfoHeader.Height,sizeof(struct Pixel*));


   for(i=0;i<InfoHeader.Height; i++) {
      pixel_arrayp[i] = (struct Pixels *)calloc(InfoHeader.Width,size_spix);
   }

   for(i=0; i < InfoHeader.Height; i++) {
      for(j=0; j < InfoHeader.Width; j++) {


         fread(&pixel_arrayp[i][j], 3,1,  BMP_in);

      }
      if(padding != 0) {
         fread(&temp, padding, 1,  BMP_in);
      }
   }


   fclose(BMP_in);
}
