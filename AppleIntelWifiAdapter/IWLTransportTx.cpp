//
//  IWLTransportTx.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"

extern const void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));

#define HOST_COMPLETE_TIMEOUT    (2 * HZ * CPTCFG_IWL_TIMEOUT_FACTOR)

/* Comparator for struct iwl_hcmd_names.
 * Used in the binary search over a list of host commands.
 *
 * @key: command_id that we're looking for.
 * @elt: struct iwl_hcmd_names candidate for match.
 *
 * @return 0 iff equal.
 */
static int iwl_hcmd_names_cmp(const void *key, const void *elt)
{
    const struct iwl_hcmd_names *name = (const struct iwl_hcmd_names *)elt;
    u8 cmd1 = *(u8 *)key;
    u8 cmd2 = name->cmd_id;

    return (cmd1 - cmd2);
}

const char *iwl_get_cmd_string(IWLTransport *trans, u32 id)
{
    u8 grp, cmd;
    struct iwl_hcmd_names *ret;
    const struct iwl_hcmd_arr *arr;
    size_t size = sizeof(struct iwl_hcmd_names);

    grp = iwl_cmd_groupid(id);
    cmd = iwl_cmd_opcode(id);

    if (!trans->command_groups || grp >= trans->command_groups_size ||
        !trans->command_groups[grp].arr)
        return "UNKNOWN";

    arr = &trans->command_groups[grp];
    ret = (struct iwl_hcmd_names *)bsearch((const void *)&cmd, arr->arr, arr->size, size, iwl_hcmd_names_cmp);
    if (!ret)
        return "UNKNOWN";
    return ret->cmd_name;
}

int iwl_cmd_groups_verify_sorted(const struct iwl_trans_config *trans)
{
    int i, j;
    const struct iwl_hcmd_arr *arr;

    for (i = 0; i < trans->command_groups_size; i++) {
        arr = &trans->command_groups[i];
        if (!arr->arr)
            continue;
        for (j = 0; j < arr->size - 1; j++)
            if (arr->arr[j].cmd_id > arr->arr[j + 1].cmd_id)
                return -1;
    }
    return 0;
}

static int iwl_pcie_send_hcmd_sync(IWLTransport *trans,
                   struct iwl_host_cmd *cmd)
{
//    struct iwl_txq *txq = trans_pcie->txq[trans_pcie->cmd_queue];
//    int cmd_idx;
//    int ret;
//
//    IWL_INFO(trans, "Attempting to send sync command %s\n",
//               iwl_get_cmd_string(trans, cmd->id));
//
//    if (test_and_set_bit(STATUS_SYNC_HCMD_ACTIVE,
//                         &trans->status)){
//        IWL_INFO(0, "Command %s: a command is already active!\n",
//                 iwl_get_cmd_string(trans, cmd->id));
//    }
//        return -EIO;
//
//    IWL_INFO(trans, "Setting HCMD_ACTIVE for command %s\n",
//               iwl_get_cmd_string(trans, cmd->id));
//
//    cmd_idx = iwl_pcie_enqueue_hcmd(trans, cmd);
//    if (cmd_idx < 0) {
//        ret = cmd_idx;
//        clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
//        IWL_ERR(trans,
//            "Error sending %s: enqueue_hcmd failed: %d\n",
//            iwl_get_cmd_string(trans, cmd->id), ret);
//        return ret;
//    }
//
//    ret = wait_event_timeout(trans_pcie->wait_command_queue,
//                 !test_bit(STATUS_SYNC_HCMD_ACTIVE,
//                       &trans->status),
//                 HOST_COMPLETE_TIMEOUT);
//    if (!ret) {
//        IWL_ERR(trans, "Error sending %s: time out after %dms.\n",
//            iwl_get_cmd_string(trans, cmd->id),
//            jiffies_to_msecs(HOST_COMPLETE_TIMEOUT));
//
//        IWL_ERR(trans, "Current CMD queue read_ptr %d write_ptr %d\n",
//            txq->read_ptr, txq->write_ptr);
//
//        clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
//        IWL_INFO(trans, "Clearing HCMD_ACTIVE for command %s\n",
//                   iwl_get_cmd_string(trans, cmd->id));
//        ret = -ETIMEDOUT;
//
//        iwl_trans_pcie_sync_nmi(trans);
//        goto cancel;
//    }
//
//    if (test_bit(STATUS_FW_ERROR, &trans->status)) {
//        iwl_trans_pcie_dump_regs(trans);
//        IWL_ERR(trans, "FW error in SYNC CMD %s\n",
//            iwl_get_cmd_string(trans, cmd->id));
//        dump_stack();
//        ret = -EIO;
//        goto cancel;
//    }
//
//    if (!(cmd->flags & CMD_SEND_IN_RFKILL) &&
//        test_bit(STATUS_RFKILL_OPMODE, &trans->status)) {
//        IWL_INFO(trans, "RFKILL in SYNC CMD... no rsp\n");
//        ret = -ERFKILL;
//        goto cancel;
//    }
//
//    if ((cmd->flags & CMD_WANT_SKB) && !cmd->resp_pkt) {
//        IWL_ERR(trans, "Error: Response NULL in '%s'\n",
//            iwl_get_cmd_string(trans, cmd->id));
//        ret = -EIO;
//        goto cancel;
//    }

    return 0;

//cancel:
//    if (cmd->flags & CMD_WANT_SKB) {
//        /*
//         * Cancel the CMD_WANT_SKB flag for the cmd in the
//         * TX cmd queue. Otherwise in case the cmd comes
//         * in later, it will possibly set an invalid
//         * address (cmd->meta.source).
//         */
//        txq->entries[cmd_idx].meta.flags &= ~CMD_WANT_SKB;
//    }
//
//    if (cmd->resp_pkt) {
//        trans->freeResp(cmd);
//        cmd->resp_pkt = NULL;
//    }
//
//    return ret;
}

static int iwl_pcie_send_hcmd_async(IWLTransport *trans,
                    struct iwl_host_cmd *cmd)
{
    int ret;

    /* An asynchronous command can not expect an SKB to be set. */
    if (WARN_ON(cmd->flags & CMD_WANT_SKB))
        return -EINVAL;

    //TODO
//    ret = iwl_pcie_enqueue_hcmd(trans, cmd);
    if (ret < 0) {
        IWL_ERR(trans,
            "Error sending %s: enqueue_hcmd failed: %d\n",
            iwl_get_cmd_string(trans, cmd->id), ret);
        return ret;
    }
    return 0;
}

int IWLTransport::pcieSendHCmd(iwl_host_cmd *cmd)
{
    /* Make sure the NIC is still alive in the bus */
    if (test_bit(STATUS_TRANS_DEAD, &this->status))
        return -ENODEV;
    
    if (!(cmd->flags & CMD_SEND_IN_RFKILL) &&
        test_bit(STATUS_RFKILL_OPMODE, &this->status)) {
        IWL_INFO(trans, "Dropping CMD 0x%x: RF KILL\n",
                 cmd->id);
        return -ERFKILL;
    }
    
    if (cmd->flags & CMD_ASYNC)
        return iwl_pcie_send_hcmd_async(this, cmd);
    
    /* We still can fail on RFKILL that can be asserted while we wait */
    return iwl_pcie_send_hcmd_sync(this, cmd);
}

void IWLTransport::txStop()
{
    
}
