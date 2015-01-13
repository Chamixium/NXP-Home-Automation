/****************************************************************************
 *
 * MODULE              AN1166 Smart Lamp Drivers
 *
 * DESCRIPTION         RGB/Monochrome bulb Driver (DR1175): Implementation
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/* SDK includes */
#include <jendefs.h>
#include "AppHardwareApi.h"

/* DK4 includes */
#include "LightingBoard.h"

/* Device includes */
#include "DriverBulb.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define ADC_FULL_SCALE   1023
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Global Variables                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool_t bBulbOn = FALSE;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void DriverBulb_vInit(void)
{
	static bool_t bFirstCalled = TRUE;

	if (bFirstCalled)
	{
		bRGB_LED_Enable();
		bRGB_LED_Off();
		bRGB_LED_SetLevel(0,0,0);
		bWhite_LED_Enable();
		bWhite_LED_Off();
		bWhite_LED_SetLevel(0);
		bFirstCalled = FALSE;
	}
}

PUBLIC void DriverBulb_vOn(void)
{
	DriverBulb_vSetOnOff(TRUE);
}

PUBLIC void DriverBulb_vOff(void)
{
	DriverBulb_vSetOnOff(FALSE);
}

PUBLIC void DriverBulb_vSetOnOff(bool_t bOn)
{
#ifdef RGB
	(bOn) ? bRGB_LED_On(): bRGB_LED_Off();
#else
	(bOn) ? bWhite_LED_On() : bWhite_LED_Off();
#endif
     bBulbOn =  bOn;
}

PUBLIC void DriverBulb_vSetLevel(uint32 u32Level)
{
#ifdef RGB
	bRGB_LED_SetGroupLevel(u32Level);
#else
	bWhite_LED_SetLevel(u32Level);
#endif
}

PUBLIC void DriverBulb_vSetColour(uint32 u32Red, uint32 u32Green, uint32 u32Blue)
{
	bRGB_LED_SetLevel(u32Red,u32Green,u32Blue);
}

PUBLIC bool_t DriverBulb_bOn(void)
{
	return (bBulbOn);
}

PUBLIC bool_t DriverBulb_bReady(void)
{
	return (TRUE);
}

PUBLIC bool_t DriverBulb_bFailed(void)
{
	return (FALSE);
}

PUBLIC void DriverBulb_vTick(void)
{
/* No timing behaviour needed in DR1175 */
}

PUBLIC int16 DriverBulb_i16Analogue(uint8 u8Adc, uint16 u16AdcRead)
{
	if (u8Adc == E_AHI_ADC_SRC_VOLT)
	{
		return ((u16AdcRead * 3600)/ADC_FULL_SCALE);
	}
	else
	{
		return(ADC_FULL_SCALE);
	}
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
