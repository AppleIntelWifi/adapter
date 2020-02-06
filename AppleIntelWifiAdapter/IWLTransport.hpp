//
//  IWLTransport.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransport_hpp
#define IWLTransport_hpp

#include "IWLIO.hpp"
#include "IWLDevice.hpp"
#include "IWLFH.h"
#include "fw/api/cmdhdr.h"

#define FH_RSCSR_FRAME_SIZE_MSK        0x00003FFF    /* bits 0-13 */
#define FH_RSCSR_FRAME_INVALID        0x55550000
#define FH_RSCSR_FRAME_ALIGN        0x40
#define FH_RSCSR_RPA_EN            BIT(25)
#define FH_RSCSR_RADA_EN        BIT(26)
#define FH_RSCSR_RXQ_POS        16
#define FH_RSCSR_RXQ_MASK        0x3F0000

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

struct iwl_rx_packet {
    /*
     * The first 4 bytes of the RX frame header contain both the RX frame
     * size and some flags.
     * Bit fields:
     * 31:    flag flush RB request
     * 30:    flag ignore TC (terminal counter) request
     * 29:    flag fast IRQ request
     * 28-27: Reserved
     * 26:    RADA enabled
     * 25:    Offload enabled
     * 24:    RPF enabled
     * 23:    RSS enabled
     * 22:    Checksum enabled
     * 21-16: RX queue
     * 15-14: Reserved
     * 13-00: RX frame size
     */
    __le32 len_n_flags;
    struct iwl_cmd_header hdr;
    u8 data[];
} __packed;

static inline u32 iwl_rx_packet_len(const struct iwl_rx_packet *pkt)
{
    return le32_to_cpu(pkt->len_n_flags) & FH_RSCSR_FRAME_SIZE_MSK;
}

static inline u32 iwl_rx_packet_payload_len(const struct iwl_rx_packet *pkt)
{
    return iwl_rx_packet_len(pkt) - sizeof(pkt->hdr);
}

/**
 * enum CMD_MODE - how to send the host commands ?
 *
 * @CMD_ASYNC: Return right away and don't wait for the response
 * @CMD_WANT_SKB: Not valid with CMD_ASYNC. The caller needs the buffer of
 *    the response. The caller needs to call iwl_free_resp when done.
 * @CMD_WANT_ASYNC_CALLBACK: the op_mode's async callback function must be
 *    called after this command completes. Valid only with CMD_ASYNC.
 */
enum CMD_MODE {
    CMD_ASYNC        = BIT(0),
    CMD_WANT_SKB        = BIT(1),
    CMD_SEND_IN_RFKILL    = BIT(2),
    CMD_WANT_ASYNC_CALLBACK    = BIT(3),
};

#define DEF_CMD_PAYLOAD_SIZE 320

/**
 * struct iwl_device_cmd
 *
 * For allocation of the command and tx queues, this establishes the overall
 * size of the largest command we send to uCode, except for commands that
 * aren't fully copied and use other TFD space.
 */
struct iwl_device_cmd {
    union {
        struct {
            struct iwl_cmd_header hdr;    /* uCode API */
            u8 payload[DEF_CMD_PAYLOAD_SIZE];
        };
        struct {
            struct iwl_cmd_header_wide hdr_wide;
            u8 payload_wide[DEF_CMD_PAYLOAD_SIZE -
                    sizeof(struct iwl_cmd_header_wide) +
                    sizeof(struct iwl_cmd_header)];
        };
    };
} __packed;

/**
 * struct iwl_device_tx_cmd - buffer for TX command
 * @hdr: the header
 * @payload: the payload placeholder
 *
 * The actual structure is sized dynamically according to need.
 */
struct iwl_device_tx_cmd {
    struct iwl_cmd_header hdr;
    u8 payload[];
} __packed;

#define TFD_MAX_PAYLOAD_SIZE (sizeof(struct iwl_device_cmd))

/*
 * number of transfer buffers (fragments) per transmit frame descriptor;
 * this is just the driver's idea, the hardware supports 20
 */
#define IWL_MAX_CMD_TBS_PER_TFD    2

/**
 * enum iwl_hcmd_dataflag - flag for each one of the chunks of the command
 *
 * @IWL_HCMD_DFL_NOCOPY: By default, the command is copied to the host command's
 *    ring. The transport layer doesn't map the command's buffer to DMA, but
 *    rather copies it to a previously allocated DMA buffer. This flag tells
 *    the transport layer not to copy the command, but to map the existing
 *    buffer (that is passed in) instead. This saves the memcpy and allows
 *    commands that are bigger than the fixed buffer to be submitted.
 *    Note that a TFD entry after a NOCOPY one cannot be a normal copied one.
 * @IWL_HCMD_DFL_DUP: Only valid without NOCOPY, duplicate the memory for this
 *    chunk internally and free it again after the command completes. This
 *    can (currently) be used only once per command.
 *    Note that a TFD entry after a DUP one cannot be a normal copied one.
 */
enum iwl_hcmd_dataflag {
    IWL_HCMD_DFL_NOCOPY    = BIT(0),
    IWL_HCMD_DFL_DUP    = BIT(1),
};

enum iwl_error_event_table_status {
    IWL_ERROR_EVENT_TABLE_LMAC1 = BIT(0),
    IWL_ERROR_EVENT_TABLE_LMAC2 = BIT(1),
    IWL_ERROR_EVENT_TABLE_UMAC = BIT(2),
};

/**
 * struct iwl_host_cmd - Host command to the uCode
 *
 * @data: array of chunks that composes the data of the host command
 * @resp_pkt: response packet, if %CMD_WANT_SKB was set
 * @_rx_page_order: (internally used to free response packet)
 * @_rx_page_addr: (internally used to free response packet)
 * @flags: can be CMD_*
 * @len: array of the lengths of the chunks in data
 * @dataflags: IWL_HCMD_DFL_*
 * @id: command id of the host command, for wide commands encoding the
 *    version and group as well
 */
struct iwl_host_cmd {
    const void *data[IWL_MAX_CMD_TBS_PER_TFD];
    struct iwl_rx_packet *resp_pkt;
    size_t resp_pkt_len;//added by zxy
    unsigned long _rx_page_addr;
    u32 _rx_page_order;

    u32 flags;
    u32 id;
    u16 len[IWL_MAX_CMD_TBS_PER_TFD];
    u8 dataflags[IWL_MAX_CMD_TBS_PER_TFD];
};

struct iwl_rx_cmd_buffer {
    struct page *_page;
    int _offset;
    bool _page_stolen;
    u32 _rx_page_order;
    unsigned int truesize;
};

/* Find TFD CB base pointer for given queue */
static inline unsigned int FH_MEM_CBBC_QUEUE(IWLDevice *trans,
                         unsigned int chnl)
{
    if (trans->cfg->trans.use_tfh) {
        WARN_ON_ONCE(chnl >= 64);
        return TFH_TFDQ_CBB_TABLE + 8 * chnl;
    }
    if (chnl < 16)
        return FH_MEM_CBBC_0_15_LOWER_BOUND + 4 * chnl;
    if (chnl < 20)
        return FH_MEM_CBBC_16_19_LOWER_BOUND + 4 * (chnl - 16);
    WARN_ON_ONCE(chnl >= 32);
    return FH_MEM_CBBC_20_31_LOWER_BOUND + 4 * (chnl - 20);
}

/**
 * enum iwl_trans_state - state of the transport layer
 *
 * @IWL_TRANS_NO_FW: no fw has sent an alive response
 * @IWL_TRANS_FW_ALIVE: a fw has sent an alive response
 */
enum iwl_trans_state {
    IWL_TRANS_NO_FW = 0,
    IWL_TRANS_FW_ALIVE    = 1,
};

#define MAX_NO_RECLAIM_CMDS    6

#define IWL_MASK(lo, hi) ((1 << (hi)) | ((1 << (hi)) - (1 << (lo))))

/*
 * Maximum number of HW queues the transport layer
 * currently supports
 */
#define IWL_MAX_HW_QUEUES        32
#define IWL_MAX_TID_COUNT    8
#define IWL_FRAME_LIMIT    64
#define IWL_MAX_RX_HW_QUEUES    16

/**
 * enum iwl_wowlan_status - WoWLAN image/device status
 * @IWL_D3_STATUS_ALIVE: firmware is still running after resume
 * @IWL_D3_STATUS_RESET: device was reset while suspended
 */
enum iwl_d3_status {
    IWL_D3_STATUS_ALIVE,
    IWL_D3_STATUS_RESET,
};

class IWLTransport : public IWLIO
{
    
public:
    IWLTransport();
    ~IWLTransport();
    
    bool init(IWLDevice *device);
    
    bool prepareCardHW();
    
    void release();
    
    bool finishNicInit();
    
    ///
    ///msix
    void initMsix();
    
    void configMsixHw();
    
    int clearPersistenceBit();
    
    
    ///apm
    void apmConfig();
    
    void swReset();
    /*
    * Enable LP XTAL to avoid HW bug where device may consume much power if
    * FW is not loaded after device reset. LP XTAL is disabled by default
    * after device HW reset. Do it only if XTAL is fed by internal source.
    * Configure device's "persistence" mode to avoid resetting XTAL again when
    * SHRD_HW_RST occurs in S3.
    */
    void apmLpXtalEnable();
    
    void apmStopMaster();
    
    void disableIntr();
    
    void enableIntr();
    
    void enableRFKillIntr();
    
    void enableFWLoadIntr();
    
    bool isRFKikkSet();
    
    ///fw
    void loadFWChunkFh(u32 dst_addr, dma_addr_t phy_addr, u32 byte_cnt);
    
    int loadFWChunk(u32 dst_addr, dma_addr_t phy_addr,
                    u32 byte_cnt);
    
    int loadSection(u8 section_num, const struct fw_desc *section);
    
    int loadCPUSections8000(const struct fw_img *image, int cpu, int *first_ucode_section);
    
    int loadCPUSections(const struct fw_img *image, int cpu, int *first_ucode_section);
    
    int loadGivenUcode(const struct fw_img *image);
    
    int loadGivenUcode8000(const struct fw_img *image);
    
    ///
    ///power
    void setPMI(bool state);
    
    //rx
    void disableICT();
    
    void rxStop();
    
    //tx
    void freeResp(struct iwl_host_cmd *cmd);
    
    int pcieSendHCmd(iwl_host_cmd *cmd);
    
    void txStop();
    
    //cmd
    int sendCmd(struct iwl_host_cmd *cmd);
    
public:
    //a bit-mask of transport status flags
    unsigned long status;
    IOSimpleLock *irq_lock;
    IOLock *mutex;
    
    //fw
    enum iwl_trans_state state;
    
    //tx
    int cmd_queue;
    bool wide_cmd_header;
    IOLock *syncCmdLock;//sync and async lock
    const struct iwl_hcmd_arr *command_groups;
    int command_groups_size;
    int cmd_fifo;
    
    //rx
    bool use_ict;
    
    bool opmode_down;
    bool is_down;
    
    
private:
    
    int setHWReady();
    
    void enableHWIntrMskMsix(u32 msk);
    
private:
    
    iwl_trans_config trans_config;
    
    
    //msix
    u32 fh_init_mask;
    u32 hw_init_mask;
    u32 fh_mask;
    u32 hw_mask;
    bool msix_enabled;
    
    //interrupt
    u32 inta_mask;
    
    ///fw
    bool ucode_write_complete;
    
};

#endif /* IWLTransport_hpp */
