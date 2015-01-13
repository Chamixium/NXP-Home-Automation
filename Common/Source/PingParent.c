/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          PingParent.c
 *
 * DESCRIPTION:        Pings the parent and check to take decision to rejoin.
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142,
 * JN5139]. You, and any third parties must reproduce the copyright and
 * warranty notice and any other legend of ownership on each copy or partial
 * copy of the software.
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/* Stack Includes */
#include <jendefs.h>
#include "dbg.h"
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "zps_apl_af.h"
#include "zps_apl_zdp.h"
#include "zps_apl_aib.h"
#include "zps_nwk_nib.h"
#include "zps_nwk_pub.h"
#include "app_timer_driver.h"
#include "Utilities.h"
#include "PingParent.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_PING_PARENT
#define TRACE_PING_PARENT FALSE
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
uint32 u32PingTime=0;

bool_t bPingSent = FALSE;
bool_t bPingRespRcvd = TRUE;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***		Tasks														  ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vIncrementPingTime
 *
 * DESCRIPTION:
 * Increment the Time for ping
 *
 * PARAMETERS:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vIncrementPingTime(uint8 u8Time)
{
	u32PingTime +=u8Time;
}

/****************************************************************************
 *
 * NAME: bPingParent
 *
 * DESCRIPTION:
 * Read Basic Cluster attribute
 *
 * PARAMETERS:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC bool_t bPingParent(void)
{

	int i=0;
	ZPS_tsNwkNib * thisNib;
	ZPS_teStatus eStatus=1;

	bPingSent=FALSE;

	if(u32PingTime >=60 )
	{
		u32PingTime=0;
		thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());
		for(i = 0 ; i < thisNib->sTblSize.u16NtActv ; i++)
		{
			if( ( ZPS_NWK_NT_AP_RELATIONSHIP_PARENT == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u2Relationship) &&
			    ( 0xFFFE != thisNib->sTbl.psNtActv[i].u16NwkAddr  ) )
			{
				PDUM_thAPduInstance hAPduInst;
				hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

				if (hAPduInst == PDUM_INVALID_HANDLE)
				{
					DBG_vPrintf(TRACE_PING_PARENT, "IEEE Address Request - PDUM_INVALID_HANDLE\n");
				}
				else
				{
					ZPS_tuAddress uAddress;
					ZPS_tsAplZdpNwkAddrReq sAplZdpNwkAddrReq;
					uint8 u8TransactionSequenceNumber;

					uAddress.u16Addr = thisNib->sTbl.psNtActv[i].u16NwkAddr;

					sAplZdpNwkAddrReq.u64IeeeAddr = thisNib->sTbl.psNtActv[i].u64ExtAddr;
					sAplZdpNwkAddrReq.u8RequestType = 0;

					eStatus = ZPS_eAplZdpNwkAddrRequest(	hAPduInst,
																		uAddress,
																		FALSE,
																		&u8TransactionSequenceNumber,
																		&sAplZdpNwkAddrReq
																		);
					break;

				}
			}
		}
	}

	if(!eStatus)
	{
		bPingSent=TRUE;
		bPingRespRcvd = FALSE;
		return TRUE;
	}
	return FALSE;
}


OS_TASK(APP_PingTimerTask)
{
	OS_eStopSWTimer(App_PingTimer);
	if( (TRUE == bPingSent) && (FALSE == bPingRespRcvd))
	{
		ZPS_eAplZdoRejoinNetwork();
	}
}

/****************************************************************************
 *
 * NAME: vCheckIfMyChild
 *
 * DESCRIPTION:
 * called on ZDP_REQ/RESP packet to see if it a NWK address response for
 * one of its own children, is so, remove
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vPingRecv( ZPS_tsAfEvent  * sStackEvent )
{
	if (APP_NWK_ADRRESS_RESPONSE == sStackEvent->uEvent.sApsZdpEvent.u16ClusterId)
	{
		bPingRespRcvd = TRUE;
	}
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
