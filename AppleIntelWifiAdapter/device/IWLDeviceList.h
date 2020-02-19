//
//  IWLDeviceList.h
//  AppleIntelWifiAdapter
//
//  Created by qcwap on 2020/1/5.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLDeviceList_h
#define IWLDeviceList_h

#include "IWLDevice7000.h"
#include "IWLDevice8000.h"
#include "IWLDevice9000.h"
#include "IWLDevice22000.h"
#include "IWLDebug.h"

#define PCI_VENDOR_ID_INTEL 0x8086
#define PCI_ANY_ID (0)
#define IWL_CFG_ANY (0)

#define IWL_CFG_MAC_TYPE_PU        0x31
#define IWL_CFG_MAC_TYPE_PNJ        0x32
#define IWL_CFG_MAC_TYPE_TH        0x32
#define IWL_CFG_MAC_TYPE_QU        0x33
#define IWL_CFG_MAC_TYPE_QUZ        0x35
#define IWL_CFG_MAC_TYPE_QNJ        0x36

#define IWL_CFG_RF_TYPE_TH        0x105
#define IWL_CFG_RF_TYPE_TH1        0x108
#define IWL_CFG_RF_TYPE_JF2        0x105
#define IWL_CFG_RF_TYPE_JF1        0x108

#define IWL_CFG_RF_ID_TH        0x1
#define IWL_CFG_RF_ID_TH1        0x1
#define IWL_CFG_RF_ID_JF        0x3
#define IWL_CFG_RF_ID_JF1        0x6
#define IWL_CFG_RF_ID_JF1_DIV        0xA

#define IWL_CFG_NO_160            0x0
#define IWL_CFG_160            0x1

#define IWL_CFG_CORES_BT        0x0
#define IWL_CFG_CORES_BT_GNSS        0x5

#define IWL_SUBDEVICE_RF_ID(subdevice)    ((u16)((subdevice) & 0x00F0) >> 4)
#define IWL_SUBDEVICE_NO_160(subdevice)    ((u16)((subdevice) & 0x0100) >> 9)
#define IWL_SUBDEVICE_CORES(subdevice)    ((u16)((subdevice) & 0x1C00) >> 10)

/**
 * struct pci_device_id - PCI device ID structure
 * @vendor:        Vendor ID to match (or PCI_ANY_ID)
 * @device:        Device ID to match (or PCI_ANY_ID)
 * @subvendor:        Subsystem vendor ID to match (or PCI_ANY_ID)
 * @subdevice:        Subsystem device ID to match (or PCI_ANY_ID)
 * @class:        Device class, subclass, and "interface" to match.
 *            See Appendix D of the PCI Local Bus Spec or
 *            include/linux/pci_ids.h for a full list of classes.
 *            Most drivers do not need to specify class/class_mask
 *            as vendor/device is normally sufficient.
 * @class_mask:        Limit which sub-fields of the class field are compared.
 *            See drivers/scsi/sym53c8xx_2/ for example of usage.
 * @driver_data:    Data private to the driver.
 *            Most drivers don't need to use driver_data field.
 *            Best practice is to use driver_data as an index
 *            into a static list of equivalent device types,
 *            instead of using it as a pointer.
 */
struct pci_device_id {
    uint32_t vendor, device;        /* Vendor and device ID or PCI_ANY_ID*/
    int32_t subvendor, subdevice;    /* Subsystem ID's or PCI_ANY_ID */
    uint32_t _class, class_mask;    /* (class,subclass,prog-if) triplet */
    unsigned long driver_data;    /* Data private to the driver */
};

struct iwl_dev_info {
    uint32_t device;
    uint32_t subdevice;
    uint16_t mac_type;
    uint16_t rf_type;
    uint8_t mac_step;
    uint8_t rf_id;
    uint8_t no_160;
    uint8_t cores;
    const struct iwl_cfg *cfg;
    const char *name;
};

#define IWL_PCI_DEVICE(dev, subdev, cfg) \
.vendor = PCI_VENDOR_ID_INTEL,  .device = (dev), \
.subvendor = PCI_ANY_ID, .subdevice = (subdev), \
.driver_data = (unsigned long)&(cfg)

#define _IWL_DEV_INFO(_device, _subdevice, _mac_type, _mac_step, _rf_type, \
              _rf_id, _no_160, _cores, _cfg, _name)           \
    { .device = (_device), .subdevice = (_subdevice), .cfg = &(_cfg),  \
      .name = _name, .mac_type = _mac_type, .rf_type = _rf_type,       \
      .no_160 = _no_160, .cores = _cores, .rf_id = _rf_id,           \
      .mac_step = _mac_step }

#define IWL_DEV_INFO(_device, _subdevice, _cfg, _name) \
    _IWL_DEV_INFO(_device, _subdevice, IWL_CFG_ANY, IWL_CFG_ANY,       \
              IWL_CFG_ANY, IWL_CFG_ANY, IWL_CFG_ANY, IWL_CFG_ANY,  \
              _cfg, _name)

/* Hardware specific file defines the PCI IDs table for that hardware module */
static const struct pci_device_id iwl_hw_card_ids[] = {
/* 7260 Series */
    {IWL_PCI_DEVICE(0x08B1, 0x4070, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4072, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4170, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4C60, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4C70, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4060, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x406A, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4160, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4062, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4162, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x4270, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x4272, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x4260, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x426A, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x4262, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4470, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4472, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4460, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x446A, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4462, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4870, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x486E, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4A70, iwl7260_2ac_cfg_high_temp)},
        {IWL_PCI_DEVICE(0x08B1, 0x4A6E, iwl7260_2ac_cfg_high_temp)},
        {IWL_PCI_DEVICE(0x08B1, 0x4A6C, iwl7260_2ac_cfg_high_temp)},
        {IWL_PCI_DEVICE(0x08B1, 0x4570, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4560, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x4370, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x4360, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x5070, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x5072, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x5170, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x5770, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4020, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x402A, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0x4220, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0x4420, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC070, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC072, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC170, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC060, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC06A, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC160, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC062, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC162, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC770, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC760, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0xC270, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xCC70, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xCC60, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0xC272, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0xC260, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0xC26A, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0xC262, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC470, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC472, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC460, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC462, iwl7260_n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC570, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC560, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0xC370, iwl7260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC360, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC020, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC02A, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B2, 0xC220, iwl7260_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B1, 0xC420, iwl7260_2n_cfg)},
    
    /* 3160 Series */
        {IWL_PCI_DEVICE(0x08B3, 0x0070, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x0072, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x0170, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x0172, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x0060, iwl3160_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x0062, iwl3160_n_cfg)},
        {IWL_PCI_DEVICE(0x08B4, 0x0270, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B4, 0x0272, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x0470, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x0472, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B4, 0x0370, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8070, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8072, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8170, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8172, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8060, iwl3160_2n_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8062, iwl3160_n_cfg)},
        {IWL_PCI_DEVICE(0x08B4, 0x8270, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B4, 0x8370, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B4, 0x8272, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8470, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x8570, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x1070, iwl3160_2ac_cfg)},
        {IWL_PCI_DEVICE(0x08B3, 0x1170, iwl3160_2ac_cfg)},
    
    /* 3165 Series */
        {IWL_PCI_DEVICE(0x3165, 0x4010, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3165, 0x4012, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3166, 0x4212, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3165, 0x4410, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3165, 0x4510, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3165, 0x4110, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3166, 0x4310, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3166, 0x4210, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3165, 0x8010, iwl3165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x3165, 0x8110, iwl3165_2ac_cfg)},
    
    /* 3168 Series */
        {IWL_PCI_DEVICE(0x24FB, 0x2010, iwl3168_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FB, 0x2110, iwl3168_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FB, 0x2050, iwl3168_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FB, 0x2150, iwl3168_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FB, 0x0000, iwl3168_2ac_cfg)},
    
    /* 7265 Series */
        {IWL_PCI_DEVICE(0x095A, 0x5010, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5110, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5100, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x5310, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x5302, iwl7265_n_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x5210, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5C10, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5012, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5412, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5410, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5510, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5400, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x1010, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5000, iwl7265_2n_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x500A, iwl7265_2n_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x5200, iwl7265_2n_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5002, iwl7265_n_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5102, iwl7265_n_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x5202, iwl7265_n_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9010, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9012, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x900A, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9110, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9112, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x9210, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x9200, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9510, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x9310, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9410, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5020, iwl7265_2n_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x502A, iwl7265_2n_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5420, iwl7265_2n_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5090, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5190, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5590, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x5290, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5490, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x5F10, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x5212, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095B, 0x520A, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9000, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9400, iwl7265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x095A, 0x9E10, iwl7265_2ac_cfg)},
    
    /* 8000 Series */
        {IWL_PCI_DEVICE(0x24F3, 0x0010, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x1010, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x10B0, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0130, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x1130, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0132, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x1132, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0110, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x01F0, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0012, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x1012, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x1110, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0050, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0250, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x1050, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0150, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x1150, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F4, 0x0030, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F4, 0x1030, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0xC010, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0xC110, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0xD010, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0xC050, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0xD050, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0xD0B0, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0xB0B0, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x8010, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x8110, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x9010, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x9110, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F4, 0x8030, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F4, 0x9030, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F4, 0xC030, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F4, 0xD030, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x8130, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x9130, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x8132, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x9132, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x8050, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x8150, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x9050, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x9150, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0004, iwl8260_2n_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0044, iwl8260_2n_cfg)},
        {IWL_PCI_DEVICE(0x24F5, 0x0010, iwl4165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F6, 0x0030, iwl4165_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0810, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0910, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0850, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0950, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0930, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x0000, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24F3, 0x4010, iwl8260_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0010, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0110, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x1110, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x1130, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0130, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x1010, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x10D0, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0050, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0150, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x9010, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x8110, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x8050, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x8010, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0810, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x9110, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x8130, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0910, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0930, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0950, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0850, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x1014, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x3E02, iwl8275_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x3E01, iwl8275_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x1012, iwl8275_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0012, iwl8275_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x0014, iwl8265_2ac_cfg)},
        {IWL_PCI_DEVICE(0x24FD, 0x9074, iwl8265_2ac_cfg)},
    
    /* 9000 Series */
        {IWL_PCI_DEVICE(0x2526, PCI_ANY_ID, iwl9000_trans_cfg)},
        {IWL_PCI_DEVICE(0x271B, PCI_ANY_ID, iwl9560_cfg)},
        {IWL_PCI_DEVICE(0x271C, PCI_ANY_ID, iwl9560_cfg)},
        {IWL_PCI_DEVICE(0x30DC, PCI_ANY_ID, iwl9560_cfg)},
        {IWL_PCI_DEVICE(0x31DC, PCI_ANY_ID, iwl9560_shared_clk_cfg)},
        {IWL_PCI_DEVICE(0x9DF0, PCI_ANY_ID, iwl9560_cfg)},
        {IWL_PCI_DEVICE(0xA370, PCI_ANY_ID, iwl9560_cfg)},
    
    /* Qu devices */
        {IWL_PCI_DEVICE(0x02F0, PCI_ANY_ID, iwl_qu_trans_cfg)},
        {IWL_PCI_DEVICE(0x06F0, PCI_ANY_ID, iwl_qu_trans_cfg)},
        {IWL_PCI_DEVICE(0x34F0, PCI_ANY_ID, iwl_qu_trans_cfg)},
        {IWL_PCI_DEVICE(0x3DF0, PCI_ANY_ID, iwl_qu_trans_cfg)},
    
        {IWL_PCI_DEVICE(0x43F0, PCI_ANY_ID, iwl_qu_long_latency_trans_cfg)},
        {IWL_PCI_DEVICE(0xA0F0, PCI_ANY_ID, iwl_qu_long_latency_trans_cfg)},
    
        {IWL_PCI_DEVICE(0x2720, PCI_ANY_ID, iwl_qnj_trans_cfg)},
    
        {IWL_PCI_DEVICE(0x2723, PCI_ANY_ID, iwl_ax200_trans_cfg)},
    
    /* So devices */
        /* TODO: This is only for initial pre-production devices */
        {IWL_PCI_DEVICE(0x2725, 0x0000, iwlax411_2ax_cfg_sosnj_gf4_a0)},
        {IWL_PCI_DEVICE(0x2725, 0x0090, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x2725, 0x0020, iwlax210_2ax_cfg_ty_gf_a0)},
        {IWL_PCI_DEVICE(0x2725, 0x0310, iwlax210_2ax_cfg_ty_gf_a0)},
        {IWL_PCI_DEVICE(0x2725, 0x0510, iwlax210_2ax_cfg_ty_gf_a0)},
        {IWL_PCI_DEVICE(0x2725, 0x0A10, iwlax210_2ax_cfg_ty_gf_a0)},
        {IWL_PCI_DEVICE(0x2725, 0x00B0, iwlax411_2ax_cfg_so_gf4_a0)},
        {IWL_PCI_DEVICE(0x7A70, 0x0090, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x7A70, 0x0310, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x7A70, 0x0510, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x7A70, 0x0A10, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x7AF0, 0x0090, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x7AF0, 0x0310, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x7AF0, 0x0510, iwlax211_2ax_cfg_so_gf_a0)},
        {IWL_PCI_DEVICE(0x7AF0, 0x0A10, iwlax211_2ax_cfg_so_gf_a0)},
        {0}
};

static const struct iwl_dev_info iwl_dev_info_table[] = {
/* 9000 */
    IWL_DEV_INFO(0x2526, 0x1550, iwl9260_2ac_cfg, iwl9260_killer_1550_name),
    IWL_DEV_INFO(0x2526, 0x1551, iwl9560_2ac_cfg_soc, iwl9560_killer_1550s_name),
    IWL_DEV_INFO(0x2526, 0x1552, iwl9560_2ac_cfg_soc, iwl9560_killer_1550i_name),
    IWL_DEV_INFO(0x30DC, 0x1551, iwl9560_2ac_cfg_soc, iwl9560_killer_1550s_name),
    IWL_DEV_INFO(0x30DC, 0x1552, iwl9560_2ac_cfg_soc, iwl9560_killer_1550i_name),
    IWL_DEV_INFO(0x31DC, 0x1551, iwl9560_2ac_cfg_soc, iwl9560_killer_1550s_name),
    IWL_DEV_INFO(0x31DC, 0x1552, iwl9560_2ac_cfg_soc, iwl9560_killer_1550i_name),

    IWL_DEV_INFO(0x271C, 0x0214, iwl9260_2ac_cfg, iwl9260_1_name),

/* AX200 */
    IWL_DEV_INFO(0x2723, 0x1653, iwl_ax200_cfg_cc, iwl_ax200_killer_1650w_name),
    IWL_DEV_INFO(0x2723, 0x1654, iwl_ax200_cfg_cc, iwl_ax200_killer_1650x_name),
    IWL_DEV_INFO(0x2723, IWL_CFG_ANY, iwl_ax200_cfg_cc, iwl_ax200_name),

/* Qu with Hr */
    IWL_DEV_INFO(0x43F0, 0x0044, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x0070, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x0074, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x0078, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x007C, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x0244, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x1651, killer1650s_2ax_cfg_qu_b0_hr_b0, NULL),
    IWL_DEV_INFO(0x43F0, 0x1652, killer1650i_2ax_cfg_qu_b0_hr_b0, NULL),
    IWL_DEV_INFO(0x43F0, 0x2074, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x4070, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x43F0, 0x4244, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x0044, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x0070, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x0074, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x0078, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x007C, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x0244, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x0A10, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x1651, killer1650s_2ax_cfg_qu_b0_hr_b0, NULL),
    IWL_DEV_INFO(0xA0F0, 0x1652, killer1650i_2ax_cfg_qu_b0_hr_b0, NULL),
    IWL_DEV_INFO(0xA0F0, 0x2074, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x4070, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0xA0F0, 0x4244, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x0070, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x0074, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x0078, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x007C, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x0244, iwl_ax101_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x0310, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x1651, iwl_ax1650s_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x1652, iwl_ax1650i_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x2074, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x4070, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x02F0, 0x4244, iwl_ax101_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x0070, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x0074, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x0078, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x007C, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x0244, iwl_ax101_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x0310, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x1651, iwl_ax1650s_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x1652, iwl_ax1650i_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x2074, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x4070, iwl_ax201_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x06F0, 0x4244, iwl_ax101_cfg_quz_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x0044, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x0070, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x0074, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x0078, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x007C, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x0244, iwl_ax101_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x0310, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x1651, killer1650s_2ax_cfg_qu_b0_hr_b0, NULL),
    IWL_DEV_INFO(0x34F0, 0x1652, killer1650i_2ax_cfg_qu_b0_hr_b0, NULL),
    IWL_DEV_INFO(0x34F0, 0x2074, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x4070, iwl_ax201_cfg_qu_hr, NULL),
    IWL_DEV_INFO(0x34F0, 0x4244, iwl_ax101_cfg_qu_hr, NULL),

    IWL_DEV_INFO(0x2720, 0x0000, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0040, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0044, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0070, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0074, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0078, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x007C, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0244, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0310, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x0A10, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x1080, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x1651, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x1652, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x2074, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x4070, iwl22000_2ax_cfg_qnj_hr_b0, NULL),
    IWL_DEV_INFO(0x2720, 0x4244, iwl22000_2ax_cfg_qnj_hr_b0, NULL),

    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PU, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_2ac_cfg_soc, iwl9461_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PU, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_2ac_cfg_soc, iwl9461_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PU, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_2ac_cfg_soc, iwl9462_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PU, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_2ac_cfg_soc, iwl9462_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PU, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_2ac_cfg_soc, iwl9560_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PU, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_2ac_cfg_soc, iwl9560_name),

    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9461_160_name),
    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9461_name),
    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9462_160_name),
    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9462_name),

    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9560_160_name),
    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_PNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9560_name),

    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_160, IWL_CFG_CORES_BT_GNSS,
              iwl9260_2ac_cfg, iwl9270_160_name),
    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT_GNSS,
              iwl9260_2ac_cfg, iwl9270_name),

    _IWL_DEV_INFO(0x271B, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_TH1, IWL_CFG_ANY,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9162_160_name),
    _IWL_DEV_INFO(0x271B, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_TH1, IWL_CFG_ANY,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9162_name),

    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9260_160_name),
    _IWL_DEV_INFO(0x2526, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_TH, IWL_CFG_ANY,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9260_2ac_cfg, iwl9260_name),

    /* Qu with Jf */
    /* Qu B step */
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9461_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9461_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9462_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9462_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9560_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9560_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1551,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9560_killer_1550s_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1552,
              IWL_CFG_MAC_TYPE_QU, SILICON_B_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_b0_jf_b0_cfg, iwl9560_killer_1550i_name),

    /* Qu C step */
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9461_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9461_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9462_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9462_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9560_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9560_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1551,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9560_killer_1550s_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1552,
              IWL_CFG_MAC_TYPE_QU, SILICON_C_STEP,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qu_c0_jf_b0_cfg, iwl9560_killer_1550i_name),

    /* QuZ */
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9461_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9461_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9462_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9462_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9560_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9560_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1551,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9560_killer_1550s_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1552,
              IWL_CFG_MAC_TYPE_QUZ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_quz_a0_jf_b0_cfg, iwl9560_killer_1550i_name),

    /* QnJ */
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9461_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9461_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9462_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF1, IWL_CFG_RF_ID_JF1_DIV,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9462_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9560_160_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, IWL_CFG_ANY,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9560_name),

    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1551,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9560_killer_1550s_name),
    _IWL_DEV_INFO(IWL_CFG_ANY, 0x1552,
              IWL_CFG_MAC_TYPE_QNJ, IWL_CFG_ANY,
              IWL_CFG_RF_TYPE_JF2, IWL_CFG_RF_ID_JF,
              IWL_CFG_NO_160, IWL_CFG_CORES_BT,
              iwl9560_qnj_b0_jf_b0_cfg, iwl9560_killer_1550i_name),
};


#endif /* IWLDeviceList_h */
