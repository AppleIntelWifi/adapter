//
//  IWLUtils.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmDriver.hpp"

/*
 * Will return 0 even if the cmd failed when RFKILL is asserted unless
 * CMD_WANT_SKB is set in cmd->flags.
 */
int IWLMvmDriver::sendCmd(struct iwl_host_cmd *cmd)
{
    int ret;
    /*
     * Synchronous commands from this op-mode must hold
     * the mutex, this ensures we don't try to send two
     * (or more) synchronous commands at a time.
     */
    //TODO
    //    if (!(cmd->flags & CMD_ASYNC))
    //        lockdep_assert_held(&mvm->mutex);
    ret = trans->sendCmd(cmd);
    /*
     * If the caller wants the SKB, then don't hide any problems, the
     * caller might access the response buffer which will be NULL if
     * the command failed.
     */
    if (cmd->flags & CMD_WANT_SKB)
        return ret;
    /* Silently ignore failures if RFKILL is asserted */
    if (!ret || ret == -ERFKILL)
        return 0;
    return ret;
}

int IWLMvmDriver::sendCmdPdu(u32 id, u32 flags, u16 len, const void *data)
{
    struct iwl_host_cmd cmd = {
        .id = id,
        .len = { len, },
        .data = { data, },
        .flags = flags,
    };
    return sendCmd(&cmd);
}

int IWLMvmDriver::sendCmdPduStatus(u32 id, u16 len, const void* data, u32* status)
{
    struct iwl_host_cmd cmd = {
        .id = id,
        .len = { len, },
        .data = { data, }
    };
    
    return sendCmdStatus(&cmd, status);
}

int IWLMvmDriver::sendCmdStatus(struct iwl_host_cmd *cmd, u32 *status)
{
    struct iwl_rx_packet *pkt;
    struct iwl_cmd_response *resp;
    int ret, resp_len;
    //    lockdep_assert_held(&mvm->mutex);
    /*
     * Only synchronous commands can wait for status,
     * we use WANT_SKB so the caller can't.
     */
    if (cmd->flags & (CMD_ASYNC | CMD_WANT_SKB)) {
        IWL_ERR(0, "cmd flags %x", cmd->flags);
        return -EINVAL;
    }
    cmd->flags |= CMD_WANT_SKB;
    ret = trans->sendCmd(cmd);
    if (ret == -ERFKILL) {
        /*
         * The command failed because of RFKILL, don't update
         * the status, leave it as success and return 0.
         */
        return 0;
    } else if (ret) {
        return ret;
    }
    pkt = cmd->resp_pkt;
    resp_len = iwl_rx_packet_payload_len(pkt);
    if (WARN_ON_ONCE(resp_len != sizeof(*resp))) {
        ret = -EIO;
        goto out_free_resp;
    }
    resp = (struct iwl_cmd_response *)pkt->data;
    *status = le32_to_cpu(resp->status);
out_free_resp:
    trans->freeResp(cmd);
    return ret;
}
