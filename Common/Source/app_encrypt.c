/****************************************************************************
 *
 * MODULE:             JN-AN-1189 ZHA Demo
 *
 * COMPONENT:          app_encrypt.c
 *
 * DESCRIPTION:        Encrypts a 5168 device
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
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
 * Copyright NXP B.V. 2014. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "dbg_uart.h"
#include "appapi.h"
#include "app_encrypt.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
//#define DEBUG_APP_ENCRYPT

#ifndef DEBUG_APP_ENCRYPT
#define TRACE_APP_ENCRYPT FALSE
#else
#define TRACE_APP_ENCRYPT TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vWriteAESKey(void);
PRIVATE bool_t bAHI_WriteCustomerSettings_PRV(
        bool_t      bJTAGdisable,
        uint8       u8VBOthreshold,
        uint8       u8CRP,
        bool_t      bEncryptedExternalFlash,
        bool_t      bDisableLoadFromExternalFlash
        );
PRIVATE void vReadAndWriteBackCustomerSettings(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern bool_t bAHI_ReadMACID(bool_t bFromIndexSector,uint64 * pu64MACID);
extern bool_t bAHI_ReadCustomerAESkey(uint32 * pu32Key);
extern bool_t bAHI_ReadUserData(uint8 u8UserFieldIndex,uint32 * pu32userData);
extern bool_t bAHI_ReadCustomerSettings(bool_t * pbJTAGdisable,
        uint8 * pu8VBOthreshold,
        uint8 * pu8CRP,
        bool_t * pbEncryptedExternalFlash,
        bool_t * pbDisableLoadFromExternalFlash);
extern bool_t bAHI_BlankCheckIndexSectorPage(uint8 a,uint8 b);
extern bool_t bAHI_WriteIndexSectorPage(
        uint8 a,
        uint8 b,
        uint32 u32FlashWriteWord1,
        uint32 u32FlashWriteWord2,
        uint32 u32FlashWriteWord3,
        uint32 u32FlashWriteWord4);
extern bool_t bAHI_WriteCustomerAESkey(uint32 * pu32AESkey);
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vEncryptDevice
 *
 * DESCRIPTION:
 * Encrypts the Device by calling this function.
 * Call this in the vAppInitialise if required.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vEncryptDevice(void)
{
//    uint32 i;
    DBG_vPrintf(TRUE,"In vEncryptDevice\n\n");
    /*Just a small delay by spinning here*/
//    for (i=0;i<100000;i++)
//    	DBG_vPrintf(TRACE_APP_ENCRYPT, "*");
    vDisplayIndexSectorSettings();
    vReadAndWriteBackCustomerSettings();
    vDisplayIndexSectorSettings();
}
/****************************************************************************
 *
 * NAME: vDisplayIndexSectorSettings
 *
 * DESCRIPTION:
 * Displays the index sectors
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vDisplayIndexSectorSettings(void)
{
	bool_t bStatus;
	uint8 i;

	/************************************************************************/
	uint64 u64CustomerMACId;
	bStatus = bAHI_ReadMACID(TRUE,&u64CustomerMACId);
	DBG_vPrintf(TRACE_APP_ENCRYPT,"\n\nTRUE => Status = %d, MACID = 0x%016llx\n",bStatus,u64CustomerMACId);
	bStatus = bAHI_ReadMACID(FALSE,&u64CustomerMACId);
	DBG_vPrintf(TRACE_APP_ENCRYPT,"FALSE => Status = %d, MACID = 0x%016llx\n\n",bStatus,u64CustomerMACId);

	/************************************************************************/
	uint32 au32CustomerAESkey[4];
	bStatus = bAHI_ReadCustomerAESkey(au32CustomerAESkey);
	DBG_vPrintf(TRACE_APP_ENCRYPT,"bAHI_ReadCustomerAESkey Status = %d\n",bStatus);
	for(i=0;i<4;i++)
	{
		DBG_vPrintf(TRACE_APP_ENCRYPT,"Key[%d] = 0x%08x \n",i,au32CustomerAESkey[i]);
	}

	/************************************************************************/
    uint8       u8UserFieldIndex;
    uint32      au32userData[4];

    for(u8UserFieldIndex= 0;u8UserFieldIndex<3;u8UserFieldIndex++)
    {
    	bStatus = bAHI_ReadUserData(u8UserFieldIndex,au32userData);
    	DBG_vPrintf(TRACE_APP_ENCRYPT,"\nUserDataIndex Index = %d, Status = %d\n",u8UserFieldIndex, bStatus);
    	for(i=0;i<4;i++)
    	{
    		DBG_vPrintf(TRACE_APP_ENCRYPT,"User Data [%d] = 0x%08x \n",i,au32userData[i]);
    	}

    }

    /************************************************************************/
    bool_t     bJTAGdisable;
    uint8      u8VBOthreshold;
    uint8      u8CRP;
    bool_t     bEncryptedExternalFlash;
    bool_t     bDisableLoadFromExternalFlash;


	bStatus = bAHI_ReadCustomerSettings(
	        &bJTAGdisable,
	        &u8VBOthreshold,
	        &u8CRP,
	        &bEncryptedExternalFlash,
	        &bDisableLoadFromExternalFlash);

	DBG_vPrintf(TRACE_APP_ENCRYPT,"\n\nbJTAGdisable =  %d\n",bJTAGdisable);
	DBG_vPrintf(TRACE_APP_ENCRYPT,"u8VBOthreshold =  %d\n",u8VBOthreshold);
	DBG_vPrintf(TRACE_APP_ENCRYPT,"u8CRP =  %d\n",u8CRP);
	DBG_vPrintf(TRACE_APP_ENCRYPT,"bEncryptedExternalFlash =  %d\n",bEncryptedExternalFlash);
	DBG_vPrintf(TRACE_APP_ENCRYPT,"bDisableLoadFromExternalFlash =  %d\n",bDisableLoadFromExternalFlash);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: bAHI_WriteCustomerSettings_PRV
 *
 * DESCRIPTION:
 * Customized function just to allow encryption with the debug and
 * no voltage check.
 * This was a copy of the bAHI_WriteCustomerSettings in the AHI lib
 * PARAMETRS:
 *      bool_t      bJTAGdisable              If JTAG needs to be disabled
 *      uint8       u8VBOthreshold            Voltage Threshold
 *      uint8       u8CRP                     CRP value
 *      bool_t      bEncryptedExternalFlash,  Encrypt Ext Flash
 *      bool_t      bDisableLoadFromExternalFlash Disable Load from Ext Flash
 *
 * RETURNS:
 * status, TRUE if success, FALSE if failed
 *
 ****************************************************************************/
PRIVATE bool_t bAHI_WriteCustomerSettings_PRV(
        bool_t      bJTAGdisable,
        uint8       u8VBOthreshold,
        uint8       u8CRP,
        bool_t      bEncryptedExternalFlash,
        bool_t      bDisableLoadFromExternalFlash
        )
{
    uint32 u32FlashWriteWord = 0;

    /* No voltage check , no CRP level check */
    if( //(u8VBOthreshold > VBO_VOLTAGE_3_00V)                     ||
        (u8CRP == 0)                                             ||
        //       (u8CRP > CRP_LEVEL_2_NO_PROGRAMMING_ALLOWED_VIA_UART)    ||
        (bAHI_BlankCheckIndexSectorPage(5, 1) == FALSE)
      )
    {
        return(FALSE);
    }

    // set JTAG - disable if true, enable if false
    if(bJTAGdisable == FALSE)
    {
        // enable JTAG
        U32_SET_BITS(&u32FlashWriteWord, 0x1);
    }

    // invert top 2 bits of u8VBOthreshold
    u8VBOthreshold ^= 0x6;
    // write value
    U32_SET_BITS(&u32FlashWriteWord, (u8VBOthreshold << 1));

    // CRP
    U32_SET_BITS(&u32FlashWriteWord, (u8CRP << 4));

    // flash encrypt - encrypt if true, clear if false
    if(bEncryptedExternalFlash == FALSE)
    {
        // flash not encrypted
    	// As the bEncryptedExternalFlash is passed as TRUE, the following line never be executed hence commented.
        //U32_SET_BITS(&u32FlashWriteWord, 0x1<<6);
    }

    // flash external load - disable load if true, enable if false
    if(bDisableLoadFromExternalFlash == FALSE)
    {
        // enable external flash load
        U32_SET_BITS(&u32FlashWriteWord, 0x1<<7);
    }

    return
    bAHI_WriteIndexSectorPage(
        5,
        1,
        u32FlashWriteWord,
        u32FlashWriteWord,
        u32FlashWriteWord,
        u32FlashWriteWord);
}

/****************************************************************************
 *
 * NAME: vWriteAESKey
 *
 * DESCRIPTION:
 * Sets the AES key in to the AES Key registers
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vWriteAESKey(void)
{
	bool_t bStatus;
	uint32 au32AESkey[4] =
	{
			0xffffffff,
			0xffffffff,
			0xffffffff,
			0xffffffff
	};
	bStatus = bAHI_WriteCustomerAESkey(au32AESkey);
	DBG_vPrintf(TRACE_APP_ENCRYPT, "\n\nWriting Key , Status = %d\n\n",bStatus );

}
/****************************************************************************
 *
 * NAME: vReadAndWriteBackCustomerSettings
 *
 * DESCRIPTION:
 * Reads and then writes back all the values to encrypt the device
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vReadAndWriteBackCustomerSettings(void)
{
	bool_t bStatus;
    bool_t      bJTAGdisable;
    uint8       u8VBOthreshold;
    uint8       u8CRP;
    bool_t      bEncryptedExternalFlash;
    bool_t      bDisableLoadFromExternalFlash;

    vWriteAESKey();

	bStatus = bAHI_ReadCustomerSettings(
									&bJTAGdisable,
									&u8VBOthreshold,
									&u8CRP,
									&bEncryptedExternalFlash,
									&bDisableLoadFromExternalFlash);
	DBG_vPrintf(TRACE_APP_ENCRYPT, "\n Read Status = %d\n",bStatus);

	if(bStatus)
	{
		/* Make the external Flash Encrypted */
		bEncryptedExternalFlash =TRUE;

		bStatus = bAHI_WriteCustomerSettings_PRV(
											bJTAGdisable,
											u8VBOthreshold,
											u8CRP,
											bEncryptedExternalFlash,
											bDisableLoadFromExternalFlash);
		DBG_vPrintf(TRACE_APP_ENCRYPT, "\n Write Status = %d\n",bStatus);
	}
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
