#include <iostream>

/* run this program using the console pauser or add your own getch, system("pause") or input loop */




#include <stdlib.h>

/******************************************************************************/
/*                             Start of crcmodel.h                            */
/******************************************************************************/
/*                                                                            */
/* Author : Ross Williams (ross@guest.adelaide.edu.au.).                      */
/* Date   : 3 June 1993.                                                      */
/* Status : Public domain.                                                    */
/*                                                                            */
/* Description : This is the header (.h) file for the reference               */
/* implementation of the Rocksoft^tm Model CRC Algorithm. For more            */
/* information on the Rocksoft^tm Model CRC Algorithm, see the document       */
/* titled "A Painless Guide to CRC Error Detection Algorithms" by Ross        */
/* Williams (ross@guest.adelaide.edu.au.). This document is likely to be in   */
/* "ftp.adelaide.edu.au/pub/rocksoft".                                        */
/*                                                                            */
/* Note: Rocksoft is a trademark of Rocksoft Pty Ltd, Adelaide, Australia.    */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* How to Use This Package                                                    */
/* -----------------------                                                    */
/* Step 1: Declare a variable of type cm_t. Declare another variable          */
/*         (p_cm say) of type p_cm_t and initialize it to point to the first  */
/*         variable (e.g. p_cm_t p_cm = &cm_t).                               */
/*                                                                            */
/* Step 2: Assign values to the parameter fields of the structure.            */
/*         If you don't know what to assign, see the document cited earlier.  */
/*         For example:                                                       */
/*            p_cm->cm_width = 16;                                            */
/*            p_cm->cm_poly  = 0x8005L;                                       */
/*            p_cm->cm_init  = 0L;                                            */
/*            p_cm->cm_refin = TRUE;                                          */
/*            p_cm->cm_refot = TRUE;                                          */
/*            p_cm->cm_xorot = 0L;                                            */
/*         Note: Poly is specified without its top bit (18005 becomes 8005).  */
/*         Note: Width is one bit less than the raw poly width.               */
/*                                                                            */
/* Step 3: Initialize the instance with a call cm_ini(p_cm);                  */
/*                                                                            */
/* Step 4: Process zero or more message bytes by placing zero or more         */
/*         successive calls to cm_nxt. Example: cm_nxt(p_cm,ch);              */
/*                                                                            */
/* Step 5: Extract the CRC value at any time by calling crc = cm_crc(p_cm);   */
/*         If the CRC is a 16-bit value, it will be in the bottom 16 bits.    */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Design Notes                                                               */
/* ------------                                                               */
/* PORTABILITY: This package has been coded very conservatively so that       */
/* it will run on as many machines as possible. For example, all external     */
/* identifiers have been restricted to 6 characters and all internal ones to  */
/* 8 characters. The prefix cm (for Crc Model) is used as an attempt to avoid */
/* namespace collisions. This package is endian independent.                  */
/*                                                                            */
/* EFFICIENCY: This package (and its interface) is not designed for           */
/* speed. The purpose of this package is to act as a well-defined reference   */
/* model for the specification of CRC algorithms. If you want speed, cook up  */
/* a specific table-driven implementation as described in the document cited  */
/* above. This package is designed for validation only; if you have found or  */
/* implemented a CRC algorithm and wish to describe it as a set of parameters */
/* to the Rocksoft^tm Model CRC Algorithm, your CRC algorithm implementation  */
/* should behave identically to this package under those parameters.          */
/*                                                                            */
/******************************************************************************/

/* The following #ifndef encloses this entire */
/* header file, rendering it indempotent.     */
#ifndef CM_DONE
#define CM_DONE

/******************************************************************************/

/* The following definitions are extracted from my style header file which    */
/* would be cumbersome to distribute with this package. The DONE_STYLE is the */
/* idempotence symbol used in my style header file.                           */

#ifndef DONE_STYLE

typedef unsigned long   ulong;
typedef unsigned char * p_ubyte_;

#ifndef TRUE
#define FALSE 0
#define TRUE  1
#endif

/* Change to the second definition if you don't have prototypes. */
#define P_(A) A
/* #define P_(A) () */

/* Uncomment this definition if you don't have void. */
/* typedef int void; */

#endif

/******************************************************************************/

/* CRC Model Abstract Type */
/* ----------------------- */
/* The following type stores the context of an executing instance of the  */
/* model algorithm. Most of the fields are model parameters which must be */
/* set before the first initializing call to cm_ini.                      */
typedef struct
  {
   int   cm_width;   /* Parameter: Width in bits [8,32].       */
   ulong cm_poly;    /* Parameter: The algorithm's polynomial. */
   ulong cm_init;    /* Parameter: Initial register value.     */
   bool  cm_refin;   /* Parameter: Reflect input bytes?        */
   bool  cm_refot;   /* Parameter: Reflect output CRC?         */
   ulong cm_xorot;   /* Parameter: XOR this to output CRC.     */

   ulong cm_reg;     /* Context: Context during execution.     */
  } cm_t;
typedef cm_t *p_cm_t;

/******************************************************************************/

/* Functions That Implement The Model */
/* ---------------------------------- */
/* The following functions animate the cm_t abstraction. */

void cm_ini P_((p_cm_t p_cm));
/* Initializes the argument CRC model instance.          */
/* All parameter fields must be set before calling this. */

void cm_nxt P_((p_cm_t p_cm,int ch));
/* Processes a single message byte [0,255]. */

void cm_blk P_((p_cm_t p_cm,p_ubyte_ blk_adr,ulong blk_len));
/* Processes a block of message bytes. */

ulong cm_crc P_((p_cm_t p_cm));
/* Returns the CRC value for the message bytes processed so far. */

/******************************************************************************/

/* Functions For Table Calculation */
/* ------------------------------- */
/* The following function can be used to calculate a CRC lookup table.        */
/* It can also be used at run-time to create or check static tables.          */

ulong cm_tab P_((p_cm_t p_cm,int index));
/* Returns the i'th entry for the lookup table for the specified algorithm.   */
/* The function examines the fields cm_width, cm_poly, cm_refin, and the      */
/* argument table index in the range [0,255] and returns the table entry in   */
/* the bottom cm_width bytes of the return value.                             */

/******************************************************************************/

/* End of the header file idempotence #ifndef */
#endif

/******************************************************************************/
/*                             End of crcmodel.h                              */
/******************************************************************************/


/******************************************************************************/
/*                             Start of crcmodel.c                            */
/******************************************************************************/
/*                                                                            */
/* Author : Ross Williams (ross@guest.adelaide.edu.au.).                      */
/* Date   : 3 June 1993.                                                      */
/* Status : Public domain.                                                    */
/*                                                                            */
/* Description : This is the implementation (.c) file for the reference       */
/* implementation of the Rocksoft^tm Model CRC Algorithm. For more            */
/* information on the Rocksoft^tm Model CRC Algorithm, see the document       */
/* titled "A Painless Guide to CRC Error Detection Algorithms" by Ross        */
/* Williams (ross@guest.adelaide.edu.au.). This document is likely to be in   */
/* "ftp.adelaide.edu.au/pub/rocksoft".                                        */
/*                                                                            */
/* Note: Rocksoft is a trademark of Rocksoft Pty Ltd, Adelaide, Australia.    */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Implementation Notes                                                       */
/* --------------------                                                       */
/* To avoid inconsistencies, the specification of each function is not echoed */
/* here. See the header file for a description of these functions.            */
/* This package is light on checking because I want to keep it short and      */
/* simple and portable (i.e. it would be too messy to distribute my entire    */
/* C culture (e.g. assertions package) with this package.                     */
/*                                                                            */
/******************************************************************************/

#include <cstdint>
#include "crcmodel.h"

/******************************************************************************/

/* The following definitions make the code more readable. */

#define BITMASK(X) (1L << (X))
#define MASK32 0xFFFFFFFFL
#define LOCAL static

/******************************************************************************/

ulong reflect P_((ulong v,int b))
{
 int   i;
 ulong t = v;
 for (i=0; i<b; i++)
   {
    if (t & 1L)
       v|=  BITMASK((b-1)-i);
    else
       v&= ~BITMASK((b-1)-i);
    t>>=1;
   }
 return v;
}

/******************************************************************************/

LOCAL ulong widmask (p_cm_t p_cm)
{
 return (((1L<<(p_cm->cm_width-1))-1L)<<1)|1L;
}

/******************************************************************************/

void cm_ini (p_cm_t p_cm)
{
 p_cm->cm_reg = p_cm->cm_init;
}

/******************************************************************************/

void cm_nxt (p_cm_t p_cm,int ch)
{
 int   i;
 ulong uch  = (ulong) ch;
 ulong topbit = BITMASK(p_cm->cm_width-1);

 if (p_cm->cm_refin) uch = reflect(uch,8);
 p_cm->cm_reg ^= (uch << (p_cm->cm_width-8));
 for (i=0; i<8; i++)
   {
    if (p_cm->cm_reg & topbit)
       p_cm->cm_reg = (p_cm->cm_reg << 1) ^ p_cm->cm_poly;
    else
       p_cm->cm_reg <<= 1;
    p_cm->cm_reg &= widmask(p_cm);
   }
}

/******************************************************************************/

void cm_blk (p_cm_t p_cm,p_ubyte_ blk_adr,ulong blk_len)
{
 while (blk_len--) cm_nxt(p_cm,*blk_adr++);
}

/******************************************************************************/

ulong cm_crc (p_cm_t p_cm)
{
 if (p_cm->cm_refot)
    return p_cm->cm_xorot ^ reflect(p_cm->cm_reg,p_cm->cm_width);
 else
    return p_cm->cm_xorot ^ p_cm->cm_reg;
}

/******************************************************************************/

ulong cm_tab (p_cm_t p_cm,int index)
{
 int   i;
 ulong r;
 ulong topbit = BITMASK(p_cm->cm_width-1);
 ulong inbyte = (ulong) index;

 if (p_cm->cm_refin) inbyte = reflect(inbyte,8);
 r = inbyte << (p_cm->cm_width-8);
 for (i=0; i<8; i++)
    if (r & topbit)
       r = (r << 1) ^ p_cm->cm_poly;
    else
       r<<=1;
 if (p_cm->cm_refin) r = reflect(r,p_cm->cm_width);
 return r & widmask(p_cm);
}

/******************************************************************************/
/*                             End of crcmodel.c                              */
/******************************************************************************/

uint32_t	CRC_CalcBlockCRC(uint32_t *buffer, uint32_t words)
 {
 cm_t        crc_model;
 uint32_t      word_to_do;
 uint8_t       byte_to_do;
 int         i;
 
     // Values for the STM32F generator.
 
     crc_model.cm_width = 32;            // 32-bit CRC
     crc_model.cm_poly  = 0x04C11DB7;    // CRC-32 polynomial
     crc_model.cm_init  = 0xFFFFFFFF;    // CRC initialized to 1's
     crc_model.cm_refin = FALSE;         // CRC calculated MSB first
     crc_model.cm_refot = FALSE;         // Final result is not bit-reversed
     crc_model.cm_xorot = 0x00000000;    // Final result XOR'ed with this
 
     cm_ini(&crc_model);
 
     while (words--)
     {
         // The STM32F10x hardware does 32-bit words at a time!!!
 
         word_to_do = *buffer++;
 
         // Do all bytes in the 32-bit word.
 
         for (i = 0; i < sizeof(word_to_do); i++)
         {
             // We calculate a *byte* at a time. If the CRC is MSB first we
             // do the next MS byte and vica-versa.
 
             if (crc_model.cm_refin == FALSE)
             {
                 // MSB first. Do the next MS byte.
 
                 byte_to_do = (uint8_t) ((word_to_do & 0xFF000000) >> 24);
                 word_to_do <<= 8;
             }
             else
             {
                 // LSB first. Do the next LS byte.
 
                 byte_to_do = (uint8_t) (word_to_do & 0x000000FF);
                 word_to_do >>= 8;
             }
 
             cm_nxt(&crc_model, byte_to_do);
         }
     }
 
     // Return the final result.
 
     return (cm_crc(&crc_model));
 }
 

unsigned checksum(void *buffer, size_t len, unsigned int seed)
{
      unsigned char *buf = (unsigned char *)buffer;
      size_t i;

      for (i = 0; i < len; ++i)
            seed += (unsigned int)(*buf++);
      return seed;
}

unsigned int crc32c_checksum(unsigned char* message, int length) {
   int i, j;
   unsigned int byte, crc, mask;
   static unsigned int table[256] = {0};

   /* Set up the table, if necessary. */

   if (table[1] == 0) {
      for (byte = 0; byte <= 255; byte++) {
         crc = byte;
         for (j = 7; j >= 0; j--) {    // Do eight times.
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
         }
         table[byte] = crc;
      }
   }

   /* Through with table setup, now calculate the CRC. */
   i = 0;
   crc = 0xFFFFFFFF;
   while (length--) {
      byte = message[i];
      crc = (crc >> 8) ^ table[(crc ^ byte) & 0xFF];
      i = i + 1;
   }
   return ~crc;
}

uint32_t CalcFletcher32(const uint32_t *startAddr, const uint32_t *endAddr)
{
    const uint16_t *ptr = (const uint16_t *)startAddr;
    const uint16_t *end = (const uint16_t *)endAddr;

    uint16_t sum1 = 0;
    uint16_t sum2 = 0;

    while (ptr <= end) {
        sum1 = sum1 + *ptr++;
        sum2 = sum2 + sum1;
    }

    return ((uint32_t)sum2 << 16) | sum1;
}






/******************************************************************************/

/* MAIN */
/* ----------------------- */
/*   */


#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
	
	
	if (argc != 3) {
		fprintf(stderr, "Usage: OSTC4pack_V4 <type> <bin file>\n");
		return(-1);
	}

	FILE *fp;
	size_t len;
	unsigned char buf[1050000];
	char *file = argv[2];
	int type =  atoi(argv[1]);
	unsigned int pruefsumme;
	
		//write File with length and cheksum
	char filename[500], filenameout[511];
	sprintf(filename,"%s",file);
	int filelength = strlen(filename);
	filename[filelength -4] = 0;
	
	if (NULL == (fp = fopen(file, "rb")))
	{
	    printf("Unable to open %s for reading\n", file);
	    return -1;
	}
	len = fread(buf, sizeof(char), sizeof(buf), fp);
	printf("%d bytes read (hex: %#x )\n", (uint32_t)len, (uint32_t)len);

        if (type == 3) {
		if (len > 0x01fff0) {
			printf("Error: File too large for type 2.\n");
			return -1;
		}
        unsigned int binaryLength = len;
        memset(buf + len, 0xFF, 0x01fff0 - len);
len = 0x01fff0;
		len = 0x01fff0;
		unsigned int internalChecksum = CalcFletcher32((uint32_t *)buf, (uint32_t *)&buf[len - 1]);
		printf("Internal checksum for type 3: %#x\n", internalChecksum);

		buf[len++] = 0x00; // reserved
		buf[len++] = 0x00;
		buf[len++] = 0x00;
		buf[len++] = 0x00;
		buf[len++] = 0x00; // type == bootloader
		buf[len++] = 0x00;
		buf[len++] = 0x01;
		buf[len++] = 0x00;
     		buf[len++] = 0xFF & (internalChecksum >> 24);
     		buf[len++] = 0xFF & (internalChecksum >> 16);
     		buf[len++] = 0xFF & (internalChecksum >> 8);
     		buf[len++] = 0xFF & internalChecksum;
     		buf[len++] = 0xFF & (binaryLength >> 24);
     		buf[len++] = 0xFF & (binaryLength >> 16);
     		buf[len++] = 0xFF & (binaryLength >> 8);
     		buf[len++] = 0xFF & binaryLength;
	}
	printf("The length is %#x\n", (unsigned int)len);

//	unsigned int checksum = crc32c_checksum(buf, len);
	unsigned int checksum = CRC_CalcBlockCRC((uint32_t *)buf, (uint32_t)(len/4));
	printf("The checksum is %#x\n", checksum);
	
	fclose(fp);
	if(type == 0)
		sprintf(filenameout,"%s_upload.bin",filename);
	else
	if(type == 2)
		sprintf(filenameout,"%s_upload.bin",filename);
	else
		sprintf(filenameout,"%s_upload.bin",filename);
	
	unsigned char buf2[4];
     
     buf2[0] = 0xFF & (len >> 24);;
     buf2[1] = 0xFF & (len >> 16);;
     buf2[2] = 0xFF & (len >> 8);
     buf2[3] = 0xFF & len;
     fp = fopen(filenameout, "wb");
    fwrite(buf2,sizeof(char),4,fp);


	unsigned char buf3offset[4];
	unsigned char bufVersion[4];
	if(type == 2)
	{
		buf3offset[0] = 0x10;
		buf3offset[1] = 0x00;
		buf3offset[2] = 0x03;
		buf3offset[3] = 0x20;
		bufVersion[0] = buf[0x00];
		bufVersion[1] = buf[0x01];
		bufVersion[2] = buf[0x02];
		bufVersion[3] = buf[0x03];
	}
	else
	if(type == 3)
	{
		buf3offset[0] = 0x10;
		buf3offset[1] = 0x00;
		buf3offset[2] = 0x00;
		buf3offset[3] = 0x00;
		bufVersion[0] = buf[0x00];
		bufVersion[1] = buf[0x01];
		bufVersion[2] = buf[0x02];
		bufVersion[3] = buf[0x03];
	}
	else
	if(type == 0)
	{
		buf3offset[0] = 0xFE;
		buf3offset[1] = 'R';
		buf3offset[2] = 'T';
		buf3offset[3] = 'E';
		bufVersion[0] = buf[0x5000];
		bufVersion[1] = buf[0x5001];
		bufVersion[2] = buf[0x5002];
		bufVersion[3] = buf[0x5003];
	}
	else
	{
		buf3offset[0] = 0xFF;
		buf3offset[1] = 0;
		buf3offset[2] = 'H';
		buf3offset[3] = 'W';
		bufVersion[0] = buf[0x10000];
		bufVersion[1] = buf[0x10001];
		bufVersion[2] = buf[0x10002];
		bufVersion[3] = buf[0x10003];
	}
	
    if (type == 2 || type == 3) {
    	printf("The offset is %#x\n", ((256 * buf3offset[1] + buf3offset[2]) * 256 + buf3offset[3]) * 256);
    }

    fwrite(buf3offset,sizeof(char),4,fp);
    
    pruefsumme = len + (256*256*256*buf3offset[0]) + (256*256*buf3offset[1]) + (256*buf3offset[2]) + buf3offset[3];
    fwrite(&pruefsumme,sizeof(char),4,fp);

    fwrite(bufVersion,sizeof(char),4,fp);

    for(int i = 0;i <len;i++)
    {
    	if(fwrite(&buf[i],1,1,fp) != 1)
     	printf("error writing\n");
	}
     
     
     buf2[0] = 0xFF & (checksum >> 24);// & 0xFF000000;
     buf2[1] =  0xFF & (checksum >> 16);
     buf2[2] = 	0xFF & (checksum >> 8);
     buf2[3] = 0xFF & checksum;
     
    fwrite(buf2,sizeof(char),4,fp);

     fclose(fp);
}
