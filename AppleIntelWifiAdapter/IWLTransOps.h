//
//  IWLTransOps.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransOps_h
#define IWLTransOps_h

#include <linux/types.h>
#include "fw/api/cmdhdr.h"
#include "IWLIO.hpp"
#include "IWLFH.h"

#define FH_RSCSR_FRAME_SIZE_MSK        0x00003FFF    /* bits 0-13 */
#define FH_RSCSR_FRAME_INVALID        0x55550000
#define FH_RSCSR_FRAME_ALIGN        0x40
#define FH_RSCSR_RPA_EN            BIT(25)
#define FH_RSCSR_RADA_EN        BIT(26)
#define FH_RSCSR_RXQ_POS        16
#define FH_RSCSR_RXQ_MASK        0x3F0000

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
    unsigned long _rx_page_addr;
    u32 _rx_page_order;

    u32 flags;
    u32 id;
    u16 len[IWL_MAX_CMD_TBS_PER_TFD];
    u8 dataflags[IWL_MAX_CMD_TBS_PER_TFD];
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

class IWLTransOps {
    
public:
    
    IWLTransOps() {}
    IWLTransOps(IWLIO *io) {
        this->io = io;
    }
    ~IWLTransOps() {}
    
    bool prepareCardHW();

    bool apmInit();
    
    void apmStop();
    
    virtual bool startHW() = 0;
    
    virtual void fwAlive(UInt32 scd_addr) = 0;
    
    virtual bool startFW() = 0;
    
    virtual void stopDevice() = 0;
    
    virtual void sendCmd(iwl_host_cmd *cmd) = 0;
    
private:
    
    int setHWReady();
    
protected:
    IWLIO *io;
    
    
private:
    
};

#endif /* IWLTransOps_h */
