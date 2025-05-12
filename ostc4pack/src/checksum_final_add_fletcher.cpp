#include <iostream>
#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void fletcher16(unsigned char *result1, unsigned char *result2, unsigned char const *data, size_t bytes )
{
        unsigned short sum1 = 0xff, sum2 = 0xff;
        size_t tlen;
 
        while (bytes) {
                tlen = bytes >= 20 ? 20 : bytes;
                bytes -= tlen;
                do {
                        sum2 += sum1 += *data++;
                } while (--tlen);
                sum1 = (sum1 & 0xff) + (sum1 >> 8);
                sum2 = (sum2 & 0xff) + (sum2 >> 8);
        }
        /* Second reduction step to reduce sums to 8 bits */
        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
        *result2 = (sum2 & 0xff);
        *result1 = (sum1 & 0xff);
//        return sum2 << 8 | sum1;
}


// This is a variant of fletcher16 with a 16 bit sum instead of an 8 bit sum,
// and modulo 2^16 instead of 2^16-1
void
hw_ostc3_firmware_checksum (unsigned char *result1, unsigned char *result2, unsigned char *result3, unsigned char *result4, unsigned char const *data, size_t bytes )
{
	unsigned short low = 0;
	unsigned short high = 0;
	for (unsigned int i = 0; i < bytes; i++) {
		low  += data[i];
		high += low;
	}
        *result1 = (low & 0xff);
        *result2 = (low/256 & 0xff);
        *result3 = (unsigned char)(high & 0xff);
        *result4 = (unsigned char)((high/256) & 0xff);
//	return (((unsigned int)high) << 16) + low;
}


int main(int argc, char** argv)
{
	unsigned index = 1;
	bool useDate = false;
	bool useType = false;
	while (index < argc && strncmp(argv[index], "-", 1) == 0) {
		if (strcmp(argv[index], "--date") == 0) {
			useDate = true;
		} else if (strcmp(argv[index], "--type") == 0) {
			useType = true;
		} else {
			printf("Unknown option: %s\n", argv[index]);

			return -1;
		}

		index++;
	}
	
	char *file;
	char *file2;
	char *file3;
	switch (argc - index) {
	case 3:
		file3 = argv[index + 2];
	case 2:
		file2 = argv[index + 1];
	case 1:
		file = argv[index];

		break;
	default:
		printf("Invalid number of arguments. Usage: checksum_final_add_fletcher <file1> [(<file2>|null) [<file3>]]\n");

		return -1;
	}

	FILE *fp, * fpout;
	size_t lenTotal,lenTemp;
	unsigned char buf[2000000];
	unsigned int pruefsumme;
	unsigned char buf2[4];

   	printf("1: %s\n",  file);
   	printf("2: %s\n",  file2);
   	printf("3: %s\n",  file3);
   	printf("\n");
     
	
		//write File with length and cheksum
	char filename[500], filenameout[510] ;
	sprintf(filename,"%s",file);
	int filelength = strlen(filename);
	filename[filelength -4] = 0;
	
	lenTotal = 0;
	if (NULL == (fp = fopen(file, "rb")))
	{
	    printf("Unable to open %s for reading\n", file);
	    return -1;
	}
	lenTemp = fread(&buf[lenTotal], sizeof(char), sizeof(buf), fp);
//	lenTemp = fread(buf, sizeof(char), sizeof(buf), fp);
	lenTotal = lenTemp;
	printf("%d bytes read (hex: %#x )\n", (uint32_t)lenTemp, (uint32_t)lenTemp);
	fclose(fp);

	unsigned typeFlags = 0x00;
	if(file2 && strcmp(file2, "null") != 0)
	{
		if (NULL == (fp = fopen(file2, "rb")))
		{
		    printf("Unable to open %s for reading\n", file2);
		    return -1;
		}
		lenTemp = fread(&buf[lenTotal], sizeof(char), sizeof(buf)-lenTotal, fp);
		lenTotal += lenTemp;
		printf("%d bytes read (hex: %#x )\n", (uint32_t)lenTemp, (uint32_t)lenTemp);
		fclose(fp);

		typeFlags |= 0x01;
	}
	if(file3)
	{
		if (NULL == (fp = fopen(file3, "rb")))
		{
		    printf("Unable to open %s for reading\n", file3);
		    return -1;
		}
		lenTemp = fread(&buf[lenTotal], sizeof(char), sizeof(buf)-lenTotal, fp);
		lenTotal += lenTemp;
		printf("%d bytes read (hex: %#x )\n", (uint32_t)lenTemp, (uint32_t)lenTemp);
		fclose(fp);

		typeFlags |= 0x02;
	}

   	printf("\n");
	printf("%d bytes read (hex: %#x ) total \n", (uint32_t)lenTotal, (uint32_t)lenTotal);

	time_t rawtime;
	time (&rawtime);
	struct tm *timeinfo;
	timeinfo = localtime(&rawtime);
	
//	sprintf(filenameout,"fwupdate_%s.bin",ctime(&rawtime));

	char type[10] = "";
	if (useType) {
		switch (typeFlags) {
		case 0x00:
			sprintf(type, "_firmware");

			break;
		case 0x01:
			sprintf(type, "_fontpack");

			break;
		case 0x02:
			sprintf(type, "_rte");

			break;
		case 0x03:
			sprintf(type, "_all");

			break;
		}
	}

	char date[8] = "";
	if (useDate) {
		sprintf(date, "_%02u%02u%02u", timeinfo->tm_year-100, timeinfo->tm_mon + 1, timeinfo->tm_mday);
	}

	sprintf(filenameout, "OSTC4update%s%s.bin", type, date);

    fpout = fopen(filenameout, "wb");     
    for(int i = 0;i <lenTotal;i++)
    {
    	if(fwrite(&buf[i],1,1,fpout) != 1)
     	printf("error writing\n");
	}
	
	hw_ostc3_firmware_checksum(&buf2[0],&buf2[1],&buf2[2],&buf2[3],buf,lenTotal);
	printf("checksum  %#x %#x %#x %#x\n", buf2[0],buf2[1], buf2[2], buf2[3]);
   	if(fwrite(&buf2[0],1,4,fpout) != 4)
    	printf("error writing checksum\n");
    ;
	fclose(fpout);
}
