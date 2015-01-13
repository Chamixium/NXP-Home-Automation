/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          app_management_bind.c
 *
 * DESCRIPTION:        ZHA Demo : Stack <-> Light-App Interaction (Implementation)
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
#include <string.h>
#include "dbg.h"
#include "os.h"
#include "os_gen.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "pdum_gen.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_aib.h"
#include "zps_nwk_sap.h"
#include "zcl_options.h"
#include "app_zbp_utilities.h"
#include "app_management_bind.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_MANAGEMENT_BIND
	#define TRACE_MANAGEMENT_BIND   FALSE
#else
	#define TRACE_MANAGEMENT_BIND   TRUE
#endif

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
/***        Local Variables                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
extern void zps_vAplZdoMgmtBindServerInit(PDUM_thAPdu hOutAPdu);
extern bool zps_bAplZdoMgmtBindServer( ZPS_tsAfEvent *psZdoServerEvent,
		                                PDUM_thAPduInstance hAPduInst,
		                                uint8* pu8SeqNum);


/****************************************************************************
 *
 * NAME: bMgmtBindServerCallback
 *
 * DESCRIPTION:
 * Called back when mgmt server req is received by the stack and served
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC bool bMgmtBindServerCallback(uint16 u16ClusterId)
{
	if (u16ClusterId == 0x0033 )
	{
		return TRUE;
	}
	return FALSE;
}


/****************************************************************************
 *
 * NAME: vAppRegisterManagementBindServer
 *
 * DESCRIPTION:
 * Registers the app handling of the bind server
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC void vAppRegisterManagementBindServer(void)
{
	ZPS_eAplZdoRegisterZdoFilterCallback(bMgmtBindServerCallback);
	zps_vAplZdoMgmtBindServerInit(apduZDP);
}


/****************************************************************************
 *
 * NAME: vHandleManagementBindEvents
 *
 * DESCRIPTION:
 * app handling of the bind server events
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC void vHandleManagementBindEvents(ZPS_tsAfEvent * psStackEvent )
{
	 uint8 u8SeqNum;
	 bool bStatus;

	if( (ZPS_EVENT_APS_DATA_INDICATION == psStackEvent->eType) ||
		(ZPS_EVENT_APS_DATA_CONFIRM    == psStackEvent->eType) ||
		(ZPS_EVENT_APS_DATA_ACK        == psStackEvent->eType)    )
	{

		PDUM_thAPduInstance hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

		if (hAPduInst == PDUM_INVALID_HANDLE)
		{
			DBG_vPrintf(TRACE_MANAGEMENT_BIND, "Allocate PDU ERR:\n");
		}
		else
		{
			bStatus = zps_bAplZdoMgmtBindServer(psStackEvent,hAPduInst,&u8SeqNum);
			if (FALSE == bStatus)
			{
				PDUM_eAPduFreeAPduInstance(hAPduInst);
			}
		}
	}
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/



/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
