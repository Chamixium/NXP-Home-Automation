/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:         zps_apl_zdo_mgmt_bind_server.c
 *
 * DESCRIPTION:       Stack Management Bind Server (Implementation)
 *
 *****************************************************************************
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <string.h>
#include "zps_apl_af.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdo.h"
#include "zps_apl_zdp.h"
#include "zps_apl.h"
#include "dbg.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER
	#define TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER    FALSE
#else
	#define TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER    TRUE
#endif
#ifndef TRACE_ASSERT
#define TRACE_ASSERT  FALSE
#else
#define TRACE_ASSERT  TRUE
#endif

#define ZDP_MGMT_BIND_RSP_FMT                           "bbbbb"
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct {
    uint8 eState;
    uint8 u8SeqNum;
    uint8 u8ConfAck;
} zps_tsZdoMgmtBindServerConfAckContext;

typedef struct{
    PDUM_thAPdu hOutAPdu;
    zps_tsZdoMgmtBindServerConfAckContext sConfAckContext;
    ZPS_tsAfDataIndEvent sDataIndicationEvent;
} tsZdoMgmtBindServerContext;

enum {
    ZPS_E_APL_ZDO_MGMT_BIND_SERVER_STATE_IDLE = 0,
    ZPS_E_APL_ZDO_MGMT_BIND_SERVER_STATE_WAITING_FOR_ACK_AND_CONF = 1
} eMgmtBindState;



/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
tsZdoMgmtBindServerContext *psMgmtBindServerContext = NULL;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Public Functions                                     ***/
/****************************************************************************/
uint8 u8ZdoMgmtBindServerUnpackApdu(
    PDUM_thAPduInstance hAPduInst,
    ZPS_tsAplZdpMgmtBindReq *pReqStruct);
bool zps_bAplZdoMgmtBindServerHandleConfirmAcks(zps_tsZdoMgmtBindServerConfAckContext *psContext, ZPS_tsAfEvent *psZdoServerEvent, uint8 eFinishState);


/****************************************************************************/
/***        Exported Private Functions                                      */
/****************************************************************************/

/****************************************************************************
 *
 * NAME:       zps_vAplZdoMgmtBindServerInit
 */
/**
 * @ingroup
 *
 * @param
 *
 * @return
 *
 * @note
 *
 ****************************************************************************/

void zps_vAplZdoMgmtBindServerInit(PDUM_thAPdu hOutAPdu)
{
    DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER,"ZPSAPL: ZDO Mgmt Bind Server initialised\n");

    psMgmtBindServerContext->hOutAPdu = hOutAPdu;
    psMgmtBindServerContext->sConfAckContext.eState = ZPS_E_APL_ZDO_MGMT_BIND_SERVER_STATE_IDLE;

}

/****************************************************************************
 *
 * NAME:       zps_bAplZdoMgmtBindServer
 */
/**
 * @ingroup
 *
 * @param
 *
 * @return
 *
 * @note
 *
 ****************************************************************************/

bool zps_bAplZdoMgmtBindServer(
    ZPS_tsAfEvent *psZdoServerEvent, PDUM_thAPduInstance hAPduInst,uint8* pu8SeqNum)
{
    bool bEventHandled = FALSE;
    ZPS_tsAplAib *psAib;
    ZPS_tsAplZdpMgmtBindReq sAplZdpMgmtBindReq;
    ZPS_tsAplApsmeBindingTable *pBindingTable;
    uint8 u8ZdpSeqNum,u8Status,maxcount;
    uint16 u16Size=0;
    uint32 u32Counter,u32Location;

    psAib = ZPS_psAplAibGetAib();
    if((psZdoServerEvent == NULL ||
        psAib == NULL ||
        psAib->psAplApsmeAibBindingTable == NULL ||
        psAib->psAplApsmeAibBindingTable->psAplApsmeBindingTable == NULL ))
    {
    	DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER," Returning from zps_bAplZdoMgmtBindServer\n");
        return FALSE;
    }
    pBindingTable = psAib->psAplApsmeAibBindingTable->psAplApsmeBindingTable;

    if (ZPS_E_APL_ZDO_MGMT_BIND_SERVER_STATE_IDLE == psMgmtBindServerContext->sConfAckContext.eState &&
        ZPS_EVENT_APS_DATA_INDICATION == psZdoServerEvent->eType)
    {
        /* ZDP message received */
        if (0x0033 == psZdoServerEvent->uEvent.sApsDataIndEvent.u16ClusterId)
        {

            DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER,"ZPSAPL: ZDO Mgmt Server received %x!!\n", psZdoServerEvent->uEvent.sApsDataIndEvent.u16ClusterId);

            bEventHandled = TRUE;
            u8Status = 0xB0;
            u8ZdpSeqNum = u8ZdoMgmtBindServerUnpackApdu(psZdoServerEvent->uEvent.sApsDataIndEvent.hAPduInst, &sAplZdpMgmtBindReq);

            DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER,"ZPSAPL: memcpy %d!!\n", sizeof(ZPS_tsAfDataIndEvent));
            memcpy (&psMgmtBindServerContext->sDataIndicationEvent, &psZdoServerEvent->uEvent.sApsDataIndEvent,sizeof(ZPS_tsAfDataIndEvent));
            DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER,"ZPSAPL: memcpy ok !!\n");

            if(sAplZdpMgmtBindReq.u8StartIndex <= pBindingTable->u32SizeOfBindingTable)
            {
            	u16Size = PDUM_u16SizeNBO(ZDP_MGMT_BIND_RSP_FMT);
            	u8Status = ZPS_E_SUCCESS;
                for(u32Counter = sAplZdpMgmtBindReq.u8StartIndex,maxcount = 0; u32Counter < pBindingTable->u32SizeOfBindingTable && maxcount < 3; u32Counter++)
                {
                	if((pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u8DstAddrMode > 0x0) &&
                			(pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u8DstAddrMode < 0x4))
                	{

                        if(pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u8DstAddrMode == 0x3)
                        {
                            u16Size += PDUM_u16SizeNBO("lbhblb");
                        }
                        else
                        {
                            u16Size +=  PDUM_u16SizeNBO("lbhbh");
                        }
                        maxcount++;
                	}
                }
                if (PDUM_E_OK == PDUM_eAPduInstanceSetPayloadSize(hAPduInst,u16Size))
                {
                   /*[ISP001377_sfr 2,6,257]*/
                   u32Location = PDUM_u16APduInstanceWriteNBO(hAPduInst, 0, ZDP_MGMT_BIND_RSP_FMT,
                                            u8ZdpSeqNum,
                                            u8Status,
                                            (uint8)pBindingTable->u32SizeOfBindingTable,
                                            sAplZdpMgmtBindReq.u8StartIndex,
                                            maxcount);

                  if(maxcount)
                  {
					   for(u32Counter = sAplZdpMgmtBindReq.u8StartIndex,maxcount = 0; u32Counter < pBindingTable->u32SizeOfBindingTable && maxcount < 3; u32Counter++)
					   {
							   u32Location += PDUM_u16APduInstanceWriteNBO(hAPduInst, u32Location, "lbhb",
												   pBindingTable->u64SourceAddress,
												   pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u8SourceEndpoint,
												   pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u16ClusterId,
												   pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u8DstAddrMode);

							   if(pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u8DstAddrMode == 0x3)
							   {
								   u32Location += PDUM_u16APduInstanceWriteNBO(hAPduInst, u32Location, "lb",
									   pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].uDstAddress.u64Addr,
									   pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].u8DestinationEndPoint);
							   }
							   else
							   {
								   u32Location += PDUM_u16APduInstanceWriteNBO(hAPduInst, u32Location, "h",
									   pBindingTable->pvAplApsmeBindingTableEntryForSpSrcAddr[u32Counter].uDstAddress.u16Addr);
							   }
							   maxcount++;
					   }
                  }
               }
            }
            else
            {
            	u16Size =  PDUM_u16SizeNBO("bb");
            	if (PDUM_E_OK == PDUM_eAPduInstanceSetPayloadSize(hAPduInst,u16Size))
            	{
            		u32Location = PDUM_u16APduInstanceWriteNBO(hAPduInst, 0, "bb",
												u8ZdpSeqNum,
												u8Status);
            	}
            }
            if((psZdoServerEvent->uEvent.sApsDataIndEvent.u8SrcAddrMode == 0x3))
            {
                DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER,"Sending the response Extended address  status = %x \n ",
                ZPS_eAplAfUnicastIeeeAckDataReq(hAPduInst,0x8033,0,0, psZdoServerEvent->uEvent.sApsDataIndEvent.uSrcAddress.u64Addr,ZPS_E_APL_AF_SECURE_NWK,1, pu8SeqNum));
            }
            else
            {
                DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER,"Sending the response short address = %x \n", ZPS_eAplAfUnicastAckDataReq(hAPduInst,0x8033,0,0, psZdoServerEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,ZPS_E_APL_AF_SECURE_NWK,1, pu8SeqNum));
            }
        }
    } else if (psMgmtBindServerContext->sConfAckContext.eState == ZPS_E_APL_ZDO_MGMT_BIND_SERVER_STATE_WAITING_FOR_ACK_AND_CONF) {
        bEventHandled = zps_bAplZdoMgmtBindServerHandleConfirmAcks(&psMgmtBindServerContext->sConfAckContext, psZdoServerEvent, ZPS_E_APL_ZDO_MGMT_BIND_SERVER_STATE_IDLE);
        if(  ZPS_EVENT_APS_DATA_INDICATION == psZdoServerEvent->eType &&
                (0x8033 == psZdoServerEvent->uEvent.sApsDataIndEvent.u16ClusterId))
        {
            bEventHandled = TRUE;
            PDUM_eAPduFreeAPduInstance(psZdoServerEvent->uEvent.sApsDataIndEvent.hAPduInst);
        }
    }

    return bEventHandled;
}

/****************************************************************************/
/***        Local Functions                                                 */
/****************************************************************************/

/****************************************************************************
 *
 * NAME:       u8ZdoMgmtBindServerUnpackApdu
 */
/**
 * @ingroup
 *
 * @param
 *
 * @return
 *
 * @note
 *
 ****************************************************************************/
uint8 u8ZdoMgmtBindServerUnpackApdu(
    PDUM_thAPduInstance hAPduInst,
    ZPS_tsAplZdpMgmtBindReq *pReqStruct)
{
    uint32 pos = 0;
    uint8 u8SequenceNumber;

    pos =  PDUM_u16APduInstanceReadNBO(hAPduInst, 0, "b", &u8SequenceNumber);
    pos += PDUM_u16APduInstanceReadNBO(hAPduInst, pos, "b", pReqStruct);
    return u8SequenceNumber;
}

/****************************************************************************
 *
 * NAME:       zps_bAplZdoMgmtBindServerHandleConfirmAcks
 */
/**
 * @ingroup
 *
 * @param
 *
 * @return
 *
 * @note
 *
 ****************************************************************************/
bool zps_bAplZdoMgmtBindServerHandleConfirmAcks(zps_tsZdoMgmtBindServerConfAckContext* psContext, ZPS_tsAfEvent *psZdoServerEvent, uint8 eFinishState)
{
    bool bEventHandled = FALSE;

    if (ZPS_EVENT_APS_DATA_CONFIRM == psZdoServerEvent->eType && psZdoServerEvent->uEvent.sApsDataConfirmEvent.u8SequenceNum == psContext->u8SeqNum)
    {
        DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER, "ZPSAPL: ZDO Server confirmed seq num = 0x%02x\n", psContext->u8SeqNum);
        psContext->u8ConfAck |= 1;
        bEventHandled = TRUE;
    }
    if (ZPS_EVENT_APS_DATA_ACK == psZdoServerEvent->eType && psZdoServerEvent->uEvent.sApsDataAckEvent.u8SequenceNum == psContext->u8SeqNum)
    {
        DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER, "ZPSAPL: ZDO Server aps ack seq num = 0x%02x\n", psContext->u8SeqNum);
        psContext->u8ConfAck |= 2;
        bEventHandled = TRUE;
    }
    /* if confirmed and acknowledged, go back to idle state */
    if (3 == psContext->u8ConfAck)
    {
        DBG_vPrintf(TRACE_ZDO_APL_ZDO_MGMT_BIND_SERVER, "ZPSAPL: ZDO Server confirmed and acked\n", psContext->u8SeqNum);
        psContext->u8ConfAck = 0;
        psContext->eState = eFinishState;
    }

    return bEventHandled;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

