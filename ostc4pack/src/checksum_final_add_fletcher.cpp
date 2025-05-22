#include <iostream>
#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(INCLUDE_VERSION_SUPPORT)

#include "settings.h"

extern "C" {
	void tHome_init(void) { }
	void tI_init(void) { }
	void tM_init(void) { }
	void tMenuEdit_init(void) { }
	void tInfoLog_init(void) { }
	void tM_build_pages(void) { }
	void GFX_build_logo_frame(void) { }
	void setButtonResponsiveness(void) { }
	void GFX_use_colorscheme(void) { }
	void GFX_build_hw_background_frame(void) { }
}
#endif

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
	int index = 1;
	bool useDate = false;
	bool useType = false;
	bool useVersion = false;
	bool printVersionOnly = false;
	while (index < argc && strncmp(argv[index], "-", 1) == 0) {
		if (strcmp(argv[index], "--date") == 0) {
			useDate = true;
		} else if (strcmp(argv[index], "--type") == 0) {
			useType = true;
#if defined(INCLUDE_VERSION_SUPPORT)
		} else if (strcmp(argv[index], "--version") == 0) {
			useVersion = true;
#endif
		} else if (strcmp(argv[index], "--print-version-only") == 0) {
			printVersionOnly = true;
		} else {
			printf("Unknown option: %s\n", argv[index]);

			return -1;
		}

		index++;
	}
	
	char *file;
	char *file2 = NULL;
	char *file3 = NULL;
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

	char versionName[26] = "";
	index = 0;
#if defined(INCLUDE_VERSION_SUPPORT)
	if (useVersion) {
		if (!printVersionOnly) {
			index += sprintf(&versionName[index], "_");
		}
		index += sprintf(&versionName[index], "%u.%u.%u%s", firmware_FirmwareData.versionFirst, firmware_FirmwareData.versionSecond, firmware_FirmwareData.versionThird, firmware_FirmwareData.versionBeta ? "-beta" : "");
	}
#endif

	if (useDate) {
		time_t rawtime;
		time (&rawtime);
		struct tm *timeinfo;
		timeinfo = localtime(&rawtime);

		if (useVersion || !printVersionOnly) {
			index += sprintf(&versionName[index], "_");
		}
		index += sprintf(&versionName[index], "%02u%02u%02u", timeinfo->tm_year-100, timeinfo->tm_mon + 1, timeinfo->tm_mday);
	}

	if (printVersionOnly) {
		printf("%s\n", versionName);

		return 0;
	}

	FILE *fp, *fpout;
	unsigned char buf[2000000];
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
	
	size_t lenTotal = 0;
	if (NULL == (fp = fopen(file, "rb")))
	{
	    printf("Unable to open %s for reading\n", file);
	    return -1;
	}
	size_t lenTemp = fread(&buf[lenTotal], sizeof(char), sizeof(buf), fp);
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

	sprintf(filenameout, "OSTC4update%s%s.bin", type, versionName);

    fpout = fopen(filenameout, "wb");     
    for (size_t i = 0; i < lenTotal; i++)
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
