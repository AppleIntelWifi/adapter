//
//  IWLTransport.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransport_hpp
#define IWLTransport_hpp

#include "IWLTransOps.h"
#include "IWLIO.hpp"

/**
 * enum iwl_trans_status: transport status flags
 * @STATUS_SYNC_HCMD_ACTIVE: a SYNC command is being processed
 * @STATUS_DEVICE_ENABLED: APM is enabled
 * @STATUS_TPOWER_PMI: the device might be asleep (need to wake it up)
 * @STATUS_INT_ENABLED: interrupts are enabled
 * @STATUS_RFKILL_HW: the actual HW state of the RF-kill switch
 * @STATUS_RFKILL_OPMODE: RF-kill state reported to opmode
 * @STATUS_FW_ERROR: the fw is in error state
 * @STATUS_TRANS_GOING_IDLE: shutting down the trans, only special commands
 *    are sent
 * @STATUS_TRANS_IDLE: the trans is idle - general commands are not to be sent
 * @STATUS_TRANS_DEAD: trans is dead - avoid any read/write operation
 */
enum iwl_trans_status {
    STATUS_SYNC_HCMD_ACTIVE,
    STATUS_DEVICE_ENABLED,
    STATUS_TPOWER_PMI,
    STATUS_INT_ENABLED,
    STATUS_RFKILL_HW,
    STATUS_RFKILL_OPMODE,
    STATUS_FW_ERROR,
    STATUS_TRANS_GOING_IDLE,
    STATUS_TRANS_IDLE,
    STATUS_TRANS_DEAD,
};

struct iwl_op_mode;

struct iwl_hcmd_names {
    u8 cmd_id;
    const char *const cmd_name;
};

#define HCMD_NAME(x)    \
    { .cmd_id = x, .cmd_name = #x }

struct iwl_hcmd_arr {
    const struct iwl_hcmd_names *arr;
    int size;
};

#define HCMD_ARR(x)    \
    { .arr = x, .size = ARRAY_SIZE(x) }

/**
 * struct iwl_trans_config - transport configuration
 *
 * @op_mode: pointer to the upper layer.
 * @cmd_queue: the index of the command queue.
 *    Must be set before start_fw.
 * @cmd_fifo: the fifo for host commands
 * @cmd_q_wdg_timeout: the timeout of the watchdog timer for the command queue.
 * @no_reclaim_cmds: Some devices erroneously don't set the
 *    SEQ_RX_FRAME bit on some notifications, this is the
 *    list of such notifications to filter. Max length is
 *    %MAX_NO_RECLAIM_CMDS.
 * @n_no_reclaim_cmds: # of commands in list
 * @rx_buf_size: RX buffer size needed for A-MSDUs
 *    if unset 4k will be the RX buffer size
 * @bc_table_dword: set to true if the BC table expects the byte count to be
 *    in DWORD (as opposed to bytes)
 * @scd_set_active: should the transport configure the SCD for HCMD queue
 * @sw_csum_tx: transport should compute the TCP checksum
 * @command_groups: array of command groups, each member is an array of the
 *    commands in the group; for debugging only
 * @command_groups_size: number of command groups, to avoid illegal access
 * @cb_data_offs: offset inside skb->cb to store transport data at, must have
 *    space for at least two pointers
 */
struct iwl_trans_config {
    struct iwl_op_mode *op_mode;

    u8 cmd_queue;
    u8 cmd_fifo;
    unsigned int cmd_q_wdg_timeout;
    const u8 *no_reclaim_cmds;
    unsigned int n_no_reclaim_cmds;

    enum iwl_amsdu_size rx_buf_size;
    bool bc_table_dword;
    bool scd_set_active;
    bool sw_csum_tx;
    const struct iwl_hcmd_arr *command_groups;
    int command_groups_size;

    u8 cb_data_offs;
};

class IWLTransport : public IWLIO
{
    
public:
    IWLTransport();
    ~IWLTransport();
    
    bool init(IWLDevice *device);
    
    void release();
    
    bool finishNicInit();
   
    ///
    ///msix
    void initMsix();
    
    void configMsixHw();
    
    
    
    
    void disableIntr();
    
    void enableIntr();
    
    IWLTransOps *getTransOps();
    
    ///
    ///power
    void setPMI(bool state);
    
private:
    
    
private:
    //a bit-mask of transport status flags
    unsigned long status;
    IWLTransOps *trans_ops;
    iwl_trans_config trans_config;
    
    
    //msix
    u32 fh_init_mask;
    u32 hw_init_mask;
    u32 fh_mask;
    u32 hw_mask;
    
    //interrupt
    u32 inta_mask;
    
    IOSimpleLock *irq_lock;
    bool msix_enabled;
    
};

#endif /* IWLTransport_hpp */
