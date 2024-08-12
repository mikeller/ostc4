/**
  ******************************************************************************
  * @file    gfx_engine.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.2
  * @date    30-April-2014
  * @brief   Main source file of GFX Graphic Engine
  *          This file provides firmware functions to manage the following
  *          functions to draw on the screen:
  *           + write string to display
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#include "gfx.h"
#include "gfx_engine.h"
#include "gfx_fonts.h"
#include "gfx_colors.h"
#include "ostc.h"
#include "settings.h"
#include "text_multilanguage.h"

/* Exported variables --------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

#define RING_BUF_SIZE			(5u)
#define MAX_COLOR_STRING_LENGTH	(100u)

typedef struct
{
	uint32_t Xdelta;
	uint32_t Ydelta;
	uint8_t	 invert;
	uint8_t  color;
	uint8_t  dualFont;
	uint8_t  resize;
	uint32_t font;
	uint8_t  spaceMode;
	uint8_t  singleSpaceWithSizeOfNextChar;
	uint8_t	useTinyFont;
	uint32_t TinyFont;
	int8_t TinyFontExtraYdelta;
	tFont *actualFont;
	uint8_t doubleSize;
} GFX_CfgWriteString;

typedef struct
{
	uint32_t  pBuffer;
	uint32_t  height;
	uint32_t  width;
	uint32_t 	leftStart;
	uint32_t 	bottomStart;
} GFX_layerSingle;
/*
typedef struct
{
	GFX_layerSingle top;
	GFX_layerSingle bottom;
} GFX_layersTopBottom;
*/
typedef struct
{
	uint32_t pActualTopBuffer;
	uint32_t pNextTopBuffer[RING_BUF_SIZE];
	GFX_layerSingle actualBottom;
	GFX_layerSingle nextBottom[RING_BUF_SIZE];
	uint8_t NextTopWrite;
	uint8_t NextBottomWrite;
	uint8_t NextTopRead;
	uint8_t NextBottomRead;
} GFX_layerControl;

typedef struct
{
	uint32_t 	StartAddress;
	int8_t 		status;
	uint8_t		caller;
} SFrameList;

enum FRAMESTATE
{
	CLEAR = 0,
	BLOCKED,
	RELEASED
};

enum LOGOSTATE
{
	LOGOOFF = 0,
	LOGOSTART = 1,
	LOGOSTOP = 255
};

// should be 43
#define MAXFRAMES 39

#define SDRAM_BANK_ADDR    	((uint32_t)0xD0000000)
#define	FBGlobalStart				SDRAM_BANK_ADDR
#define	FBOffsetEachIndex		(800*480*2)

#define SDRAM_DOUBLE_BUFFER_ONE ((uint32_t)(FBGlobalStart + (MAXFRAMES * FBOffsetEachIndex)))
#define SDRAM_DOUBLE_BUFFER_TWO ((uint32_t)(SDRAM_DOUBLE_BUFFER_ONE + (2 * FBOffsetEachIndex)))
#define SDRAM_DOUBLE_BUFFER_END ((uint32_t)(SDRAM_DOUBLE_BUFFER_TWO + (2 * FBOffsetEachIndex)))

/* Semi Private variables ---------------------------------------------------------*/

DMA2D_HandleTypeDef	Dma2dHandle;
static LTDC_HandleTypeDef	LtdcHandle;

/* Private variables ---------------------------------------------------------*/

static uint8_t DMA2D_at_work = 0;

static GFX_layerControl FrameHandler = { 0 };

static uint32_t pInvisibleFrame = 0;
static uint32_t pLogoFrame = 0;
static uint8_t logoStatus;
static uint32_t pBackgroundHwFrame = 0;
static uint8_t backgroundHwStatus;

static SFrameList frame[MAXFRAMES];

static void GFX_clear_frame_immediately(uint32_t pDestination);
static void GFX_draw_image_color(GFX_DrawCfgScreen *hgfx, SWindowGimpStyle window, const tImage *image);
/* ITM Trace-----------------------------------------------------------------*/

#include "stdio.h"

#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000

struct __FILE { int handle; /* Add whatever needed */ };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f) {
  if (DEMCR & TRCENA) {
    while (ITM_Port32(0) == 0);
    ITM_Port8(0) = ch;
  }
  return(ch);
}

uint32_t MinU32GFX(uint32_t a, uint32_t b)
{
	return ((a<b)?a:b);
}


uint32_t MaxU32GFX(uint32_t a, uint32_t b)
{
	return((a>b)?a:b);
}

/* Private function prototypes -----------------------------------------------*/

static uint32_t GFX_write_char(GFX_DrawCfgWindow* hgfx, GFX_CfgWriteString* cfg, uint8_t character, tFont *Font);
static uint32_t GFX_write_substring(GFX_CfgWriteString* cfg, GFX_DrawCfgWindow* hgfx, uint8_t textId, int8_t nextCharFor2Byte);
static uint32_t GFX_write__Modify_Xdelta__Centered(GFX_CfgWriteString* cfg, GFX_DrawCfgWindow* hgfx, const char *pText);
static uint32_t GFX_write__Modify_Xdelta__RightAlign(GFX_CfgWriteString* cfg, GFX_DrawCfgWindow* hgfx, const char *pTextInput);
static void GFX_Error_Handler(void);
static void GFX_Dma2d_TransferComplete(DMA2D_HandleTypeDef* Dma2dHandle);
static void GFX_Dma2d_TransferError(DMA2D_HandleTypeDef* Dma2dHandle);
static void  GFX_clear_frame_dma2d(uint8_t frameId);

static uint32_t GFX_doubleBufferOne(void);
static uint32_t GFX_doubleBufferTwo(void);


/* Exported functions --------------------------------------------------------*/

uint8_t GFX_logoStatus(void)
{
	return logoStatus;
}

void GFX_SetWindowLayer0(uint32_t pDestination, int16_t XleftGimpStyle, int16_t XrightGimpStyle, int16_t YtopGimpStyle, int16_t YbottomGimpStyle)
{
	int16_t XSize, YSize, X0, Y0;

	if(XleftGimpStyle 	< 0) 		XleftGimpStyle = 0;
	if(XrightGimpStyle 	< 0) 		XrightGimpStyle = 0;
	if(XleftGimpStyle 	> 799)	XleftGimpStyle = 800;
	if(XrightGimpStyle 	> 799)	XrightGimpStyle = 800;

	if(YtopGimpStyle 		< 0) 		YtopGimpStyle = 0;
	if(YbottomGimpStyle < 0) 		YbottomGimpStyle = 0;
	if(YtopGimpStyle 		> 479)	YtopGimpStyle = 480;
	if(YbottomGimpStyle > 479)	YbottomGimpStyle = 480;

/*
	XSize = YbottomGimpStyle 	- YtopGimpStyle;
	YSize = XrightGimpStyle		- XleftGimpStyle;
	if((XSize <= 0) || (YSize <= 0))
		return;
	X0 = 479 - YbottomGimpStyle;
	Y0 = XleftGimpStyle;
	while((LTDC->CPSR & LTDC_CPSR_CYPOS) <= (uint32_t)800);
	HAL_LTDC_SetWindowSize(&LtdcHandle, XSize, YSize, LayerIdx);
	HAL_LTDC_SetWindowPosition(&LtdcHandle, X0, Y0,LayerIdx);
	HAL_LTDC_SetAddress(&LtdcHandle, pDestination, LayerIdx);
*/

	XSize = XrightGimpStyle		- XleftGimpStyle;
	YSize = YbottomGimpStyle 	- YtopGimpStyle;
	if((XSize <= 0) || (YSize <= 0))
		return;
	Y0 = 479 - YbottomGimpStyle;
	X0 = XleftGimpStyle;

	GFX_SetFrameBottom(pDestination, X0, Y0, XSize, YSize);
}


void GFX_logoAutoOff(void)
{
	if(logoStatus == LOGOOFF)
		logoStatus = LOGOSTART;
}


void GFX_hwBackgroundOn(void)
{
	backgroundHwStatus = LOGOSTART;
}


void GFX_hwBackgroundOff(void)
{
	backgroundHwStatus = LOGOSTOP;
}

void GFX_build_hw_background_frame(void)
{
	GFX_DrawCfgScreen	tLogoTemp;
	SWindowGimpStyle windowGimp;
	
	pBackgroundHwFrame = getFrame(1);
	backgroundHwStatus = 0;

	tLogoTemp.FBStartAdress = pBackgroundHwFrame;
	tLogoTemp.ImageHeight = 480;
	tLogoTemp.ImageWidth = 800;
	tLogoTemp.LayerIndex = 1;

	windowGimp.left = (800 - 400) / 2;
	windowGimp.top =  0;//(480 -  46) / 2;
	
	GFX_draw_image_color(&tLogoTemp, windowGimp, &ImgHWcolor);
/*	
	char localtext[256];
	uint8_t ptr = 0;
	
	localtext[ptr++] = ' ';
	localtext[ptr++] = ' ';
	localtext[ptr++] = 'O';
	localtext[ptr++] = 'S';
	localtext[ptr++] = ' ';
	ptr += GFX_printf_firmware(&localtext[ptr]);
	localtext[ptr] = 0;

	write_content_simple(&tLogoTemp, 0, 800, 240-24, &FontT24,localtext,CLUT_Font020);
*/	
}


void GFX_build_logo_frame(void)
{
	GFX_DrawCfgScreen	tLogoTemp;
	SWindowGimpStyle windowGimp;
	
	pLogoFrame = getFrame(1);
	logoStatus = LOGOOFF;

	tLogoTemp.FBStartAdress = pLogoFrame;
	tLogoTemp.ImageHeight = 480;
	tLogoTemp.ImageWidth = 800;
	tLogoTemp.LayerIndex = 1;

	windowGimp.left = (800 - 400) / 2;
	windowGimp.top =  (480 -  46) / 2;
	
	GFX_draw_image_color(&tLogoTemp, windowGimp, &ImgHWcolor);
/*	
	char localtext[256];
	uint8_t ptr = 0;
	
	localtext[ptr++] = ' ';
	localtext[ptr++] = ' ';
	localtext[ptr++] = 'O';
	localtext[ptr++] = 'S';
	localtext[ptr++] = ' ';
	ptr += GFX_printf_firmware(&localtext[ptr]);
	localtext[ptr] = 0;

	write_content_simple(&tLogoTemp, 0, 800, 240-24, &FontT24,localtext,CLUT_Font020);
*/	
}

void GFX_init(uint32_t  * pDestinationOut)
{
	frame[0].StartAddress = FBGlobalStart;
	GFX_clear_frame_immediately(frame[0].StartAddress);
	frame[0].status = CLEAR;
	frame[0].caller = 0;

	for(int i=1;i<MAXFRAMES;i++)
	{
		frame[i].StartAddress = frame[i-1].StartAddress + FBOffsetEachIndex;
		GFX_clear_frame_immediately(frame[i].StartAddress);
		frame[i].status = CLEAR;
		frame[i].caller = 0;
	}

	pInvisibleFrame = getFrame(2);
	*pDestinationOut = pInvisibleFrame;

	GFX_build_logo_frame();
	GFX_build_hw_background_frame();
	
  /* Register to memory mode with ARGB8888 as color Mode */
  Dma2dHandle.Init.Mode         = DMA2D_R2M;
  Dma2dHandle.Init.ColorMode    = DMA2D_ARGB4444;//to fake AL88,  before: DMA2D_ARGB8888;
  Dma2dHandle.Init.OutputOffset = 0;

  /* DMA2D Callbacks Configuration */
  Dma2dHandle.XferCpltCallback  = GFX_Dma2d_TransferComplete;
  Dma2dHandle.XferErrorCallback = GFX_Dma2d_TransferError;

  Dma2dHandle.Instance  = DMA2D;

  /* DMA2D Initialisation */
	if(HAL_DMA2D_Init(&Dma2dHandle) != HAL_OK)
		GFX_Error_Handler();

  if(HAL_DMA2D_ConfigLayer(&Dma2dHandle, 1) != HAL_OK)
		GFX_Error_Handler();

	DMA2D_at_work = 255;
}
void GFX_init1_no_DMA(uint32_t  * pDestinationOut, uint8_t blockFrames)
{
	frame[0].StartAddress = FBGlobalStart;
	GFX_clear_frame_immediately(frame[0].StartAddress);
	frame[0].status = CLEAR;
	frame[0].caller = 0;

	for(int i=1;i<MAXFRAMES;i++)
	{
		frame[i].StartAddress = frame[i-1].StartAddress + FBOffsetEachIndex;
		GFX_clear_frame_immediately(frame[i].StartAddress);
		frame[i].status = CLEAR;
		frame[i].caller = 0;
	}

	for(int i=0;i<blockFrames;i++)
	{
		frame[i].status = BLOCKED;
		frame[i].caller = 1;
	}
	
	pInvisibleFrame = getFrame(2);
	*pDestinationOut = pInvisibleFrame;

	GFX_build_logo_frame();
	GFX_build_hw_background_frame();
}


void GFX_init2_DMA(void)
{
  /* Register to memory mode with ARGB8888 as color Mode */
  Dma2dHandle.Init.Mode         = DMA2D_R2M;
  Dma2dHandle.Init.ColorMode    = DMA2D_ARGB4444;//to fake AL88,  before: DMA2D_ARGB8888;
  Dma2dHandle.Init.OutputOffset = 0;

  /* DMA2D Callbacks Configuration */
  Dma2dHandle.XferCpltCallback  = GFX_Dma2d_TransferComplete;
  Dma2dHandle.XferErrorCallback = GFX_Dma2d_TransferError;

  Dma2dHandle.Instance  = DMA2D;

  /* DMA2D Initialisation */
	if(HAL_DMA2D_Init(&Dma2dHandle) != HAL_OK)
		GFX_Error_Handler();

  if(HAL_DMA2D_ConfigLayer(&Dma2dHandle, 1) != HAL_OK)
		GFX_Error_Handler();

	DMA2D_at_work = 255;
}


void GFX_SetFrameTop(uint32_t pDestination)
{
	uint8_t NextTopWork = FrameHandler.NextTopWrite + 1;

	if(NextTopWork == RING_BUF_SIZE)
	{
		NextTopWork = 0;
	}

	if(pDestination == 0)
		pDestination = pInvisibleFrame;

	FrameHandler.pNextTopBuffer[NextTopWork] = pDestination;
	FrameHandler.NextTopWrite = NextTopWork;
}


void GFX_SetFrameBottom(uint32_t pDestination, uint32_t x0, uint32_t y0, uint32_t width, uint32_t height)
{
	uint8_t NextBottomWork = FrameHandler.NextBottomWrite + 1;

	if(NextBottomWork == RING_BUF_SIZE)
	{
		NextBottomWork = 0;
	}

	if(pDestination == 0)
		pDestination = pInvisibleFrame;

	FrameHandler.nextBottom[NextBottomWork].pBuffer = pDestination;
	FrameHandler.nextBottom[NextBottomWork].height = height;
	FrameHandler.nextBottom[NextBottomWork].width = width;
	FrameHandler.nextBottom[NextBottomWork].leftStart = x0;
	FrameHandler.nextBottom[NextBottomWork].bottomStart = y0;
	FrameHandler.NextBottomWrite = NextBottomWork;
}


void GFX_SetFramesTopBottom(uint32_t pTop, uint32_t pBottom, uint32_t heightBottom)
{
	GFX_SetFrameTop(pTop);
	GFX_SetFrameBottom(pBottom, 0, 0, 800, heightBottom);
}


static uint32_t GFX_get_pActualFrameTop(void)
{
	return FrameHandler.pActualTopBuffer;
}


static uint32_t GFX_get_pActualFrameBottom(void)
{
	return FrameHandler.actualBottom.pBuffer;
}


void GFX_start_VSYNC_IRQ(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  GPIO_InitStructure.Pin = VSYNC_IRQ_PIN;
  HAL_GPIO_Init(VSYNC_IRQ_GPIO_PORT, &GPIO_InitStructure);

  HAL_NVIC_SetPriority(VSYNC_IRQ_EXTI_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(VSYNC_IRQ_EXTI_IRQn);
}


void GFX_change_LTDC(void)
{
	uint32_t pTop = 0;
	uint32_t pBot = 0;
	uint32_t heightBot = 0;
	uint32_t widthBot = 0;
	uint32_t leftStartBot = 0;
	uint32_t bottomStartBot = 0;
	uint8_t change_position = 0;
	uint8_t change_size = 0;
	uint8_t nextBottomBackup = FrameHandler.NextBottomRead;		/* Restore entry value in case off logo handling */

	// Top Frame
	if(FrameHandler.NextTopRead != FrameHandler.NextTopWrite)
	{
		FrameHandler.NextTopRead++;
		if(FrameHandler.NextTopRead == RING_BUF_SIZE)
		{
			FrameHandler.NextTopRead = 0;
		}
	}
	pTop = FrameHandler.pNextTopBuffer[FrameHandler.NextTopRead];

	if(FrameHandler.pActualTopBuffer != pTop)
	{
		HAL_LTDC_SetAddress(&LtdcHandle, pTop, 1);
		FrameHandler.pActualTopBuffer = pTop;
	}	
	
	// Bottom Frame
	if(FrameHandler.NextBottomRead != FrameHandler.NextBottomWrite)
	{
		FrameHandler.NextBottomRead++;
		if(FrameHandler.NextBottomRead == RING_BUF_SIZE)
		{
			FrameHandler.NextBottomRead = 0;
		}
	}
	if(logoStatus != LOGOOFF)
	{
		switch(logoStatus)
			{
			case LOGOSTART:
				HAL_LTDC_SetAlpha(&LtdcHandle, 0, 1);
				HAL_LTDC_SetAlpha(&LtdcHandle, 34, 0);
				HAL_LTDC_ConfigCLUT(&LtdcHandle, (uint32_t *)indexHWcolor, indexHWcolorSIZE, 0);
				HAL_LTDC_SetAddress(&LtdcHandle, pLogoFrame, 0);
				HAL_LTDC_SetWindowSize(&LtdcHandle, 480, 800, 0);
				HAL_LTDC_SetWindowPosition(&LtdcHandle, 0, 0, 0);
				logoStatus = 2;
				FrameHandler.NextBottomRead = nextBottomBackup;
				break;

			case LOGOSTOP:
				HAL_LTDC_SetAlpha(&LtdcHandle, 255, 1);
				HAL_LTDC_ConfigCLUT(&LtdcHandle, ColorLUT, CLUT_END, 0);

				pBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].pBuffer;
				heightBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].height;
				widthBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].width;
				leftStartBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].leftStart;
				bottomStartBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].bottomStart;
				HAL_LTDC_SetWindowSize(&LtdcHandle, heightBot, widthBot, 0);
				HAL_LTDC_SetWindowPosition(&LtdcHandle, bottomStartBot, leftStartBot, 0);
				HAL_LTDC_SetAddress(&LtdcHandle, pBot, 0);
				HAL_LTDC_SetAlpha(&LtdcHandle, 255, 0);
				FrameHandler.actualBottom.height = heightBot;
				FrameHandler.actualBottom.width = widthBot;
				FrameHandler.actualBottom.leftStart = leftStartBot;
				FrameHandler.actualBottom.bottomStart = bottomStartBot;
				FrameHandler.actualBottom.pBuffer = pBot;

				logoStatus = LOGOOFF;
				if(backgroundHwStatus == 2)
				{
					backgroundHwStatus = LOGOSTART;
				}
				break;
			default:
				if(logoStatus < 35)
				{
					logoStatus++;
					if(logoStatus <= 15)
						HAL_LTDC_SetAlpha(&LtdcHandle, 17*logoStatus, 0);
				}
				else
				{
					logoStatus +=20;
					HAL_LTDC_SetAlpha(&LtdcHandle, logoStatus-55, 1);
					HAL_LTDC_SetAlpha(&LtdcHandle, 255+55-logoStatus, 0);
				}
				FrameHandler.NextBottomRead = nextBottomBackup;
				break;
			}
		return;
	}
	else if (backgroundHwStatus != LOGOOFF)
	{

		switch(backgroundHwStatus)
			{
			case LOGOSTART:
				HAL_LTDC_ConfigCLUT(&LtdcHandle, (uint32_t *)indexHWcolor, indexHWcolorSIZE, 0);
				HAL_LTDC_SetAddress(&LtdcHandle, pBackgroundHwFrame, 0);
				HAL_LTDC_SetWindowSize(&LtdcHandle, 480, 800, 0);
				HAL_LTDC_SetWindowPosition(&LtdcHandle, 0, 0, 0);
				backgroundHwStatus = 2;
				FrameHandler.NextBottomRead = nextBottomBackup;
				break;

			case LOGOSTOP:
				HAL_LTDC_ConfigCLUT(&LtdcHandle, ColorLUT, CLUT_END, 0);
				pBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].pBuffer;
				heightBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].height;
				widthBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].width;
				leftStartBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].leftStart;
				bottomStartBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].bottomStart;
				HAL_LTDC_SetWindowSize(&LtdcHandle, heightBot, widthBot, 0);
				HAL_LTDC_SetWindowPosition(&LtdcHandle, bottomStartBot, leftStartBot, 0);
				HAL_LTDC_SetAddress(&LtdcHandle, pBot, 0);
				HAL_LTDC_SetAlpha(&LtdcHandle, 255, 0);
				FrameHandler.actualBottom.height = heightBot;
				FrameHandler.actualBottom.width = widthBot;
				FrameHandler.actualBottom.leftStart = leftStartBot;
				FrameHandler.actualBottom.bottomStart = bottomStartBot;
				FrameHandler.actualBottom.pBuffer = pBot;
				backgroundHwStatus = LOGOOFF;
				break;

			default:
				FrameHandler.NextBottomRead = nextBottomBackup;
				break;
			}

		return;
	}
	else
	{
		pBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].pBuffer;
		heightBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].height;
		widthBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].width;
		leftStartBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].leftStart;
		bottomStartBot = FrameHandler.nextBottom[FrameHandler.NextBottomRead].bottomStart;

		if(FrameHandler.actualBottom.pBuffer == pBot)
			pBot = 0;
		
		if((FrameHandler.actualBottom.height != heightBot) || (FrameHandler.actualBottom.width != widthBot))
			change_size = 1;

		if((FrameHandler.actualBottom.leftStart != leftStartBot) || (FrameHandler.actualBottom.bottomStart != bottomStartBot))
			change_position = 1;

		if(pBot || change_size || change_position)
		{
			if(heightBot && widthBot)
				HAL_LTDC_SetWindowSize(&LtdcHandle, heightBot, widthBot, 0);

			if(change_position || leftStartBot || bottomStartBot)
				HAL_LTDC_SetWindowPosition(&LtdcHandle, bottomStartBot, leftStartBot, 0);

			if(pBot)
				HAL_LTDC_SetAddress(&LtdcHandle, pBot, 0);

			if(change_size)
			{
				FrameHandler.actualBottom.height = heightBot;
				FrameHandler.actualBottom.width = widthBot;
			}
			if(change_position)
			{
				FrameHandler.actualBottom.leftStart = leftStartBot;
				FrameHandler.actualBottom.bottomStart = bottomStartBot;
			}
			if(pBot)
				FrameHandler.actualBottom.pBuffer = pBot;
		}		
	}
}

uint8_t GFX_is_colorschemeDiveStandard(void)
{
	return (ColorLUT[CLUT_Font027] == 0x00FFFFFF);
}


void change_CLUT_entry(uint8_t entryToChange, uint8_t entryUsedForChange)
{
/* bug fix
	static uint8_t counter = 0;
	
	if(entryToChange == 0x1C)
		counter++;
*/	
	ColorLUT[entryToChange] = ColorLUT[entryUsedForChange];
	HAL_LTDC_ConfigCLUT(&LtdcHandle, ColorLUT, CLUT_END, 1);
	if(logoStatus == LOGOOFF)
		HAL_LTDC_ConfigCLUT(&LtdcHandle, ColorLUT, CLUT_END, 0);
}


void GFX_use_colorscheme(uint8_t colorscheme)
{
	uint8_t ColorSchemeStart;

	if(colorscheme > 3)
		colorscheme = 0;

	ColorSchemeStart = CLUT_Colorscheme0 + (8 * colorscheme);
	for(int i=1; i<8; i++)
	{
		ColorLUT[CLUT_Font027 + i] = ColorLUT[ColorSchemeStart + i];
	}
	change_CLUT_entry(CLUT_Font027, ColorSchemeStart);
}


void GFX_VGA_transform(uint32_t pSource, uint32_t pDestination)
{
	int h, v;
	uint32_t offsetSource, offsetSourceStartOfLine;

	offsetSourceStartOfLine = 480 + 480 - 2;
	for(v=0;v<480;v++)
	{
		offsetSource = offsetSourceStartOfLine;
		for(h=0;h<640;h++)
		{
			*(__IO uint8_t*)pDestination = *(uint8_t*)(pSource + offsetSource);
			pDestination++;
			offsetSource += 1;
			*(__IO uint8_t*)pDestination = *(uint8_t*)(pSource + offsetSource);
			pDestination++;
			offsetSource += 480 + 479;
		}
		offsetSourceStartOfLine -= 2;
	}
}


static void GFX_clear_frame_immediately(uint32_t pDestination)
{
	uint32_t i;
	uint32_t* pfill = (uint32_t*) pDestination;


	for(i = 200*480; i > 0; i--)
	{
		*pfill++ = 0;
		*pfill++ = 0;
	}
}


void GFX_clear_window_immediately(GFX_DrawCfgWindow* hgfx)
{
	uint32_t pDestination, i, j;
	uint16_t left, width, bottom, height, nextlineStep;

	pDestination = (uint32_t)hgfx->Image->FBStartAdress;

	left 		= hgfx->WindowX0;
	width 	= 1 + hgfx->WindowX1 - left;
	bottom 	= hgfx->WindowY0;
	height 	= 1 + hgfx->WindowY1 - bottom;
	nextlineStep = hgfx->Image->ImageHeight - height;
	nextlineStep *= 2;

	pDestination += 2 * bottom;
	pDestination += 2 * hgfx->Image->ImageHeight * left;

	for(j = width; j > 0; j--)
	{
		for(i = height; i > 0; i--)
		{
			*(__IO uint16_t*)pDestination = 0;
			pDestination += 2;
		}
		pDestination += nextlineStep;
	}
}


static void  GFX_clear_frame_dma2d(uint8_t frameId)
{
	if(frameId >= MAXFRAMES)
		return;

	DMA2D_at_work = frameId;

	if (HAL_DMA2D_Start_IT(&Dma2dHandle, 0x0000000000, frame[frameId].StartAddress, 480, 800) != HAL_OK)
		GFX_Error_Handler();
}


void GFX_fill_buffer(uint32_t pDestination, uint8_t alpha, uint8_t color)
{

	union al88_u
	{
		uint8_t al8[2];
		uint16_t al88;
	};
	union al88_u colorcombination;
	uint32_t i;
	uint32_t* pfill = (uint32_t*) pDestination;
	uint32_t fillpattern;

	colorcombination.al8[0] = color;
	colorcombination.al8[1] = alpha;

	fillpattern = (colorcombination.al88 << 16) | colorcombination.al88;
	for(i = 800*480/2; i > 0; i--)
	{
		*pfill++ = fillpattern;
	}
}


static void gfx_flip(point_t *p1, point_t *p2)
{
	point_t temp;
	
	temp = *p1;
	*p1 = *p2;
	*p2 = temp;
}


static inline void gfx_brush(uint8_t thickness, GFX_DrawCfgScreen *hgfx, uint16_t x0, uint16_t y0, uint8_t color)
{
	uint16_t* pDestination;
	uint8_t offset = thickness/2;
	int16_t stepdir;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(pSettings->FlipDisplay)
	{
		pDestination = (uint16_t*)hgfx->FBStartAdress;
		pDestination += (hgfx->ImageHeight * (hgfx->ImageWidth - x0 + offset)) + (480 - y0+offset);
		stepdir = -1;
	}
	else
	{
		pDestination = (uint16_t*)hgfx->FBStartAdress;
		pDestination += (x0 - offset)*hgfx->ImageHeight + (y0-offset);
		stepdir = 1;
	}
	for(int x=thickness;x>0;x--)
	{
		for(int y=thickness;y>0;y--)
		{
			*(__IO uint16_t*)pDestination = 0xFF00 + color;
			pDestination += stepdir;
		}
		pDestination += stepdir * (hgfx->ImageHeight - thickness);
	}
}


void GFX_draw_thick_line(uint8_t thickness, GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color)
{
	if(thickness < 2)
		GFX_draw_line(hgfx,  start,  stop,  color);
	
	int x0 = start.x;
	int y0 = start.y;
	int x1 = stop.x;
	int y1 = stop.y;
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
	int err = (dx>dy ? dx : -dy)/2, e2;

	
	if(start.x == stop.x)
	{
		if(start.y > stop.y) gfx_flip(&start,&stop);
		for (int j = stop.y - start.y; j > 0; j--)
		{
			gfx_brush(thickness,hgfx,start.x,start.y++,color);
		}
	}
	else
	if(start.y == stop.y)
	{
		if(start.x > stop.x) gfx_flip(&start,&stop);
		
		for (int j = stop.x - start.x; j > 0; j--)
		{
			gfx_brush(thickness,hgfx,start.x++,start.y,color);
		}
	}
	else // diagonal
	{
		for(;;)
		{
			gfx_brush(thickness,hgfx,x0,y0,color);
			if (x0==x1 && y0==y1) break;
			e2 = err;
			if (e2 >-dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}	
}


void GFX_draw_line(GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color)
{
	uint16_t* pDestination;
	uint32_t j;
	int16_t stepdir;
	SSettings* pSettings;
	pSettings = settingsGetPointer();


	/* horizontal line */
	if(start.x == stop.x)
	{
		if(start.y > stop.y) gfx_flip(&start,&stop);

		pDestination = (uint16_t*)hgfx->FBStartAdress;
		if(pSettings->FlipDisplay)
		{
			pDestination += (799 - start.x) * hgfx->ImageHeight;
			pDestination += (479 - start.y);
			stepdir = -1;
		}
		else
		{
			pDestination += start.x * hgfx->ImageHeight;
			pDestination += start.y;
			stepdir = 1;
		}
		for (j = stop.y - start.y; j > 0; j--)
		{
				*(__IO uint16_t*)pDestination = 0xFF00 + color;
				pDestination += stepdir;
		}
	}
	else /* vertical line ? */
	if(start.y == stop.y)
	{
		if(start.x > stop.x) gfx_flip(&start,&stop);
		pDestination = (uint16_t*)hgfx->FBStartAdress;

		if(pSettings->FlipDisplay)
		{
			pDestination += (799 - start.x) * hgfx->ImageHeight;
			pDestination += (479 - start.y);
			stepdir = -1;
		}
		else
		{
			pDestination += start.x * hgfx->ImageHeight;
			pDestination += start.y;
			stepdir = 1;
		}

		for (j = stop.x - start.x; j > 0; j--)
		{
			*(__IO uint16_t*)pDestination = 0xFF00 + color;
			pDestination += stepdir * hgfx->ImageHeight;
		}
	}
	else /* diagonal */
	{
		int x0 = start.x;
		int y0 = start.y;
		int x1 = stop.x;
		int y1 = stop.y;
		int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
		int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
		int err = (dx>dy ? dx : -dy)/2, e2;
	 
		for(;;)
		{
			pDestination = (uint16_t*)hgfx->FBStartAdress;

			if(pSettings->FlipDisplay)
			{
				pDestination += (((799 - x0) * hgfx->ImageHeight) + (479 - y0));
			}
			else
			{
				pDestination += ((x0 * hgfx->ImageHeight) + y0);
			}

			*(__IO uint16_t*)pDestination = 0xFF00 + color;
			if (x0==x1 && y0==y1) break;
			e2 = err;
			if (e2 >-dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}
}


void GFX_draw_image_monochrome(GFX_DrawCfgScreen *hgfx, SWindowGimpStyle window, const tImage *image, uint8_t color)
{
	uint16_t* pDestination;
	uint32_t j;
	point_t start, stop;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	start.x = window.left;
	start.y = (hgfx->ImageHeight - image->height - window.top);
	stop.y = start.y + image->height;
	stop.x = start.x + image->width;
	j = 0;

	if(pSettings->FlipDisplay)
	{
		for(int xx = start.x; xx < stop.x; xx++)
		{
			pDestination = (uint16_t*)hgfx->FBStartAdress;
			pDestination += (hgfx->ImageHeight - start.y) + (stop.x * hgfx->ImageHeight) ;
			pDestination -= (xx - start.x) * hgfx->ImageHeight;

			for(int yy = start.y; yy < stop.y; yy++)
			{
				*(__IO uint16_t*)pDestination-- = (image->data[j++] << 8) + color;
			}
		}
	}
	else
	{
		for(int xx = start.x; xx < stop.x; xx++)
		{
			pDestination = (uint16_t*)hgfx->FBStartAdress;
			pDestination += xx * hgfx->ImageHeight;
			pDestination += start.y;
			for(int yy = start.y; yy < stop.y; yy++)
			{
				*(__IO uint16_t*)pDestination++ = (image->data[j++] << 8) + color;
			}
		}
	}
}
	
static void GFX_draw_image_color(GFX_DrawCfgScreen *hgfx, SWindowGimpStyle window, const tImage *image)
{
	uint16_t* pDestination;

	uint32_t j;
	point_t start, stop;

	start.x = window.left;
	start.y = (hgfx->ImageHeight - image->height - window.top);
	stop.y = start.y + image->height;
	stop.x = start.x + image->width;
	j = 0;
	
	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(pSettings->FlipDisplay)
	{
		for(int xx = start.x; xx < stop.x; xx++)
		{
			pDestination = (uint16_t*)hgfx->FBStartAdress;
			pDestination += (hgfx->ImageHeight - start.y) + (stop.x * hgfx->ImageHeight);
			pDestination -= (xx - start.x) * hgfx->ImageHeight;

			for(int yy = start.y; yy < stop.y; yy++)
			{
				*(__IO uint16_t*)pDestination-- = 0xFF << 8 | image->data[j++];
			}
		}
	}
	else
	{
		for(int xx = start.x; xx < stop.x; xx++)
		{
			pDestination = (uint16_t*)hgfx->FBStartAdress;
			pDestination += xx * hgfx->ImageHeight;
			pDestination += start.y;
			for(int yy = start.y; yy < stop.y; yy++)
			{
				*(__IO uint16_t*)pDestination++ = 0xFF << 8 | image->data[j++];
			}
		}
	}
}


/* this is NOT fast nor optimized */
static void GFX_draw_pixel(GFX_DrawCfgScreen *hgfx, int16_t x, int16_t y, uint8_t color)
{
	uint16_t* pDestination;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	pDestination = (uint16_t*)hgfx->FBStartAdress;
	if(pSettings->FlipDisplay)
	{
		pDestination += (800 - x) * hgfx->ImageHeight;
		pDestination += (480 - y);
	}
	else
	{
		pDestination += x * hgfx->ImageHeight;
		pDestination += y;
	}
	*(__IO uint16_t*)pDestination = 0xFF << 8 | color;
}

/* this is NOT fast nor optimized */
void GFX_draw_circle(GFX_DrawCfgScreen *hgfx, point_t center, uint8_t radius, int8_t color)
{
  int x, y;
  int l;
  int r2, y2;
  int y2_new;
  int ty;

  /* cos pi/4 = 185363 / 2^18 (approx) */
  l = (radius * 185363) >> 18;

	/* hw */
	l += 1;
	
  /* At x=0, y=radius */
  y = radius;

  r2 = y2 = y * y;
  ty = (2 * y) - 1;
  y2_new = r2 + 3;

  for (x = 0; x <= l; x++) {
    y2_new -= (2 * x) - 3;

    if ((y2 - y2_new) >= ty) {
      y2 -= ty;
      y -= 1;
      ty -= 2;
    }

    GFX_draw_pixel (hgfx,  x + center.x,  y + center.y, color);
    GFX_draw_pixel (hgfx,  x + center.x, -y + center.y, color);
    GFX_draw_pixel (hgfx, -x + center.x,  y + center.y, color);
    GFX_draw_pixel (hgfx, -x + center.x, -y + center.y, color);

    GFX_draw_pixel (hgfx,  y + center.x,  x + center.y, color);
    GFX_draw_pixel (hgfx,  y + center.x, -x + center.y, color);
    GFX_draw_pixel (hgfx, -y + center.x,  x + center.y, color);
    GFX_draw_pixel (hgfx, -y + center.x, -x + center.y, color);
  }
}


void GFX_draw_colorline(GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color)
{
	uint32_t pDestination;
	uint32_t j;
	uint32_t temp;

	if(start.x == stop.x)
	{
		if(stop.y < start.y)
		{
			temp = stop.y;
			stop.y = start.y;
			start.y = temp;
		}
		pDestination = (uint32_t)hgfx->FBStartAdress;
		pDestination += start.x * hgfx->ImageHeight * 2;
		pDestination += start.y * 2;
		for (j = stop.y - start.y; j > 0; j--)
		{
			*(__IO uint8_t*)pDestination = color;
			pDestination += 1;
			*(__IO uint8_t*)pDestination = 0xFF;
			pDestination += 1;
		}
	}
	else
	if(start.y == stop.y)
	{
		if(stop.x < start.x)
		{
			temp = stop.x;
			stop.x = start.x;
			start.x = temp;
		}
		pDestination = (uint32_t)hgfx->FBStartAdress;
		pDestination += start.x * hgfx->ImageHeight * 2;
		pDestination += start.y * 2;
		for (j = stop.x - start.x; j > 0; j--)
		{
			*(__IO uint8_t*)pDestination = color;
			pDestination += 1;
			*(__IO uint8_t*)pDestination = 0xFF;
			pDestination -= 1;
			pDestination += hgfx->ImageHeight * 2;
		}
	}
	else // diagonal Bresenham's_line_algorithm
	{
		int x0 = start.x;
		int y0 = start.y;
		int x1 = stop.x;
		int y1 = stop.y;
		int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
		int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
		int err = (dx>dy ? dx : -dy)/2, e2;
	 
		for(;;)
		{
			pDestination = (uint32_t)hgfx->FBStartAdress;
			pDestination += ((x0 * hgfx->ImageHeight) + y0) * 2;
			*(__IO uint8_t*)pDestination = color;
			pDestination += 1;
			*(__IO uint8_t*)pDestination = 0xFF;
			if (x0==x1 && y0==y1) break;
			e2 = err;
			if (e2 >-dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}
}


void GFX_draw_Grid(GFX_DrawCfgScreen *hgfx, SWindowGimpStyle window, int vlines, float vdeltaline, int hlines, float hdeltalines, uint8_t color)
{
    point_t p1;
    point_t p2;
    int winthight = window.bottom - window.top;
	int winwidth = window.right - window.left;
	float deltaline = 0;

    if(vlines > 0)
    {
        deltaline = ((float)winwidth) /vlines;

        p1.y = 479 - window.top;
        p2.y = 479 - window.bottom;
		for(int i = 0; i <= vlines; i++)
		{
            p1.x = window.left + (int)(i * deltaline + 0.5f);
            p2.x = p1.x ;
            //GFX_draw_colorline(hgfx, p1,p2, color );
            GFX_draw_line(hgfx, p1,p2, color );
		}
    }
    if(vdeltaline > 0)
    {
        p1.y = 479 - window.top;
        p2.y = 479 - window.bottom;
		for(int i = 0; i < winwidth/vdeltaline; i++)
		{
            p1.x = window.left + (int)(i * vdeltaline + 0.5f);
            p2.x = p1.x ;
          //  GFX_draw_colorline(hgfx, p1,p2, color );
            GFX_draw_line(hgfx, p1,p2, color );
		}
    }
    if(hlines > 0)
    {
        deltaline = ((float)winthight)/hlines;
        p1.x = window.left;
        p2.x = window.right;
		for(int i = 0; i <= hlines; i++)
		{
            p1.y = 479 - window.top - (int)(i * deltaline + 0.5f);
            p2.y = p1.y;
           // GFX_draw_colorline(hgfx, p1,p2, color );
            GFX_draw_line(hgfx, p1,p2, color );
		}
    }
}

//  ===============================================================================
//	GFX_graph_print
/// @brief	Print all those nice curves, especially in logbook und miniLiveLogGraph
///	@version 0.0.2 hw 160519
///	
/// 151022 hw -bug fix 
/// - die aktuelle Version macht keine Linien mehr �ber die gesamte Bildschirmh�he.
/// - daf�r sind L�cher in der Kurve (z.B. Temperaturgraph Tauchgang Matthias 17.10.15 15:19)
///
/// more details about range can be found in show_logbook_logbook_show_log_page2() - temperature graph
///
///	@param 	window: top and bottom is only the range used by the data of the graph, not the entire screen / scale
/// @param 	drawVeilUntil: ist auff�llen des Bereichs unter der Kurve mit etwas hellerer Farbe
///	@param 	Xdivide: wird bisher nichr benutzt.
//  ===============================================================================


void GFX_graph_print(GFX_DrawCfgScreen *hgfx, const  SWindowGimpStyle *window, const int16_t drawVeilUntil, uint8_t Xdivide, uint16_t dataMin, uint16_t dataMax,  uint16_t *data, uint16_t datalength, uint8_t color, uint8_t *colour_data)
{
	uint16_t* pDestination_tmp;
	uint16_t* pDestination_start;
	uint16_t* pDestination_end;
	uint16_t* pDestination_zero_veil;

	SSettings* pSettings;

	uint32_t max = 0;
	int windowheight = -1;
	int windowwidth = -1;
	int i = -1;
	int w1 = -1;
	int w2 = -1;

	uint32_t h_ulong = 0;
	uint32_t h_ulong_old = 0;
	_Bool invert = 0;

	uint16_t dataDelta = 0;
	uint16_t dataDeltaHalve = 0;
	uint16_t dataTemp = 0;
	
	uint8_t colorDataTemp;
	uint8_t	colormask = 0;

	pSettings = settingsGetPointer();
	pDestination_zero_veil = 0;

	if(dataMin > dataMax)
	{
		uint16_t dataFlip;
		dataFlip = dataMin;
		dataMin = dataMax;
		dataMax = dataFlip;
		invert = 1;
	}
	else
		invert = 0;

	colormask = color;

	pSettings = settingsGetPointer();
		
	if(window->bottom > 479)
		return;
	if(window->top > 479)
		return;
	if(window->right > 799)
		return;
	if(window->left > 799)
		return;
	if(window->bottom < 0)
		return;
	if(window->top < 0)
		return;
	if(window->right < 0)
		return;
	if(window->left < 0)
		return;
	if(window->bottom <= window->top)
		return;
	if(window->right <= window->left)
		return;
	
	windowheight = window->bottom - window->top ;
	windowwidth = window->right - window->left;
	w1 = 0;
	w2 = 0;
	if(dataMax == dataMin)
		dataMax++;
	dataDelta =  (unsigned long)(dataMax - dataMin);
	dataDeltaHalve = dataDelta / 2;
	while((w1 <= windowwidth) && (w2 < datalength))
	{
		int tmp = (10 * w1 * (long)datalength)/windowwidth;
		w2 = tmp/10;
		int rest = tmp - w2*10;
		if(rest >= 5)
			w2++;

		if((datalength - 1) < w2)
			w2 = datalength-1;

		if(colour_data != NULL)
		{
			colorDataTemp = colour_data[w2];
			colormask =    color + colorDataTemp;
		}

		dataTemp = data[w2];
		if(Xdivide > 1)
		{
			w2++;
			for(i=1;i<Xdivide;i++)
			{
				if(data[w2]>dataTemp)
					dataTemp = data[w2];
				w2++;
			}
		}
		
		if(dataTemp > dataMin)
			dataTemp -= dataMin;
		else
			dataTemp = 0;

		if(invert)
		{
			if(dataTemp < dataDelta)
				dataTemp = dataDelta - dataTemp;
			else
				dataTemp = 0;
		}
		
		h_ulong = (unsigned long)dataTemp;
		h_ulong *= windowheight;
		h_ulong += dataDeltaHalve;
		h_ulong /= dataDelta;
		
		if(h_ulong > (window->bottom - window->top))
			h_ulong = (window->bottom - window->top);

		if(!pSettings->FlipDisplay)
		{
			if(drawVeilUntil > 0)
			{
					pDestination_zero_veil = (uint16_t*)hgfx->FBStartAdress;
					pDestination_zero_veil += ((479 - (drawVeilUntil - 2) ) + ((w1 + window->left) * hgfx->ImageHeight) );
			}
			else if(drawVeilUntil < 0 )
			{
					pDestination_zero_veil = (uint16_t*)hgfx->FBStartAdress;
					pDestination_zero_veil += ((479 + (drawVeilUntil)) + ((w1 + window->left) * hgfx->ImageHeight) );
			}
		}
		else
		{
			if(drawVeilUntil > 0)
			{
				pDestination_zero_veil = (uint16_t*)hgfx->FBStartAdress;
				pDestination_zero_veil += (((drawVeilUntil) ) + ( (window->right - w1) * hgfx->ImageHeight) );
							}
			else if(drawVeilUntil < 0 )
			{
				pDestination_zero_veil = (uint16_t*)hgfx->FBStartAdress;
				pDestination_zero_veil += 479 -  drawVeilUntil + ( (window->right - w1 -1) * hgfx->ImageHeight);
			}
		}
		if(h_ulong + window->top > max)
		{
			max = h_ulong + window->top;
		}

// hw 160519 wof�r ist das? Damit funktioniert Temperatur 25,5�C nicht!
//		if((dataMax == 255) || (data[w2] != 255))
//		{
			//output_content[pointer] = colormask;
			//output_mask[pointer] = true;
		if(dataTemp != 0xFFFF)	/* do not draw invalid data pixels */
		{
			if(w1 > 0)
			{
				pDestination_start = (uint16_t*)hgfx->FBStartAdress;
				if(!pSettings->FlipDisplay)
				{
					pDestination_start += (((479 - (window->top)) + ((w1 + window->left) * hgfx->ImageHeight)));
				}
				else
				{
					pDestination_start += (((window->top) + ((window->right - w1) * hgfx->ImageHeight)));
				}
				pDestination_end = pDestination_start;

				if(!pSettings->FlipDisplay)
				{
					if(h_ulong >= h_ulong_old)
					{
						pDestination_start -= h_ulong_old;
						pDestination_end -= h_ulong;
					}
					else
					{
						pDestination_start -= h_ulong;
						pDestination_end -= h_ulong_old;
					}
				}
				else
				{
					if(h_ulong < h_ulong_old)
					{
						pDestination_start += h_ulong_old;
						pDestination_end += h_ulong;
					}
					else
					{
						pDestination_start += h_ulong;
						pDestination_end += h_ulong_old;
					}
				}

				
				// deco stops
				if(drawVeilUntil < 0)
				{
					if(!pSettings->FlipDisplay)
					{
						pDestination_tmp = pDestination_end;
						while(pDestination_tmp <= pDestination_zero_veil)
						{
							*(__IO uint16_t*)pDestination_tmp = (0x80 << 8) | colormask;
							pDestination_tmp++;
						}
					}
					else
					{
						pDestination_tmp = pDestination_zero_veil;
						while(pDestination_tmp <=  pDestination_end)
						{
							*(__IO uint16_t*)pDestination_tmp = (0x80 << 8) | colormask;
							pDestination_tmp++;
						}
					}
				}
				else
				{
					// regular graph with veil underneath if requested
					// von oben nach unten
					// von grossen pDestination Werten zu kleinen pDestination Werten
					{
						pDestination_tmp = pDestination_start;
						while(pDestination_tmp >= pDestination_end)
						{
							*(__IO uint16_t*)pDestination_tmp = (0xFF << 8) | colormask ;
							pDestination_tmp--;
						}
					}

					if(!pSettings->FlipDisplay)
					{
						while((drawVeilUntil > 0) && (pDestination_tmp >= pDestination_zero_veil))
						{
							*(__IO uint16_t*)pDestination_tmp = (0x20 << 8) | colormask ;
							pDestination_tmp--;
						}
					}
					else
					{
						pDestination_tmp = pDestination_start;
						while((drawVeilUntil > 0) && (pDestination_tmp <=  pDestination_zero_veil))
						{
							*(__IO uint16_t*)pDestination_tmp = (0x20 << 8) | colormask ;
							pDestination_tmp++;
						}
					}
				}
			}
			h_ulong_old = h_ulong;
		}
		w1++;
		w2++;
	}
}


void GFX_draw_header(GFX_DrawCfgScreen *hgfx, uint8_t colorId)
{
	uint32_t pDestination;
	point_t start, stop, now;
	uint8_t alpha;

	/* display coordinate system */
	start.y = 400;
	stop.y = 479;

	start.x = 0;
	stop.x = 799;

	now.y = start.y;
	now.x = start.x;

	while (now.x <= stop.x)
	{
		now.y = start.y;
		pDestination = (uint32_t)hgfx->FBStartAdress;
		pDestination += now.x * hgfx->ImageHeight * 2;
		pDestination += now.y * 2;
		now.x += 1;

		alpha = 27;
		while(alpha < 246)
		{
			alpha += 9;
			*(__IO uint8_t*)pDestination = colorId;
			pDestination += 1;
			*(__IO uint8_t*)pDestination = alpha;
			pDestination += 1;
			now.y += 1;
		}

		while(now.y <= stop.y)
		{
			*(__IO uint8_t*)pDestination = colorId;
			pDestination += 1;
			*(__IO uint8_t*)pDestination = 0xFF;
			pDestination += 1;
			now.y += 1;
		}
	}
}

void GFX_draw_box2(GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color, uint8_t roundCorners)
{
	point_t point2, point4;

	if(roundCorners)
	{
		point2.x  = stop.x - start.x;
		point2.y  = stop.y - start.y;
		GFX_draw_box(hgfx,start,point2,1,color);
	}
	else
	{
		point2.x  = stop.x;
		point2.y  = start.y;

		point4.x  = start.x;
		point4.y  = stop.y;
		
		GFX_draw_line(hgfx,start,point2,color);
		GFX_draw_line(hgfx,point2,stop,color);
		GFX_draw_line(hgfx,stop,point4,color);
		GFX_draw_line(hgfx,point4,start,color);
	}
}

void GFX_draw_box(GFX_DrawCfgScreen *hgfx, point_t LeftLow, point_t WidthHeight, uint8_t Style, uint8_t color)
{
	uint16_t* pDestination;
	uint16_t* pStart;
	uint32_t j;
	uint32_t lineWidth, lineHeight;
	int x, y;
	uint8_t intensity;
	int stepdir;

	typedef struct {
			int x;
			int y;
			uint8_t intensity;
	} corner_t;
	const corner_t corner[16] = {
		{3,3,255}, // nur einmal
		{9,0,242},
		{8,0,194},
		{7,0,115},
		{6,0,36},
		{9,1,33},
		{8,1,84},
		{7,1,161},
		{6,1,255},
		{5,1,242},
		{4,1,36},
		{6,2,33},
		{5,2,84},
		{4,2,255},
		{3,2,84},
		{4,3,110}
	};

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	lineWidth = WidthHeight.x;
	lineHeight = WidthHeight.y;
	pStart = (uint16_t*)hgfx->FBStartAdress;

	if(!pSettings->FlipDisplay)
	{
		pStart += LeftLow.x * hgfx->ImageHeight;
		pStart += LeftLow.y;
		stepdir = 1;
	}
	else
	{
		pStart += (799 - LeftLow.x) * hgfx->ImageHeight;
		pStart += (479 - LeftLow.y);
		stepdir = -1;
	}

	// Untere Linie
	pDestination = pStart;
	if(Style)
	{
		pDestination += stepdir * 10 * hgfx->ImageHeight;
		lineWidth -= 18;
	}
	for (j = lineWidth; j > 0; j--)
	{

		*(__IO uint16_t*)pDestination = 0xFF00 + color;
		pDestination += stepdir * hgfx->ImageHeight;
	}

	// Obere Linie

	pDestination = pStart + stepdir * WidthHeight.y;
	if(Style)
	{
		pDestination += stepdir * 10 * hgfx->ImageHeight;
	}

	for (j = lineWidth; j > 0; j--)
	{
		*(__IO uint16_t*)pDestination = 0xFF00 + color;
		pDestination += stepdir * hgfx->ImageHeight;
	}

	// Linke Linie
	pDestination = pStart;

	if(Style)
	{
		pDestination += stepdir * 10;
		lineHeight -= 18;
	}

	for (j = lineHeight; j > 0; j--)
	{
		*(__IO uint16_t*)pDestination = 0xFF00 + color;
		pDestination += stepdir;
	}


	// Rechte Linie

	pDestination = pStart + stepdir * WidthHeight.x * hgfx->ImageHeight;
	if(Style)
	{
		pDestination += stepdir * 10;
	}

	for (j = lineHeight; j > 0; j--)
	{
		*(__IO uint16_t*)pDestination = 0xFF00 + color;
		pDestination += stepdir;
	}


	// Ecken wenn notwendig == Style
	if(Style)
	{
		// links unten
		pDestination = pStart;
		x = corner[0].x;
		y = corner[0].y;
		intensity = corner[0].intensity;

    *(__IO uint16_t*)(pDestination  + stepdir * (y + (x * hgfx->ImageHeight))) = (intensity << 8) + color;

		for(j = 15; j > 0; j--)
		{
			x = corner[j].x;
			y = corner[j].y;
			intensity = corner[j].intensity;
			*(__IO uint16_t*)(pDestination + stepdir * (y + (x * hgfx->ImageHeight))) = (intensity << 8) + color;
			*(__IO uint16_t*)(pDestination + stepdir	* (x + (y * hgfx->ImageHeight))) = (intensity << 8) + color;
		}
		// links oben
		pDestination = pStart + stepdir * WidthHeight.y;
		x = corner[0].x;
		y = corner[0].y;
		intensity = corner[0].intensity;
    *(__IO uint16_t*)(pDestination + stepdir * (-y + (x * hgfx->ImageHeight))) = (intensity << 8) + color;

		for(j = 15; j > 0; j--)
		{
			x = corner[j].x;
			y = corner[j].y;
			intensity = corner[j].intensity;
			*(__IO uint16_t*)(pDestination + stepdir * (-y + (x * hgfx->ImageHeight))) = (intensity << 8) + color;
			*(__IO uint16_t*)(pDestination + stepdir * (-x + (y * hgfx->ImageHeight))) = (intensity << 8) + color;
		}
		// rechts unten
		pDestination = pStart + stepdir * WidthHeight.x * hgfx->ImageHeight;
		x = corner[0].x;
		y = corner[0].y;
		intensity = corner[0].intensity;
    *(__IO uint16_t*)(pDestination + stepdir * (y - (x * hgfx->ImageHeight))) = (intensity << 8) + color;

		for(j = 15; j > 0; j--)
		{
			x = corner[j].x;
			y = corner[j].y;
			intensity = corner[j].intensity;
			*(__IO uint16_t*)(pDestination + stepdir * (y - (x * hgfx->ImageHeight))) = (intensity << 8) + color;
			*(__IO uint16_t*)(pDestination + stepdir * (x - (y * hgfx->ImageHeight))) = (intensity << 8) + color;
		}
		// rechts oben
		pDestination = pStart + stepdir * WidthHeight.y + stepdir * WidthHeight.x * hgfx->ImageHeight;
		x = corner[0].x;
		y = corner[0].y;
		intensity = corner[0].intensity;
    *(__IO uint16_t*)(pDestination + stepdir * -1 * (y + (x * hgfx->ImageHeight))) = (intensity << 8) + color;

		for(j = 15; j > 0; j--)
		{
			x = corner[j].x;
			y = corner[j].y;
			intensity = corner[j].intensity;
			*(__IO uint16_t*)(pDestination + stepdir * -1 * (y + (x * hgfx->ImageHeight))) = (intensity << 8) + color;
			*(__IO uint16_t*)(pDestination + stepdir * -1 * (x + (y * hgfx->ImageHeight))) = (intensity << 8) + color;
		}
	}
}


/**
  ******************************************************************************
  * @brief   GFX write label. /  Write string with defined color
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    07-July-2014
  ******************************************************************************
	*
  * @param  hgfx: 			check gfx_engine.h.
  * @param  color: 			16bit Alpha+CLUT.
  * @retval None
  */

uint32_t GFX_write_label(const tFont *Font, GFX_DrawCfgWindow* hgfx, const char *pText, uint8_t color)
{
	return GFX_write_string_color(Font, hgfx, pText, 0, color);
}


/**
  ******************************************************************************
  * @brief   GFX writeGfx_write_label_varstring. /  Write string with all parameters and font color options
	heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  XleftGimpStyle:
  * @param  XrightGimpStyle:
  * @param  YtopGimpStyle:
  * @param  color:
  * @param  tFont:
  * @param  text: 	text to be printed
  * @retval None
  */

void Gfx_write_label_var(GFX_DrawCfgScreen *screenInput, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const uint8_t color, const char *text)
{

    GFX_DrawCfgWindow	hgfx;


	SSettings* pSettings;
	pSettings = settingsGetPointer();


	if(XrightGimpStyle > 799)
		XrightGimpStyle = 799;
	if(XleftGimpStyle >= XrightGimpStyle)
		XleftGimpStyle = 0;
	if(YtopGimpStyle > 479)
		YtopGimpStyle = 479;
    hgfx.Image = screenInput;
	hgfx.WindowNumberOfTextLines = 1;
	hgfx.WindowLineSpacing = 0;
	hgfx.WindowTab = 0;

	if(!pSettings->FlipDisplay)
	{
		hgfx.WindowX0 = XleftGimpStyle;
		hgfx.WindowX1 = XrightGimpStyle;
		hgfx.WindowY1 = 479 - YtopGimpStyle;
		if(hgfx.WindowY1 < Font->height)
			hgfx.WindowY0 = 0;
		else
			hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
	}
	else
	{
		hgfx.WindowX0 = 800 - XrightGimpStyle;
		hgfx.WindowX1 = 800 - XleftGimpStyle;
		hgfx.WindowY0 = YtopGimpStyle;
		if(hgfx.WindowY0 + Font->height > 480)
			hgfx.WindowY1 = 480;
		else
			hgfx.WindowY1 = hgfx.WindowY0 + Font->height;
	}
	GFX_write_label(Font, &hgfx, text, color);
}

/**
  ******************************************************************************
  * @brief   GFX write string. /  Write string with all parameters and font options
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  hgfx: 			check gfx_engine.h.
  * @param  color: 			32bit ARGB8888.
  * @retval None
  */

uint16_t GFX_return_offset(const tFont *Font, char *pText, uint8_t position)
{
	char character;
	uint16_t digit, i;
	uint8_t found;
	uint16_t distance;

	if(position == 0)
		return 0;

	distance = 0;
	for(digit = 0; digit < position; digit++)
	{
		character = pText[digit];
		if(character == 0)
			return 0;

		found = 0;
		for(i=0;i<Font->length;i++)
		{
			if(Font->chars[i].code == character)
			{
				found = 1;
				break;
			}
		}
		if(found)
		{
			distance += (uint16_t)(Font->chars[i].image->width);
			if(Font == &FontT144)
				distance += 3;
			else
			if(Font == &FontT105)
				distance += 2;
		}
	}
	return distance;

	/*	 FEHLT:
	if(*pText < ' ')
	if((*pText) & 0x80)

	if(((tFont *)settings.font == &FontT105) && settings.dualFont && ((*pText == '.') || (*pText == ':')))
		settings.font = (uint32_t)&FontT54;
	*/
}

void GFX_clean_line(GFX_DrawCfgWindow* hgfx, uint32_t line_number)
{
	uint16_t height;
	uint32_t pDestination, i, j;
	uint16_t left, width, bottom, nextlineStep;

	bottom = hgfx->WindowY0;

	if(hgfx->WindowNumberOfTextLines && line_number && (line_number <= hgfx->WindowNumberOfTextLines))
	{
		bottom += hgfx->WindowLineSpacing * (hgfx->WindowNumberOfTextLines - line_number);
		height = hgfx->WindowLineSpacing;
	}
	else
	{
		height 	= 1 + hgfx->WindowY1 - bottom;
	}

	pDestination = (uint32_t)hgfx->Image->FBStartAdress;

	left 		= hgfx->WindowX0;
	width 	= 1 + hgfx->WindowX1 - left;
	nextlineStep = hgfx->Image->ImageHeight - height;
	nextlineStep *= 2;
	pDestination += 2 * bottom;
	pDestination += 2 * hgfx->Image->ImageHeight * left;

	for(j = width; j > 0; j--)
	{
		for(i = height; i > 0; i--)
		{
			*(__IO uint16_t*)pDestination = 0;
			pDestination += 2;
		}
		pDestination += nextlineStep;
	}
}


void GFX_clean_area(GFX_DrawCfgScreen *tMscreen, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, uint16_t YBottomGimpStyle)
{
	uint16_t height;
	uint32_t pDestination, i, j;
	int32_t left, width, bottom, nextlineStep;

	bottom = tMscreen->ImageHeight - YBottomGimpStyle;
	height = 1 + YBottomGimpStyle - YtopGimpStyle;

	if(bottom < 0)
		bottom = 0;
	if(height > tMscreen->ImageHeight)
		height = tMscreen->ImageHeight;
	
	pDestination = tMscreen->FBStartAdress;

	left 		= XleftGimpStyle;
	width 	= 1 + XrightGimpStyle - left;
	if(width < 1)
		width = 1;

	if(width > tMscreen->ImageWidth)
		width = tMscreen->ImageWidth;

	nextlineStep = tMscreen->ImageHeight - height;
	nextlineStep *= 2;
	pDestination += 2 * bottom;
	pDestination += 2 * tMscreen->ImageHeight * left;

	for(j = width; j > 0; j--)
	{
		for(i = height; i > 0; i--)
		{
			*(__IO uint16_t*)pDestination = 0;
			pDestination += 2;
		}
		pDestination += nextlineStep;
	}
}


uint32_t GFX_write_string(const tFont *Font, GFX_DrawCfgWindow* hgfx, const char *pText, uint32_t line_number)
{
	return GFX_write_string_color(Font, hgfx, pText, line_number, 0);
}

uint32_t GFX_write_string_color(const tFont *Font, GFX_DrawCfgWindow* hgfx, const char *pText, uint32_t line_number, uint8_t color)
{
	if(hgfx->Image->FBStartAdress < FBGlobalStart)
		return 0;

	GFX_CfgWriteString settings;
	uint32_t newXdelta;
	uint8_t minimal = 0;
//	uint32_t try_again;

	if(hgfx->WindowNumberOfTextLines && line_number && (line_number <= hgfx->WindowNumberOfTextLines))
	{
		settings.Ydelta = hgfx->WindowLineSpacing * (hgfx->WindowNumberOfTextLines - line_number);
	}
	else
	{
		settings.Ydelta = 0;
	}
	settings.font = (uint32_t)Font;
	settings.Xdelta = 0;
	settings.color = color;
	settings.invert = 0;
	settings.resize = 0;
	settings.dualFont = 0;
	settings.spaceMode = 0;
	settings.singleSpaceWithSizeOfNextChar = 0;
	settings.useTinyFont = 0;
	settings.TinyFontExtraYdelta = 0;
	settings.TinyFont = (uint32_t)Font;
	settings.doubleSize = 0;

	if((*pText) == TXT_MINIMAL) // for customtext and anything with Sonderzeichen
		minimal = 1;
	else
		minimal = 0;

	if(Font == &FontT144)
		settings.TinyFont = (uint32_t)&FontT84;
	else
	if(Font == &FontT105)
		settings.TinyFont = (uint32_t)&FontT54;
	else
	if(Font == &FontT54)
	{
		settings.TinyFont = (uint32_t)&FontT48;
		settings.TinyFontExtraYdelta = -9;
	}
	else
	if(Font == &FontT48)
	{
		settings.TinyFont = (uint32_t)&FontT24;
		settings.TinyFontExtraYdelta = 6;
	}
	else
	if(Font == &FontT42)
	{
		settings.TinyFont = (uint32_t)&FontT24;
		settings.TinyFontExtraYdelta = 2;
	}

	settings.actualFont = (tFont *)settings.font;

	while ((*pText != 0) && (settings.Xdelta != 0x0000FFFF))// und fehlend: Abfrage window / image size
	{
//		try_again = 0;

		if((*pText == '\177') && !minimal)
		{
			if(settings.singleSpaceWithSizeOfNextChar)
			{
				settings.singleSpaceWithSizeOfNextChar = 0;
				pText++;
				settings.Xdelta += *pText;
			}
			else
				settings.singleSpaceWithSizeOfNextChar = 1;
		}
		else
		if(*pText < ' ')
		{
			/* Xdelta -inline- changes */
			if((*pText == '\t') && !minimal)
				settings.Xdelta = hgfx->WindowTab - hgfx->WindowX0;
			else
			if(*pText == '\r') // carriage return, no newline
				settings.Xdelta = 0;
			else
			if((*pText == '\001')) // center
				settings.Xdelta = GFX_write__Modify_Xdelta__Centered(&settings, hgfx, pText+1);
			else
			if((*pText == '\002')) // right
				settings.Xdelta = GFX_write__Modify_Xdelta__RightAlign(&settings, hgfx, pText+1);
			else
			if((*pText == '\003') && !minimal) // doubleSize
				settings.doubleSize = 1;
			else
			/* Xdelta -up/down changes */
			if((*pText == '\f') && !minimal) // form feed = top align
			{
					if((hgfx->WindowY1 - hgfx->WindowY0) >= ((tFont *)settings.font)->height)
					{
						settings.Ydelta = hgfx->WindowY1 - hgfx->WindowY0;
						settings.Ydelta -= ((tFont *)settings.font)->height;
					}
			}
			else
			if(*pText == '\n') // newline, no carriage return
			{
				if(hgfx->WindowNumberOfTextLines && (line_number < hgfx->WindowNumberOfTextLines))
				{
					line_number++;
					settings.Ydelta = hgfx->WindowLineSpacing * (hgfx->WindowNumberOfTextLines - line_number);
				}
			}
			else
			/* Font style changes */
			if(*pText == '\a')
				settings.invert = settings.invert ? 0 : 1;
			else
			if((*pText == '\016') && !minimal)
			{
				if(settings.dualFont == 0)
					settings.dualFont = 1;
				else
					settings.actualFont = (tFont *)settings.TinyFont;
			}
			else
			if((*pText == '\017') && !minimal)
			{
				settings.dualFont = 0;
				settings.actualFont = (tFont *)settings.font;
			}
			else
#ifndef BOOTLOADER_STANDALONE
			if((*pText == '\005') && !minimal)
			{
				newXdelta = GFX_write_char(hgfx, &settings, 'a', (tFont *)&Awe48);
				settings.Xdelta = newXdelta;
			}
			else
			if((*pText == '\006') && !minimal)
			{
				newXdelta = GFX_write_char(hgfx, &settings, 'b', (tFont *)&Awe48);
				settings.Xdelta = newXdelta;
			}
			else
#endif
			if((*pText >= '\020') && (*pText <= '\032') && !minimal)
				settings.color = *pText - '\020';
			else
			if((*pText == '\034') && !minimal)
				settings.spaceMode = 1;
			else
			if((*pText == '\035') && !minimal)
				settings.spaceMode = 0;
		}
		else
		if(((*pText) == TXT_2BYTE) && !minimal)
		{
			pText++;
			settings.Xdelta = GFX_write_substring(&settings, hgfx, (uint8_t)TXT_2BYTE, (int8_t)*pText);
		}
		else
		if(((*pText) & 0x80) && !minimal)
			settings.Xdelta = GFX_write_substring(&settings, hgfx, (uint8_t)*pText, 0);
		else
		if(!settings.invert && (*pText == ' '))
		{
			if(settings.spaceMode == 0)
				settings.Xdelta += ((tFont *)settings.font)->spacesize;
			else
				settings.Xdelta += ((tFont *)settings.font)->spacesize2Monospaced;
		}
		else
		if((settings.spaceMode == 1) && (*pText == ' '))
				settings.Xdelta += ((tFont *)settings.font)->spacesize2Monospaced;
		else
		{
			if(((tFont *)settings.font == &FontT144) && ((*pText == '.') || (*pText == ':')))
					settings.actualFont = (tFont *)settings.TinyFont;
			else
			if(((tFont *)settings.font == &FontT105) && settings.dualFont && ((*pText == '.') || (*pText == ':')))
					settings.actualFont = (tFont *)settings.TinyFont;

			if(settings.actualFont == (tFont *)settings.TinyFont)
				settings.Ydelta += settings.TinyFontExtraYdelta;

			newXdelta = GFX_write_char(hgfx, &settings, *(uint8_t *)pText, settings.actualFont);
			settings.Xdelta = newXdelta;

			if(settings.actualFont == (tFont *)settings.TinyFont)
				settings.Ydelta -= settings.TinyFontExtraYdelta;
		}
		if(pText != 0) /* for TXT_2BYTE */
			pText++;
	}
	return settings.Ydelta;
}

/* Private functions ---------------------------------------------------------*/
/******************************************************************************
                            Static Function
*******************************************************************************/

/**
  ******************************************************************************
  * @brief   GFX write substring. /  Write string without parameters
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  hgfx: 			check gfx_engine.h.
  * @param  color: 			32bit ARGB8888.
  * @retval None
  */

static uint32_t GFX_write_substring(GFX_CfgWriteString* cfg, GFX_DrawCfgWindow* hgfx, uint8_t textId, int8_t nextCharFor2Byte)
{
	uint8_t i, j;
	uint32_t found;
	uint32_t pText;
	uint16_t decodeUTF8;
#ifndef BOOTLOADER_STANDALONE
	uint8_t gfx_selected_language = 0;
	SSettings *pSettings;
	pSettings = settingsGetPointer();
	gfx_selected_language = pSettings->selected_language;
	if(gfx_selected_language >= LANGUAGE_END) gfx_selected_language = 0;
#endif		

// -----------------------------
 	if(textId != (uint8_t)TXT_2BYTE)
	{
		found = 0;
		j = 0;
		for(i=(uint8_t)TXT_Language;i<(uint8_t)TXT_END;i++)
		{
#ifndef BOOTLOADER_STANDALONE
			if(text_array[j].code == textId)
			{
				found = 1;
				break;
			}
#endif
			j++;
		}
		if(!found)
			return cfg->Xdelta;

// -----------------------------
#ifndef BOOTLOADER_STANDALONE
		pText = (uint32_t)text_array[j].text[gfx_selected_language];
		if(!pText)
			pText = (uint32_t)text_array[j].text[0];
		else
		if(*(char*)pText == 0)
			pText = (uint32_t)text_array[j].text[0];
#endif
	}
// -----------------------------
	else
	{
		if(!nextCharFor2Byte)
			return cfg->Xdelta;
		
		found = 0;
		for(j=0;j<(uint8_t)TXT2BYTE_END-(uint8_t)TXT2BYTE_START;j++)
		{
#ifndef BOOTLOADER_STANDALONE
			if((uint8_t)text_array2[j].code == (uint8_t)nextCharFor2Byte)
			{
				found = 1;
				break;
			}
#endif
		}
		if(!found)
			return cfg->Xdelta;
#ifndef BOOTLOADER_STANDALONE
// -----------------------------
		pText = (uint32_t)text_array2[j].text[gfx_selected_language];
		if(!pText)
			pText = (uint32_t)text_array2[j].text[0];
		else
		if(*(char*)pText == 0)
			pText = (uint32_t)text_array2[j].text[0];
#endif
	}
// -----------------------------
	
	if(cfg->actualFont == (tFont *)cfg->TinyFont)
		cfg->Ydelta += cfg->TinyFontExtraYdelta;

	while (*(char*)pText != 0)// und fehlend: Abfrage window / image size
	{
		if(*(char*)pText == '\t')
			cfg->Xdelta = hgfx->WindowTab - hgfx->WindowX0;
		else
		if((*(char*)pText == ' ') && (cfg->invert == 0))	/* bypass drawing of white space only for not inverted mode */
		{
			cfg->Xdelta += ((tFont *)cfg->actualFont)->spacesize;
		}
		else
		if((*(char*)pText) & 0x80) /* Identify a UNICODE character other than standard ASCII using the highest bit */
		{
			decodeUTF8 = ((*(char*)pText) & 0x1F) << 6; /* use 5bits of first byte for upper part of unicode */
			pText++;
			decodeUTF8 |= (*(char*)pText) & 0x3F; /* add lower 6bits as second part of the unicode */
			if (decodeUTF8 <= 0xff) /* The following function has a uint8 input parameter ==> no UNICODEs > 0xff supported */
			{
				cfg->Xdelta = GFX_write_char(hgfx, cfg, (uint8_t)decodeUTF8, (tFont *)cfg->actualFont);
			}
		}
		else
			cfg->Xdelta = GFX_write_char(hgfx, cfg, *(uint8_t *)pText, (tFont *)cfg->actualFont);

    pText++;
	}

	if(cfg->actualFont == (tFont *)cfg->TinyFont)
		cfg->Ydelta -= cfg->TinyFontExtraYdelta;

	return cfg->Xdelta;
}


/**
  ******************************************************************************
  * @brief   GFX write char. /  Write non-inverted, non-colored with entire 8 bit range
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  hgfx: 			check gfx_engine.h.
  * @param  Ydelta: 		input
  * @param  character: 	character
  * @param  *Font:		 	pointer to font to be used for this char
	* @retval Ydelta:			0x0000FFFF if not successful or char_truncated
  */

static uint32_t GFX_write_char_doubleSize(GFX_DrawCfgWindow* hgfx, GFX_CfgWriteString* cfg, uint8_t character, tFont *Font)
{
	uint32_t i, j;
	uint32_t width, height;
	uint32_t found;
	uint16_t* pDestination;
	uint32_t pSource;
	uint32_t OffsetDestination;
	uint32_t width_left;
	uint32_t height_left;
	uint32_t char_truncated_WidthFlag;
	uint32_t char_truncated_Height;
	uint8_t fill;
	uint32_t widthFont, heightFont;
	uint32_t nextLine;
	int32_t stepdir;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(pSettings->FlipDisplay)
	{
		stepdir = -1;	/* decrement address while putting pixels */
	}
	else
	{
		stepdir = 1;
	}


	if(hgfx->Image->ImageWidth <= (hgfx->WindowX0 + cfg->Xdelta))
		return 0x0000FFFF;

	// -----------------------------
	found = 0;
	for(i=0;i<Font->length;i++)
	{
		if(Font->chars[i].code == character)
		{
			found = 1;
			break;
		}
	}
	if(!found)
		return cfg->Xdelta;

	pSource = ((uint32_t)Font->chars[i].image->data);
	pDestination = (uint16_t*)(hgfx->Image->FBStartAdress);

	heightFont = Font->chars[i].image->height;
	widthFont = Font->chars[i].image->width;

	height = heightFont*2;
	width = widthFont*2;


	if(pSettings->FlipDisplay)
	{
		pDestination += (uint32_t)(hgfx->WindowX1 - cfg->Xdelta) * hgfx->Image->ImageHeight; /* set pointer to delta row */
		pDestination += (hgfx->WindowY1 - cfg->Ydelta);							   /* set pointer to delta colum */
	}
	else
	{
		pDestination += (uint32_t)(hgfx->WindowX0 + cfg->Xdelta) * hgfx->Image->ImageHeight;  /* set pointer to delta row */
		pDestination += (hgfx->WindowY0 + cfg->Ydelta);							   /* set pointer to delta colum */
	}
	OffsetDestination = (hgfx->Image->ImageHeight - height);
	nextLine = hgfx->Image->ImageHeight;

// -----------------------------
	char_truncated_WidthFlag = 0;
	if(!pSettings->FlipDisplay)
	{
		width_left = hgfx->Image->ImageWidth - (hgfx->WindowX0 + cfg->Xdelta);
	}
	else
	{
		width_left = (hgfx->WindowX1 - cfg->Xdelta);
	}
	if(width_left < width)
	{
		char_truncated_WidthFlag = 1;
		width = width_left;
		widthFont = width/2;
	}
// -----------------------------

	char_truncated_Height = 0;
	height_left = hgfx->Image->ImageHeight - (hgfx->WindowY0 + cfg->Ydelta);
	if(height_left < height)
	{
		char_truncated_Height = height - height_left;
		if((char_truncated_Height & 1) != 0)
		{
			height_left -= 1;
			char_truncated_Height += 1;
		}
		height = height_left;
		heightFont = height/2;
	}

	OffsetDestination += char_truncated_Height;
// -----------------------------
	if(height == 0)
		return 0x0000FFFF;
// -----------------------------

	if(cfg->singleSpaceWithSizeOfNextChar)
	{
		cfg->singleSpaceWithSizeOfNextChar = 0;
 
		if(cfg->invert)
			fill = 0xFF;
		else
			fill = 0;

		height /= 2;
		for(i = width; i > 0; i--)
		{
			for (j = height; j > 0; j--)
			{
				*(__IO uint16_t*)pDestination =  fill << 8 | cfg->color;
				pDestination += stepdir;
				*(__IO uint16_t*)pDestination =  fill << 8 | cfg->color;
				pDestination += stepdir;
			}
			pDestination += stepdir * OffsetDestination;
		}
	}
	else
	if(cfg->invert)
	{
		if((heightFont & 3) == 0) /* unroll for perfomance, by 4 if possible, by 2 (16bit) otherwise */
		{
			heightFont /= 4;
			for(i = widthFont; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{
					for (j = heightFont; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;
					}
					pSource += char_truncated_Height;
				}
				else
				{
					pSource++;
					for (j = heightFont; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
					}
				}
				pDestination += (OffsetDestination + nextLine) * stepdir;
			}
		}
		else
		{
			heightFont /= 2;
			for(i = widthFont; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{
					for (j = heightFont; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  (0xFF - *(uint8_t*)pSource) << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;
					}
					pSource += char_truncated_Height;
				}
				else
				{
					pSource++;
					for (j = heightFont; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + nextLine) =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
					}
				}
				pDestination += (OffsetDestination + nextLine) * stepdir;
			}
		}
	} /* inverted */
	else
	{
		if((heightFont & 3) == 0) /* unroll for perfomance, by 4 if possible, by 2 (16bit) otherwise */
		{
			heightFont /= 4;
			for(i = widthFont; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{
					for (j = heightFont; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;
					}
					pSource += char_truncated_Height;
				}
				else
				{
					pSource++;
					pDestination +=  stepdir * height;
				}
				pDestination += stepdir * (OffsetDestination + nextLine);
			}
		}
		else
		{
			heightFont /= 2;
			for(i = widthFont; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{
					for (j = heightFont; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;

						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   *(uint8_t*)pSource << 8 | cfg->color;
						*(__IO uint16_t*)(pDestination + (stepdir * nextLine)) =   *(uint8_t*)pSource << 8 | cfg->color;
						pDestination += stepdir;
						pSource++;
					}
					pSource += char_truncated_Height;
				}
				else
				{
					pSource++;
					pDestination +=  stepdir * height;
				}
				pDestination += stepdir * (OffsetDestination + nextLine);
			}
		}
	}

// -----------------------------

	if(Font == &FontT144)
		width += 6;
	else
	if(Font == &FontT105)
		width += 4;

// -----------------------------

	if(char_truncated_WidthFlag)
		return 0x0000FFFF;
	else
		return cfg->Xdelta + width;
	
}


/**
  ******************************************************************************
  * @brief   GFX write char. /  Write non-inverted, non-colored with entire 8 bit range
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  hgfx: 			check gfx_engine.h.
  * @param  Ydelta: 		input
  * @param  character: 	character
  * @param  *Font:		 	pointer to font to be used for this char
	* @retval Ydelta:			0x0000FFFF if not successful or char_truncated
  */

static uint32_t GFX_write_char(GFX_DrawCfgWindow* hgfx, GFX_CfgWriteString* cfg, uint8_t character, tFont *Font)
{

#ifndef BOOTLOADER_STANDALONE
	Font = GFX_Check_Extra_Font(character, Font);
#endif
	if(cfg->doubleSize)
	{
		return GFX_write_char_doubleSize(hgfx, cfg, character, Font);
	}
	
	uint32_t i, j;
	uint32_t width, height;
	uint32_t found;
	uint16_t* pDestination;
	uint32_t pSource;
	uint32_t OffsetDestination;
	uint32_t width_left;
	uint32_t height_left;
	uint32_t char_truncated_WidthFlag;
	uint32_t char_truncated_Height;
	uint8_t fill;
	uint32_t fillpattern;
	int16_t stepdir;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(pSettings->FlipDisplay)
	{
		stepdir = -1;	/* decrement address while putting pixels */
	}
	else
	{
		stepdir = 1;
	}

	if(hgfx->Image->ImageWidth <= (hgfx->WindowX0 + cfg->Xdelta))
		return 0x0000FFFF;

	// -----------------------------
	found = 0;
	for(i=0;i<Font->length;i++)
	{
		if(Font->chars[i].code == character)
		{
			found = 1;
			break;
		}
	}
	if(!found)
		return cfg->Xdelta;
// -----------------------------
/*
	if(Font == &Font144)
		cfg->Xdelta += 3;
	else
	if(Font == &Font84)
		cfg->Xdelta += 2;
*/
// -----------------------------


	pSource = ((uint32_t)Font->chars[i].image->data);
	pDestination = (uint16_t*)(hgfx->Image->FBStartAdress);


	height = Font->chars[i].image->height;
	width = Font->chars[i].image->width;

	OffsetDestination = hgfx->Image->ImageHeight - height;


	/* Xyyyyy   y= height */
	/* Xyyyyy   x= width  */
	/* Xyyyyy             */

	if(pSettings->FlipDisplay)
	{
		pDestination += (hgfx->WindowX1 - cfg->Xdelta) * hgfx->Image->ImageHeight; /* set pointer to delta row */
		pDestination += (hgfx->WindowY1 - cfg->Ydelta);							   /* set pointer to delta colum */
	}
	else
	{
		pDestination += (hgfx->WindowX0 + cfg->Xdelta) * hgfx->Image->ImageHeight; /* set pointer to delta row */
		pDestination += (hgfx->WindowY0 + cfg->Ydelta);							   /* set pointer to delta colum */
	}


// -----------------------------
	char_truncated_WidthFlag = 0;
	if(!pSettings->FlipDisplay)
	{
		width_left = hgfx->Image->ImageWidth - (hgfx->WindowX0 + cfg->Xdelta);
	}
	else
	{
		width_left = (hgfx->WindowX1 - cfg->Xdelta);
	}
	if(width_left < width)
	{
		char_truncated_WidthFlag = 1;
		width = width_left;
	}
// -----------------------------
	char_truncated_Height = 0;
	if(!pSettings->FlipDisplay)
	{
		height_left = hgfx->Image->ImageHeight - (hgfx->WindowY0 + cfg->Ydelta);
	}
	else
	{
		height_left = (hgfx->WindowY1 - cfg->Ydelta);
	}
	if(height_left < height)
	{
		char_truncated_Height = height - height_left;
		if((char_truncated_Height & 1) != 0)
		{
			height_left -= 1;
			char_truncated_Height += 1;
		}
		height = height_left;
	}
	OffsetDestination += char_truncated_Height;
// -----------------------------
	if(height == 0)
		return 0x0000FFFF;
// -----------------------------
	
	if(cfg->singleSpaceWithSizeOfNextChar)
	{
		cfg->singleSpaceWithSizeOfNextChar = 0;
 
		if(cfg->invert)
			fill = 0xFF;
		else
			fill = 0;

		height /= 2;
		for(i = width; i > 0; i--)
		{
			for (j = height; j > 0; j--)
			{
				*(__IO uint16_t*)pDestination =  fill << 8 | cfg->color;
				pDestination += stepdir;
				*(__IO uint16_t*)pDestination =  fill << 8 | cfg->color;
				pDestination += stepdir;
			}
			pDestination += stepdir * OffsetDestination;
		}
	}
	else
	if(cfg->invert)
	{
		if((height & 3) == 0) /* unroll for perfomance, by 4 if possible, by 2 (16bit) otherwise */
		{
			height /= 4;
			for(i = width; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{

					for (j = height; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =   (0xFF - *(uint8_t*)pSource++) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   (0xFF - *(uint8_t*)pSource++) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   (0xFF - *(uint8_t*)pSource++) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   (0xFF - *(uint8_t*)pSource++) << 8 | cfg->color;
						pDestination += stepdir;
					}
					pSource += char_truncated_Height;
				}
				else /* empty line => fast fill */
				{
					pSource++;
					fillpattern = (( 0xFF << 8 | cfg->color) << 16) | ( 0xFF << 8 | cfg->color);
					if(pSettings->FlipDisplay) pDestination--; /* address fill from high to low */
					for (j = height; j > 0; j--)
					{
						*(__IO uint32_t*)pDestination =  fillpattern;
						pDestination += stepdir;
						pDestination += stepdir;
						*(__IO uint32_t*)pDestination =  fillpattern;
						pDestination += stepdir;
						pDestination += stepdir;
					}
					if(pSettings->FlipDisplay) pDestination++;
				}
				pDestination += stepdir * OffsetDestination;
			}
		}
		else
		{
			height /= 2;
			for(i = width; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{
					for (j = height; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =   (0xFF - *(uint8_t*)pSource++) << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =   (0xFF - *(uint8_t*)pSource++) << 8 | cfg->color;
						pDestination += stepdir;
					}
					pSource += char_truncated_Height;
				}
				else
				{
					pSource++;
					for (j = height; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  0xFF << 8 | cfg->color;
						pDestination += stepdir;
					}
				}
				pDestination += stepdir * OffsetDestination;
			}
		}
	}
	else  /* not inverted */
	{
		if((height & 3) == 0) /* unroll for perfomance, by 4 if possible, by 2 (16bit) otherwise */
		{

			height /= 4;

			for(i = width; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{
					for (j = height; j > 0; j--)
					{
							*(__IO uint16_t*)pDestination =  ( *(uint8_t*)pSource++ << 8) | (cfg->color);
							pDestination += stepdir;
							*(__IO uint16_t*)pDestination =  ( *(uint8_t*)pSource++ << 8) | (cfg->color);
							pDestination += stepdir;
							*(__IO uint16_t*)pDestination =  ( *(uint8_t*)pSource++ << 8) | (cfg->color);
							pDestination += stepdir;
							*(__IO uint16_t*)pDestination =  ( *(uint8_t*)pSource++ << 8) | (cfg->color);
							pDestination += stepdir;
					}

					pSource += char_truncated_Height;
				}
				else  /* clear line */
				{
					pSource++;
					fillpattern = (cfg->color << 16) | cfg->color;
					if(pSettings->FlipDisplay) pDestination--; /* address fill from high to low */

					for (j = height; j > 0; j--)
					{
						*(__IO uint32_t*)pDestination =  fillpattern;
						pDestination += stepdir;
						pDestination += stepdir;
						*(__IO uint32_t*)pDestination =  fillpattern;
						pDestination += stepdir;
						pDestination += stepdir;
					}
					if(pSettings->FlipDisplay) pDestination++;
				}
				pDestination += stepdir * OffsetDestination;
			}
		}
		else
		{
			height /= 2;
			for(i = width; i > 0; i--)
			{
				if(*(uint8_t*)pSource != 0x01)
				{
					for (j = height; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =  ( *(uint8_t*)pSource++ << 8) | (cfg->color);
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  ( *(uint8_t*)pSource++ << 8) | (cfg->color);
						pDestination += stepdir;
					}
					pSource += char_truncated_Height;
				}
				else /* clear line */
				{
					pSource++;
					for (j = height; j > 0; j--)
					{
						*(__IO uint16_t*)pDestination =  cfg->color;
						pDestination += stepdir;
						*(__IO uint16_t*)pDestination =  cfg->color;
						pDestination += stepdir;
					}
				}
				pDestination += stepdir * OffsetDestination;
			}
		}
	}

// -----------------------------

	if(Font == &FontT144)
		width += 3;
	else
	if(Font == &FontT105)
		width += 2;
/*
	else
	if(Font == &Font144)
		width += 3;
	else
	if(Font == &Font84)
		width += 1;
*/
// -----------------------------

	if(char_truncated_WidthFlag)
		return 0x0000FFFF;
	else
		return cfg->Xdelta + width;
}

#ifndef BOOTLOADER_STANDALONE

/**
  ******************************************************************************
  * @brief   GFX write Modify helper for center and right align.
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    17-March-2015
  ******************************************************************************
	*
  * @param  *cText: 			output
  * @param  *pTextInput:	input
	* @param  gfx_selected_language: 	gfx_selected_language
	* @retval counter and *cText content
  */
static int8_t GFX_write__Modify_helper(char *cText, const char *pTextInput, uint8_t gfx_selected_language)
{
	uint32_t pText, backup;
	uint8_t textId;
	int8_t counter;
	uint32_t found;
	uint32_t j;
	
	pText = (uint32_t)pTextInput;
	counter = 0;
	while((counter < 100) && (*(char*)pText != 0) && (*(char*)pText != '\r') && (*(char*)pText != '\n'))
	{
		if((*(char*)pText) == TXT_2BYTE)
		{
			backup = pText;
			
			found = 0;
			j = 0;
			textId = (int8_t)*(char*)(pText + 1);
			if(textId != 0)
			{
				for(j=0;j<(uint8_t)TXT2BYTE_END-(uint8_t)TXT2BYTE_START;j++)
				{
					if((uint8_t)text_array2[j].code == (uint8_t)textId)
					{
						found = 1;
						break;
					}
				}
				if(found)
				{
						pText = (uint32_t)text_array2[j].text[gfx_selected_language];
						if(!pText)
							pText = (uint32_t)text_array2[j].text[0];
						else
						if(*(char*)pText == 0)
							pText = (uint32_t)text_array2[j].text[0];

						while((counter < 100) && (*(char*)pText != 0))
							cText[counter++] = *(char*)pText++; 
				}	
				pText = backup + 2;
			}
			else
				pText = 0;
		}
		if(0 != pText && ((*(char*)pText) & 0x80))
		{
			backup = pText;
			
			found = 0;
			j = 0;
			textId = (uint8_t)*(char*)pText;
			for(uint8_t ii=(uint8_t)TXT_Language;ii<(uint8_t)TXT_END;ii++)
			{
				if(text_array[j].code == textId)
				{
					found = 1;
					break;
				}
				j++;
			}
			if(found)
			{
					pText = (uint32_t)text_array[j].text[gfx_selected_language];
					if(!pText)
						pText = (uint32_t)text_array[j].text[0];
					else
					if(*(char*)pText == 0)
						pText = (uint32_t)text_array[j].text[0];

					while((counter < 100) && (*(char*)pText != 0))
						cText[counter++] = *(char*)pText++; 
			}	
			pText = backup + 1;
		}
		else
		{
			cText[counter++] = *(char*)pText++; 
		}
	}
	cText[counter] = 0;
	return counter;
}

#endif
/**
  ******************************************************************************
  * @brief   GFX write Modify Ydelta for align. /  calc Ydelta for start
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  *hgfx: 		check gfx_engine.h.
  * @param  *cfg:		 	Ydelta, Font
  * @param  *pText: 	character
	* @retval Ydelta:		0 if text has to start left ( and probably does not fit)
  */

static uint32_t GFX_write__Modify_Xdelta__Centered(GFX_CfgWriteString* cfg, GFX_DrawCfgWindow* hgfx, const char *pTextInput)
{
	char cText[101];
	uint32_t result;
	uint32_t Xsum;
	uint32_t j;
	uint32_t pText;
	uint16_t decodeUTF8;
	uint8_t tinyState = 0;		/* used to identify the usage of tiny font */
	tFont* ptargetFont;

#ifndef BOOTLOADER_STANDALONE
	uint8_t gfx_selected_language = 0;
	SSettings *pSettings;
	pSettings = settingsGetPointer();
	gfx_selected_language = pSettings->selected_language;
	if(gfx_selected_language >= LANGUAGE_END)
#endif
// -----------------------------
#ifndef BOOTLOADER_STANDALONE
	GFX_write__Modify_helper(cText,pTextInput,gfx_selected_language);
#endif
	pText = (uint32_t)&cText[0];
	Xsum = 0;
	j = 0;
	ptargetFont = (tFont *)cfg->font;
	while (*(char*)pText != 0)// und fehlend: Abfrage window / image size
	{
		if(*(char*)pText == '\016')	/* request font change */
		{
			tinyState++;
		}
		if(*(char*)pText == '\017')	/* request font reset */
		{
			tinyState = 0;
		}

		if((ptargetFont == &FontT105) && ((*(char*)pText == '.') || (*(char*)pText == ':')))
		{
			tinyState++;
		}

		if(tinyState > 1)
		{
			ptargetFont = (tFont *)cfg->TinyFont;
		}
		else
		{
			ptargetFont = (tFont *)cfg->font;
		}

		decodeUTF8 = *(char*)pText; /* place ASCII char */
		if((*(char*)pText == '\005') || (*(char*)pText == '\006'))
		{
			Xsum += 45;
		}
		else
		{
			if((*(char*)pText) & 0x80) /* Identify a UNICODE character other than standard ASCII using the highest bit */
			{
				decodeUTF8 = ((*(char*)pText) & 0x1F) << 6; /* use 5bits of first byte for upper part of unicode */
				pText++;
				decodeUTF8 |= (*(char*)pText) & 0x3F; /* add lower 6bits as second part of the unicode */
			}
			else
			{
				decodeUTF8 = *(char*)pText; /* place ASCII char */
			}
			Xsum += GFX_Character_Width(decodeUTF8, ptargetFont);
		}

		pText++;
		j++;
		if((ptargetFont == &FontT144) && (*(char*)pText != 0))
			Xsum += 3;
		else
		if((ptargetFont == &FontT105) && (*(char*)pText != 0))
			Xsum += 2;
	}
	pText -= j;

	if(cfg->doubleSize)
		Xsum *= 2;
	
	result = hgfx->WindowX1 - hgfx->WindowX0;
	if(Xsum < result)
	{
		result -= Xsum;
		result /= 2;
	}
	else
		result = 0;
	return result;
}


static uint32_t GFX_write__Modify_Xdelta__RightAlign(GFX_CfgWriteString* cfg, GFX_DrawCfgWindow* hgfx, const char *pTextInput)
{
	uint32_t result;
	uint32_t Xsum;
	uint32_t j;
	tFont *font;
	char cText[101];
	uint32_t pText;
	uint16_t decodeUTF8;
	uint8_t tinyState = 0;		/* used to identify the usage of tiny font */

#ifndef BOOTLOADER_STANDALONE
	uint8_t gfx_selected_language = 0;
	SSettings *pSettings;
	pSettings = settingsGetPointer();
	gfx_selected_language = pSettings->selected_language;
	if(gfx_selected_language >= LANGUAGE_END)
#else
	cText[0] = 0;
#endif
// -----------------------------
#ifndef BOOTLOADER_STANDALONE
	GFX_write__Modify_helper(cText,pTextInput,gfx_selected_language);
#endif
	pText = (uint32_t)&cText[0];
// -----------------------------

	font = (tFont *)cfg->font;
	Xsum = 0;
	j = 0;

	while (*(char*)pText != 0)// und fehlend: Abfrage window / image size
	{
		if(*(char*)pText == '\016')	/* request font change */
		{
			tinyState++;
		}
		if(*(char*)pText == '\017')	/* request font reset */
		{
			tinyState = 0;
		}

		if((font == &FontT144) && (*(char*)pText == '.'))
		{
			tinyState++;
		}
		if((font == &FontT105) && ((*(char*)pText == '.') || (*(char*)pText == ':')))
		{
			tinyState++;
		}

		if(tinyState > 1)
		{
			font = (tFont *)cfg->TinyFont;
		}
		else
		{
			font = (tFont *)cfg->font;
		}

		if(*(char*)pText == ' ')
		{
			Xsum += font->spacesize;
		}
		else
		if((*(char*)pText == '\005') || (*(char*)pText == '\006'))
		{
			Xsum += 45;
		}
		else
		{
			if((*(char*)pText) & 0x80) /* Identify a UNICODE character other than standard ASCII using the highest bit */
			{
				decodeUTF8 = ((*(char*)pText) & 0x1F) << 6; /* use 5bits of first byte for upper part of unicode */
				pText++;
				decodeUTF8 |= (*(char*)pText) & 0x3F; /* add lower 6bits as second part of the unicode */
			}
			else
			{
				decodeUTF8 = *(char*)pText;
			}
			Xsum += GFX_Character_Width(decodeUTF8, font);  /* lookup character and add width */
		}
		pText++;
		j++;
		if((font == &FontT144) && (*(char*)pText != 0))
			Xsum += 3;
		else
		if((font == &FontT105) && (*(char*)pText != 0))
			Xsum += 2;
	}
	pText -= j;

	if(cfg->doubleSize)
		Xsum *= 2;
	
	result = hgfx->WindowX1 - hgfx->WindowX0 - 1;
	if(Xsum < result)
		result -= Xsum;
	else
		result = 0;

	return result;
}

void GFX_LTDC_Init(void)
{
	/*
		HSYNC=10 (9+1)
		HBP=10 (19-10+1)
		ActiveW=480 (499-10-10+1)
		HFP=8 (507-480-10-10+1)

		VSYNC=2 (1+1)
		VBP=2 (3-2+1)
		ActiveH=800 (803-2-2+1)
		VFP=2 (805-800-2-2+1)
	*/

  /* Timing configuration */
  /* Horizontal synchronization width = Hsync - 1 */
  LtdcHandle.Init.HorizontalSync = 9;
  /* Vertical synchronization height = Vsync - 1 */
  LtdcHandle.Init.VerticalSync = 1;
  /* Accumulated horizontal back porch = Hsync + HBP - 1 */
  LtdcHandle.Init.AccumulatedHBP = 19;
  /* Accumulated vertical back porch = Vsync + VBP - 1 */
  LtdcHandle.Init.AccumulatedVBP = 3;
  /* Accumulated active width = Hsync + HBP + Active Width - 1 */
  LtdcHandle.Init.AccumulatedActiveW = 499;//500;//499;
  /* Accumulated active height = Vsync + VBP + Active Heigh - 1 */
  LtdcHandle.Init.AccumulatedActiveH = 803;
  /* Total width = Hsync + HBP + Active Width + HFP - 1 */
  LtdcHandle.Init.TotalWidth = 507;//508;//507;
  /* Total height = Vsync + VBP + Active Heigh + VFP - 1 */
  LtdcHandle.Init.TotalHeigh = 805;

	/* Configure R,G,B component values for LCD background color */
	LtdcHandle.Init.Backcolor.Red= 0;
	LtdcHandle.Init.Backcolor.Blue= 0;
	LtdcHandle.Init.Backcolor.Green= 0;

	/* LCD clock configuration */
	/* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
	/* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
	/* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/4 = 48 Mhz */
	/* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_8 = 48/4 = 6Mhz */

/* done in main.c SystemClockConfig

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
	PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
	PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
	PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
*/
	/* Polarity */
	LtdcHandle.Init.HSPolarity = LTDC_HSPOLARITY_AL;
	LtdcHandle.Init.VSPolarity = LTDC_VSPOLARITY_AL;
	LtdcHandle.Init.DEPolarity = LTDC_DEPOLARITY_AL;
	LtdcHandle.Init.PCPolarity = LTDC_PCPOLARITY_IIPC;//LTDC_PCPOLARITY_IPC;

	LtdcHandle.Instance = LTDC;

  /* Configure the LTDC */
  if(HAL_LTDC_Init(&LtdcHandle) != HAL_OK) // auch init der GPIO Pins
  {
    /* Initialization Error */
    GFX_Error_Handler();
  }
}

void GFX_LTDC_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address)
{
  LTDC_LayerCfgTypeDef   Layercfg;

 /* Layer Init */
  Layercfg.WindowX0 = 0;
  Layercfg.WindowX1 = 480;
  Layercfg.WindowY0 = 0;
  Layercfg.WindowY1 = 800;
  Layercfg.PixelFormat = LTDC_PIXEL_FORMAT_AL88;//LTDC_PIXEL_FORMAT_ARGB8888;
  Layercfg.FBStartAdress = FB_Address;
  Layercfg.Alpha = 255;
  Layercfg.Alpha0 = 0;
  Layercfg.Backcolor.Blue = 0;
  Layercfg.Backcolor.Green = 0;
  Layercfg.Backcolor.Red = 0;
  Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  Layercfg.ImageWidth = 480;
  Layercfg.ImageHeight = 800;

	HAL_LTDC_ConfigCLUT(&LtdcHandle, ColorLUT, CLUT_END, LayerIndex);
  HAL_LTDC_ConfigLayer(&LtdcHandle, &Layercfg, LayerIndex);
	HAL_LTDC_EnableCLUT(&LtdcHandle, LayerIndex);
}

static uint32_t GFX_doubleBufferOne(void)
{
	return SDRAM_DOUBLE_BUFFER_ONE;
}


static uint32_t GFX_doubleBufferTwo(void)
{
	return SDRAM_DOUBLE_BUFFER_TWO;
}

uint32_t getFrame(uint8_t callerId)
{
	static uint8_t lastFrameProvided = 0;
	uint8_t i;

/* first iteration: look for a clear frame */
	i = lastFrameProvided;
	do
	{
		i++;
		if(i == MAXFRAMES)
		{
			i = 0;
		}
	} while((i != lastFrameProvided) && (frame[i].status != CLEAR));

	if((i < MAXFRAMES) && (frame[i].status == CLEAR))
	{
		frame[i].status = BLOCKED;
		frame[i].caller = callerId;
		lastFrameProvided = i;
		return frame[i].StartAddress;
	}

/* second iteration: look for a frame which may be reused after clearing */
	i = lastFrameProvided;
	do
	{
		i++;
		if(i == MAXFRAMES)
		{
			i = 0;
		}
	}while((i != lastFrameProvided) && (frame[i].status != RELEASED));


	if((i < MAXFRAMES) && (frame[i].status == RELEASED))
	{
		GFX_clear_frame_immediately(frame[i].StartAddress);
		frame[i].status = BLOCKED;
		lastFrameProvided = i;
		return frame[i].StartAddress;
	}
	return 0;
}


void GFX_forceReleaseFramesWithId(uint8_t callerId)
{
	for(int i=0; i<MAXFRAMES; i++)
		if((frame[i].caller == callerId) && (frame[i].status == BLOCKED))
			frame[i].status = RELEASED;
}


void releaseAllFramesExcept(uint8_t callerId, uint32_t frameStartAddress)
{
	for(int i=0; i<MAXFRAMES; i++)
		if((frame[i].caller == callerId) && (frame[i].status == BLOCKED) && (frame[i].StartAddress != frameStartAddress))
			frame[i].status = RELEASED;
}


uint8_t releaseFrame(uint8_t callerId, uint32_t frameStartAddress)
{
	static uint8_t countErrorCalls = 0;
	
	if(frameStartAddress < FBGlobalStart)
		return 2;


	uint8_t i;

	i = 0;
	while((i < MAXFRAMES) && (frame[i].StartAddress != frameStartAddress))
		i++;

	if((i < MAXFRAMES) && (frame[i].StartAddress == frameStartAddress))
	{
		if(frame[i].caller == callerId)
		{
			frame[i].status = RELEASED;
			return 1;
		}
		else
			countErrorCalls++;
	}
	return 0;
}


uint16_t blockedFramesCount(void)
{
	uint16_t count = MAXFRAMES;
	
	for(int i = 0;i<MAXFRAMES;i++)
		if(frame[i].status == BLOCKED)
			count--;
		
	return count;
}


uint8_t housekeepingFrame(void)
{
	static uint8_t countLogClear = 0;
	uint8_t i;
	uint8_t retVal = 1;
	
	if(DMA2D_at_work == 255)
	{
		i = 0;
		/* skip frame cleaning for actual frames which have not yet been replaced by new top/bottom frames */
		while((i < MAXFRAMES) && ((frame[i].status != RELEASED) || (frame[i].StartAddress == GFX_get_pActualFrameTop()) || (frame[i].StartAddress == GFX_get_pActualFrameBottom())))
			i++;
	
		if((i < MAXFRAMES) && (frame[i].status == RELEASED))
		{
			if(frame[i].caller == 15)
				countLogClear++;
			GFX_clear_frame_dma2d(i);
		}
		else
		{
			retVal = 0;		/* no more frame to be cleaned found */
		}
	}
	return retVal;
}


static void GFX_Dma2d_TransferComplete(DMA2D_HandleTypeDef* Dma2dHandle)
{
	if(DMA2D_at_work < MAXFRAMES)
		frame[DMA2D_at_work].status = CLEAR;

	DMA2D_at_work = 255;
}


static void GFX_Dma2d_TransferError(DMA2D_HandleTypeDef* Dma2dHandle)
{

}

static void GFX_Error_Handler(void)
{
    /* Turn LED3 on */
//    BSP_LED_On(LED3);
    while(1)
    {
    }
}

void write_content_simple(GFX_DrawCfgScreen *tMscreen, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t color)
{
	GFX_DrawCfgWindow	hgfx;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(!pSettings->FlipDisplay)
	{
		if(XrightGimpStyle > 799)
			XrightGimpStyle = 799;
		if(XleftGimpStyle >= XrightGimpStyle)
			XleftGimpStyle = 0;
		if(YtopGimpStyle > 479)
			YtopGimpStyle = 479;
	}
	hgfx.Image = tMscreen;
	hgfx.WindowNumberOfTextLines = 1;
	hgfx.WindowLineSpacing = 0;
	hgfx.WindowTab = 0;

	if(!pSettings->FlipDisplay)
	{
		hgfx.WindowX0 = XleftGimpStyle;
		hgfx.WindowX1 = XrightGimpStyle;
		hgfx.WindowY1 = 479 - YtopGimpStyle;
		if(hgfx.WindowY1 < Font->height)
			hgfx.WindowY0 = 0;
		else
			hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
	}
	else
	{
		hgfx.WindowX0 = 800 - XrightGimpStyle;
		hgfx.WindowX1 = 800 - XleftGimpStyle;
		hgfx.WindowY0 = YtopGimpStyle;
		if(hgfx.WindowY0 + Font->height >= 479)
			hgfx.WindowY1 = 479;
		else
			hgfx.WindowY1 = hgfx.WindowY0 + Font->height;
	}
	GFX_write_string_color(Font, &hgfx, text, 0, color);
}


void gfx_write_topline_simple(GFX_DrawCfgScreen *tMscreen, const char *text, uint8_t color)
{
	GFX_DrawCfgWindow	hgfx;
	const tFont *Font = &FontT48;
	
	hgfx.Image = tMscreen;
	hgfx.WindowNumberOfTextLines = 1;
	hgfx.WindowLineSpacing = 0;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	hgfx.WindowTab = 0;
	hgfx.WindowX0 = 20;
	hgfx.WindowX1 = 779;

	if(!pSettings->FlipDisplay)
	{
		hgfx.WindowY1 = 479;
		hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
	}
	else
	{
		hgfx.WindowY0 = 0;
		hgfx.WindowY1 = Font->height;
	}
	GFX_write_label(Font, &hgfx, text, color);
}


void gfx_write_page_number(GFX_DrawCfgScreen *tMscreen, uint8_t page, uint8_t total, uint8_t color)
{
	GFX_DrawCfgWindow	hgfx;
	const tFont *Font = &FontT48;
	char text[7];
	uint8_t i, secondDigitPage, secondDigitTotal;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(total > 8)
	{
		Font = &FontT24;
	}

	hgfx.Image = tMscreen;
	hgfx.WindowNumberOfTextLines = 1;
	hgfx.WindowLineSpacing = 0;
	hgfx.WindowTab = 0;

	if(!pSettings->FlipDisplay)
	{
		hgfx.WindowX1 = 779;
		if(Font == &FontT24)
		{
			hgfx.WindowX0 = hgfx.WindowX1 - (Font->spacesize*3);
		}
		else
		{
			hgfx.WindowX0 = hgfx.WindowX1 - (Font->spacesize2Monospaced*3);
		}
		hgfx.WindowY1 = 479;
		hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
	}
	else
	{
		hgfx.WindowX1 = 25*5;
		hgfx.WindowX0 = 0;
		hgfx.WindowY1 = Font->height;;
		hgfx.WindowY0 = 0;
	}
	if(page > 99)
		page = 99;
	if(total > 99)
		total = 99;
	
	i = 0;
	text[i++] = '\002';
	
	secondDigitPage = page / 10;
	page -= secondDigitPage * 10;

	secondDigitTotal = total / 10;
	total -= secondDigitTotal * 10;
	
	if(secondDigitPage)
		text[i++] = '0' + secondDigitPage;
	text[i++] = '0' + page;

	text[i++] = '/';

	if(secondDigitTotal)
		text[i++] = '0' + secondDigitTotal;
	text[i++] = '0' + total;

	text[i] = 0;
	
	GFX_clear_window_immediately(&hgfx);
	GFX_write_label(Font, &hgfx, text, color);
}


uint8_t gfx_number_to_string(uint8_t max_digits, _Bool fill, char *pText, uint32_t input)
{
	uint8_t digits[10];
	uint32_t number, divider;
	int first;
	uint8_t out;
	
	number = input;
	first = 0;
	divider = 1000000000;
	for(int i=9;i>=0;i--)
	{
		digits[i] = (uint8_t)(number / divider);
		number -= digits[i] * divider;
		divider /= 10;
		if((first == 0) && (digits[i] != 0))
			first = i;
	}
	
	if((first + 1) > max_digits)
	{
		for(int i = 0; i<max_digits; i++)
			pText[i] = '9';
		out = max_digits;
	}
	else if(fill)
	{
		int i = 0;
		for(int k = max_digits; k>0; k--)
			pText[i++] = digits[k -1] +  '0';
		out = max_digits;
	}
	else
	{
		int i = 0;
		for(int k = first; k>=0; k--)
			pText[i++] = digits[k] +  '0';
		out = i;
	}
	
	return out;
}


	/* output is
	 * 0->
	 *    |
	 *    v
	 *
	 * input is
	 * 
	 *   -> 
	 * A
	 * |
	 * 0
	 */
void GFX_screenshot(void)
{
	uint32_t pSource = GFX_get_pActualFrameTop();
	uint32_t pSourceBottom =GFX_get_pActualFrameBottom();
	uint32_t pBottomNew = getFrame(99);
	uint32_t pDestination = GFX_doubleBufferOne();
	uint32_t sourceNow;

	
	uint32_t 	bot_leftStart = FrameHandler.actualBottom.leftStart; 			// x0 z.B. 0
	uint32_t 	bot_bottomStart = FrameHandler.actualBottom.bottomStart; 	// y0 z.B. 25
	uint32_t  bot_width = FrameHandler.actualBottom.width;							// 800
	uint32_t  bot_height = FrameHandler.actualBottom.height;						// 390

	struct split
	{
		uint8_t blue;
		uint8_t green;
		uint8_t red;
		uint8_t alpha;
	};

	union inout_u
	{
		uint32_t in;
		struct split out;
	};
	
	union inout_u value;

/* test	
 uint32_t pSourceTemp = pSource + (2*479);
	for (int j = 0xFFFF; j > 0x00FF; j -= 0x0100)
	{
		*(__IO uint16_t*)pSourceTemp = j;
		pSourceTemp += 480*2;
	}
*/
	// Top Layer
	const unsigned width = 800, height = 480;
	const uint32_t heightX2 = height*2;
	
	for(unsigned y = 0; y < height; y++)
	{
		sourceNow = pSource + 2 * ((height - 1) - y);
		for(unsigned x = 0; x < width; x++)
		{
//			sourceNow += 2 * height * x  + 2 * (height - 1 - y);
			value.in = ColorLUT[*(__IO uint8_t*)(sourceNow)];
			value.out.alpha = *(__IO uint8_t*)(sourceNow + 1);
			
			*(__IO uint8_t*)(pDestination++) = value.out.red;
			*(__IO uint8_t*)(pDestination++) = value.out.green;
			*(__IO uint8_t*)(pDestination++) = value.out.blue;
			*(__IO uint8_t*)(pDestination++) = value.out.alpha;
			sourceNow += heightX2;
		}
	}
	
	// Bottom Layer
	// build newBottom
	pSource = pSourceBottom;
	for(unsigned x = bot_leftStart; x < bot_leftStart+bot_width; x++)
	{
		for(unsigned y = bot_bottomStart; y < bot_bottomStart+bot_height; y++)
		{
				pDestination = pBottomNew + (2 * y);
				pDestination += heightX2 * x;
				*(__IO uint16_t*)(pDestination) = *(__IO uint16_t*)(pSource);
				pSource += 2;
		}
	}		
	
	// output Bottom Layer
	pSource = pBottomNew;
	pDestination = GFX_doubleBufferTwo();

	for(unsigned y = 0; y < height; y++)
	{
		sourceNow = pSource + 2 * ((height - 1) - y);
		for(unsigned x = 0; x < width; x++)
		{
//		sourceNow =  pSource + 2 * height * x  + 2 * (height - 1 - y);
			value.in = ColorLUT[*(__IO uint8_t*)(sourceNow)];
			value.out.alpha = *(__IO uint8_t*)(sourceNow + 1);
			
			*(__IO uint8_t*)(pDestination++) = value.out.red;
			*(__IO uint8_t*)(pDestination++) = value.out.green;
			*(__IO uint8_t*)(pDestination++) = value.out.blue;
			*(__IO uint8_t*)(pDestination++) = value.out.alpha;
			sourceNow += heightX2;
		}
	}
	releaseFrame(99,pBottomNew);
/*
	// das kommt dazu!
	unsigned yEnd = 480 - FrameHandler.actualBottom.bottomStart;
	unsigned yStart = yEnd - FrameHandler.actualBottom.height;
	
	if(yStart > 0)
	{
		for(unsigned y = 0; y < yStart; y++)
		for(unsigned x = 0; x < width; x++)
		{
			*(__IO uint8_t*)(pDestination++) = 0;
			*(__IO uint8_t*)(pDestination++) = 0;
			*(__IO uint8_t*)(pDestination++) = 0;
			*(__IO uint8_t*)(pDestination++) = 0;
		}
	}
	for(unsigned y = yStart; y < yEnd; y++)
	for(unsigned x = 0; x < width; x++)
	{
		sourceNow =  pSource + 2 * height * x  + 2 * (height - 1 - y);
		value.in = ColorLUT[*(__IO uint8_t*)(sourceNow)];
		value.out.alpha = *(__IO uint8_t*)(sourceNow + 1);
		
		*(__IO uint8_t*)(pDestination++) = value.out.red;
		*(__IO uint8_t*)(pDestination++) = value.out.green;
		*(__IO uint8_t*)(pDestination++) = value.out.blue;
		*(__IO uint8_t*)(pDestination++) = value.out.alpha;
	}
	if(yEnd < 480)
	{
		for(unsigned y = yEnd; y < 480; y++)
		for(unsigned x = 0; x < width; x++)
		{
			*(__IO uint8_t*)(pDestination++) = 0;
			*(__IO uint8_t*)(pDestination++) = 0;
			*(__IO uint8_t*)(pDestination++) = 0;
			*(__IO uint8_t*)(pDestination++) = 0;
		}
	}
*/	
}

#ifndef BOOTLOADER_STANDALONE
tFont* GFX_Check_Extra_Font(uint8_t character, tFont *Font)
{
	uint32_t i;
	uint32_t found;

	found = 0;
	for(i=0;i<Font->length;i++)
	{
		if(Font->chars[i].code == character)
		{
			found = 1;
			break;
		}
	}
	if (!found && Font == &FontT54)
	{
		Font = (tFont *)&FontT54Extra;
	}
	else if (!found && (Font == &FontT84 || Font == &FontT84Spaced))
	{
		Font = (tFont *)&FontT84Extra;
	}
	else if (!found && Font == &FontT105)
	{
		Font = (tFont *)&FontT105Extra;
	}

	return Font;
}
#endif
uint32_t GFX_Character_Width(uint8_t character, tFont *Font)
{
	uint32_t i;
#ifndef BOOTLOADER_STANDALONE
	uint32_t found;
#endif

	for(i=0;i<Font->length;i++)
	{
		if(Font->chars[i].code == character)
		{
			return Font->chars[i].image->width;
		}
	}

#ifndef BOOTLOADER_STANDALONE
	found = 0;
	if (Font == &FontT54)
	{
		found = 1;
		Font = (tFont *)&FontT54Extra;
	}
	else if (Font == &FontT84 || Font == &FontT84Spaced)
	{
		found = 1;
		Font = (tFont *)&FontT84Extra;
	}
	else if (Font == &FontT105)
	{
		found = 1;
		Font = (tFont *)&FontT105Extra;
	}

	if (found)
	{
		for(i=0;i<Font->length;i++)
		{
			if(Font->chars[i].code == character)
			{
				return Font->chars[i].image->width;
			}
		}
	}
#endif
	return 0;
}

void Gfx_colorsscheme_mod(char *text, uint8_t alternativeColor)
{
	char *p = text;
	uint8_t index = 0;

	while ((*p) && (index < MAX_COLOR_STRING_LENGTH))
	{
		if (*p == '\020')
		{
			if(!GFX_is_colorschemeDiveStandard())
			{
				*p = '\027';
			}
			else if(alternativeColor != 0)
			{
				*p += alternativeColor;
			}
		}
		p++;
		index++;
	}
}

