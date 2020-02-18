//
//  IWLSCD.h
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/16/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"
#ifndef IWLSCD_h
#define IWLSCD_h

static inline void iwl_scd_txq_set_chain(IWLTransport *trans,
                     u16 txq_id)
{
    //iwl_set_bits_prph(trans, SCD_QUEUECHAIN_SEL, BIT(txq_id));
    trans->iwlSetBitsPRPH(SCD_QUEUECHAIN_SEL, BIT(txq_id));
}

static inline void iwl_scd_txq_enable_agg(IWLTransport *trans,
                      u16 txq_id)
{
    trans->iwlSetBitsPRPH(SCD_AGGR_SEL, BIT(txq_id));
}

static inline void iwl_scd_txq_disable_agg(IWLTransport *trans,
                       u16 txq_id)
{
    trans->iwlSetBitsPRPH(SCD_AGGR_SEL, BIT(txq_id));
}

static inline void iwl_scd_disable_agg(IWLTransport *trans)
{
    trans->iwlSetBitsPRPH(SCD_AGGR_SEL, 0);
}

static inline void iwl_scd_activate_fifos(IWLTransport *trans)
{
    trans->iwlSetBitsPRPH(SCD_TXFACT, IWL_MASK(0, 7));
}

static inline void iwl_scd_deactivate_fifos(IWLTransport *trans)
{
    trans->iwlSetBitsPRPH(SCD_TXFACT, 0);
}

static inline void iwl_scd_enable_set_active(IWLTransport *trans,
                         u32 value)
{
   trans->iwlSetBitsPRPH(SCD_EN_CTRL, value);
}

static inline unsigned int SCD_QUEUE_WRPTR(unsigned int chnl)
{
    if (chnl < 20)
        return SCD_BASE + 0x18 + chnl * 4;
    WARN_ON_ONCE(chnl >= 32);
    return SCD_BASE + 0x284 + (chnl - 20) * 4;
}

static inline unsigned int SCD_QUEUE_RDPTR(unsigned int chnl)
{
    if (chnl < 20)
        return SCD_BASE + 0x68 + chnl * 4;
    WARN_ON_ONCE(chnl >= 32);
    return SCD_BASE + 0x2B4 + chnl * 4;
}

static inline unsigned int SCD_QUEUE_STATUS_BITS(unsigned int chnl)
{
    if (chnl < 20)
        return SCD_BASE + 0x10c + chnl * 4;
    WARN_ON_ONCE(chnl >= 32);
    return SCD_BASE + 0x334 + chnl * 4;
}

static inline void iwl_scd_txq_set_inactive(IWLTransport *trans,
                        u16 txq_id)
{
    trans->iwlSetBitsPRPH(SCD_QUEUE_STATUS_BITS(txq_id),
               (0 << SCD_QUEUE_STTS_REG_POS_ACTIVE)|
               (1 << SCD_QUEUE_STTS_REG_POS_SCD_ACT_EN));
}

#endif /* IWLSCD_h */
