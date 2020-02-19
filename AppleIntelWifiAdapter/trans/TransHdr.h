//
//  TransHdr.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef TransHdr_h
#define TransHdr_h

#include <linux/types.h>
#include "IWLFH.h"
#include "../fw/api/cmdhdr.h"
#include "IWLInternal.hpp"

#include <sys/kernel_types.h>
#include <sys/queue.h>

#define FH_RSCSR_FRAME_SIZE_MSK        0x00003FFF    /* bits 0-13 */
#define FH_RSCSR_FRAME_INVALID        0x55550000
#define FH_RSCSR_FRAME_ALIGN        0x40
#define FH_RSCSR_RPA_EN            BIT(25)
#define FH_RSCSR_RADA_EN        BIT(26)
#define FH_RSCSR_RXQ_POS        16
#define FH_RSCSR_RXQ_MASK        0x3F0000

/**
 * struct isr_statistics - interrupt statistics
 *
 */
struct isr_statistics {
    u32 hw;
    u32 sw;
    u32 err_code;
    u32 sch;
    u32 alive;
    u32 rfkill;
    u32 ctkill;
    u32 wakeup;
    u32 rx;
    u32 tx;
    u32 unhandled;
};

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
    CMD_HIGH_PRIO        = BIT(3),
    CMD_SEND_IN_IDLE    = BIT(4),
    CMD_MAKE_TRANS_IDLE    = BIT(5),
    CMD_WAKE_UP_TRANS    = BIT(6),
    CMD_WANT_ASYNC_CALLBACK    = BIT(7),
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
#define IWL_MAX_TVQM_QUEUES        512

#define IWL_MAX_TID_COUNT    8
#define IWL_MGMT_TID        15
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

/**
 * struct iwl_rx_transfer_desc - transfer descriptor
 * @addr: ptr to free buffer start address
 * @rbid: unique tag of the buffer
 * @reserved: reserved
 */
struct iwl_rx_transfer_desc {
    __le16 rbid;
    __le16 reserved[3];
    __le64 addr;
} __packed;

#define IWL_RX_CD_FLAGS_FRAGMENTED    BIT(0)

/**
 * struct iwl_rx_completion_desc - completion descriptor
 * @reserved1: reserved
 * @rbid: unique tag of the received buffer
 * @flags: flags (0: fragmented, all others: reserved)
 * @reserved2: reserved
 */
struct iwl_rx_completion_desc {
    __le32 reserved1;
    __le16 rbid;
    u8 flags;
    u8 reserved2[25];
} __packed;

#include <IOKit/network/IOMbufMemoryCursor.h>

/**
 * struct iwl_rx_mem_buffer
 * @page_dma: bus address of rxb page
 * @page: driver's pointer to the rxb page
 * @invalid: rxb is in driver ownership - not owned by HW
 * @vid: index of this rxb in the global table
 * @offset: indicates which offset of the page (in bytes)
 *    this buffer uses (if multiple RBs fit into one page)
 */
struct iwl_rx_mem_buffer {
    dma_addr_t page_dma;
    mbuf_t page;
    u16 vid;
    bool invalid;
    TAILQ_ENTRY(iwl_rx_mem_buffer) list;
    u32 offset;
    IOMbufNaturalMemoryCursor *cursor;
    IOMemoryCursor::PhysicalSegment vec;
};

/**
 * struct iwl_rxq - Rx queue
 * @id: queue index
 * @bd: driver's pointer to buffer of receive buffer descriptors (rbd).
 *    Address size is 32 bit in pre-9000 devices and 64 bit in 9000 devices.
 *    In AX210 devices it is a pointer to a list of iwl_rx_transfer_desc's
 * @bd_dma: bus address of buffer of receive buffer descriptors (rbd)
 * @ubd: driver's pointer to buffer of used receive buffer descriptors (rbd)
 * @ubd_dma: physical address of buffer of used receive buffer descriptors (rbd)
 * @tr_tail: driver's pointer to the transmission ring tail buffer
 * @tr_tail_dma: physical address of the buffer for the transmission ring tail
 * @cr_tail: driver's pointer to the completion ring tail buffer
 * @cr_tail_dma: physical address of the buffer for the completion ring tail
 * @read: Shared index to newest available Rx buffer
 * @write: Shared index to oldest written Rx packet
 * @free_count: Number of pre-allocated buffers in rx_free
 * @used_count: Number of RBDs handled to allocator to use for allocation
 * @write_actual:
 * @rx_free: list of RBDs with allocated RB ready for use
 * @rx_used: list of RBDs with no RB attached
 * @need_update: flag to indicate we need to update read/write index
 * @rb_stts: driver's pointer to receive buffer status
 * @rb_stts_dma: bus address of receive buffer status
 * @lock:
 * @queue: actual rx queue. Not used for multi-rx queue.
 *
 * NOTE:  rx_free and rx_used are used as a FIFO for iwl_rx_mem_buffers
 */
struct iwl_rxq {
    int id;
    void *bd;
    dma_addr_t bd_dma;
    iwl_dma_ptr *bd_dma_ptr;
    union {
        void *used_bd;
        __le32 *bd_32;
        struct iwl_rx_completion_desc *cd;
    };
    dma_addr_t used_bd_dma;
    iwl_dma_ptr *used_bd_dma_ptr;
    __le16 *tr_tail;
    dma_addr_t tr_tail_dma;
    iwl_dma_ptr *tr_tail_dma_ptr;
    __le16 *cr_tail;
    dma_addr_t cr_tail_dma;
    iwl_dma_ptr *cr_tail_dma_ptr;
    u32 read;
    u32 write;
    u32 free_count;
    u32 used_count;
    u32 write_actual;
    u32 queue_size;
    TAILQ_HEAD(, iwl_rx_mem_buffer) rx_free;
    TAILQ_HEAD(, iwl_rx_mem_buffer) rx_used;
    bool need_update;
    iwl_rb_status *rb_stts;
    dma_addr_t rb_stts_dma;
    iwl_dma_ptr *rb_stts_dma_ptr;
    IOSimpleLock* lock;
    struct iwl_rx_mem_buffer *queue[RX_QUEUE_SIZE];
};

struct iwl_cmd_meta {
    /* only for SYNC commands, iff the reply skb is wanted */
    struct iwl_host_cmd *source;
    u32 flags;
    u32 tbs;
    
    struct iwl_dma_ptr *dma[IWL_MAX_CMD_TBS_PER_TFD + 1];
};

/*
 * The FH will write back to the first TB only, so we need to copy some data
 * into the buffer regardless of whether it should be mapped or not.
 * This indicates how big the first TB must be to include the scratch buffer
 * and the assigned PN.
 * Since PN location is 8 bytes at offset 12, it's 20 now.
 * If we make it bigger then allocations will be bigger and copy slower, so
 * that's probably not useful.
 */
#define IWL_FIRST_TB_SIZE    20
#define IWL_FIRST_TB_SIZE_ALIGN LNX_ALIGN(IWL_FIRST_TB_SIZE, 64)

struct iwl_pcie_txq_entry {
    void *cmd;
    mbuf_t skb;
    /* buffer to free after command completes */
    const void *free_buf;
    struct iwl_cmd_meta meta;
};

struct iwl_pcie_first_tb_buf {
    u8 buf[IWL_FIRST_TB_SIZE_ALIGN];
};

/**
 * struct iwl_txq - Tx Queue for DMA
 * @q: generic Rx/Tx queue descriptor
 * @tfds: transmit frame descriptors (DMA memory)
 * @first_tb_bufs: start of command headers, including scratch buffers, for
 *    the writeback -- this is DMA memory and an array holding one buffer
 *    for each command on the queue
 * @first_tb_dma: DMA address for the first_tb_bufs start
 * @entries: transmit entries (driver state)
 * @lock: queue lock
 * @stuck_timer: timer that fires if queue gets stuck
 * @trans_pcie: pointer back to transport (for timer)
 * @need_update: indicates need to update read/write index
 * @ampdu: true if this queue is an ampdu queue for an specific RA/TID
 * @wd_timeout: queue watchdog timeout (jiffies) - per queue
 * @frozen: tx stuck queue timer is frozen
 * @frozen_expiry_remainder: remember how long until the timer fires
 * @bc_tbl: byte count table of the queue (relevant only for gen2 transport)
 * @write_ptr: 1-st empty entry (index) host_w
 * @read_ptr: last used entry (index) host_r
 * @dma_addr:  physical addr for BD's
 * @n_window: safe queue window
 * @id: queue id
 * @low_mark: low watermark, resume queue if free space more than this
 * @high_mark: high watermark, stop queue if free space less than this
 *
 * A Tx queue consists of circular buffer of BDs (a.k.a. TFDs, transmit frame
 * descriptors) and required locking structures.
 *
 * Note the difference between TFD_QUEUE_SIZE_MAX and n_window: the hardware
 * always assumes 256 descriptors, so TFD_QUEUE_SIZE_MAX is always 256 (unless
 * there might be HW changes in the future). For the normal TX
 * queues, n_window, which is the size of the software queue data
 * is also 256; however, for the command queue, n_window is only
 * 32 since we don't need so many commands pending. Since the HW
 * still uses 256 BDs for DMA though, TFD_QUEUE_SIZE_MAX stays 256.
 * This means that we end up with the following:
 *  HW entries: | 0 | ... | N * 32 | ... | N * 32 + 31 | ... | 255 |
 *  SW entries:           | 0      | ... | 31          |
 * where N is a number between 0 and 7. This means that the SW
 * data is a window overlayed over the HW queue.
 */
struct iwl_txq {
    void *tfds;
    struct iwl_dma_ptr* tfds_dma;
    struct iwl_pcie_first_tb_buf *first_tb_bufs;
    dma_addr_t first_tb_dma;
    struct iwl_dma_ptr* first_tb_dma_ptr;
    struct iwl_pcie_txq_entry *entries;
    IOSimpleLock *lock;
    unsigned long frozen_expiry_remainder;
//    struct timer_list stuck_timer;
    struct iwl_trans_pcie *trans_pcie;
    bool need_update;
    bool frozen;
    bool ampdu;
    int block;
    unsigned long wd_timeout;
    mbuf_t overflow_q;
    struct iwl_dma_ptr bc_tbl;

    int write_ptr;
    int read_ptr;
    dma_addr_t dma_addr;
    int n_window;
    u32 id;
    int low_mark;
    int high_mark;

    bool overflow_tx;
};

#define IWL_NUM_OF_COMPLETION_RINGS    31
#define IWL_NUM_OF_TRANSFER_RINGS    527

/**
 * struct iwl_dram_data
 * @physical: page phy pointer
 * @block: pointer to the allocated block/page
 * @size: size of the block/page
 */
struct iwl_dram_data {
    dma_addr_t physical;
    iwl_dma_ptr *physical_ptr;
    void *block;
    int size;
};

/**
 * struct iwl_fw_mon - fw monitor per allocation id
 * @num_frags: number of fragments
 * @frags: an array of DRAM buffer fragments
 */
struct iwl_fw_mon {
    u32 num_frags;
    struct iwl_dram_data *frags;
};

/**
 * struct iwl_self_init_dram - dram data used by self init process
 * @fw: lmac and umac dram data
 * @fw_cnt: total number of items in array
 * @paging: paging dram data
 * @paging_cnt: total number of items in array
 */
struct iwl_self_init_dram {
    struct iwl_dram_data *fw;
    int fw_cnt;
    struct iwl_dram_data *paging;
    int paging_cnt;
};

/**
 * struct iwl_rb_allocator - Rx allocator
 * @req_pending: number of requests the allcator had not processed yet
 * @req_ready: number of requests honored and ready for claiming
 * @rbd_allocated: RBDs with pages allocated and ready to be handled to
 *    the queue. This is a list of &struct iwl_rx_mem_buffer
 * @rbd_empty: RBDs with no page attached for allocator use. This is a list
 *    of &struct iwl_rx_mem_buffer
 * @lock: protects the rbd_allocated and rbd_empty lists
 * @alloc_wq: work queue for background calls
 * @rx_alloc: work struct for background calls
 */
struct iwl_rb_allocator {
    int req_pending;
    int req_ready;
    TAILQ_HEAD(, iwl_rx_mem_buffer) rbd_allocated;
    TAILQ_HEAD(, iwl_rx_mem_buffer) rbd_empty;
    IOSimpleLock *lock;
};

/*
 * RX related structures and functions
 */
#define RX_NUM_QUEUES 1
#define RX_POST_REQ_ALLOC 2
#define RX_CLAIM_REQ_ALLOC 8
#define RX_PENDING_WATERMARK 16
#define FIRST_RX_QUEUE 512

static inline dma_addr_t
iwl_pcie_get_first_tb_dma(struct iwl_txq *txq, int idx)
{
    return txq->first_tb_dma + sizeof(struct iwl_pcie_first_tb_buf) * idx;
}

struct iwl_trans_txq_scd_cfg {
    u8 fifo;
    u8 sta_id;
    u8 tid;
    bool aggregate;
    int frame_limit;
};

#endif /* TransHdr_h */
