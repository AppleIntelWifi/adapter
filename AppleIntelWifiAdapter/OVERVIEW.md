#  Overview
This document serves as a general overview of every folder.

## firmware
This folder is where all of the device firmware blobs are stored. These are licensed (and copyrighted) by Intel, and no copyright should
be claimed on them.

## compat
This folder contains headers to ease the transition from a Linux-based driver to a BSD driver. It also contains ieee80211, which
was ripped out of OpenBSD.

## device
This folder contains device-specific parameters, and are generally based on parameters from iwlwifi/iwlmvm.

## apple80211
This folder contains headers that allow us to interface with Apple's native 802.11 stack. Rather then implementing it ourselves,
with ieee80211, the idea is that we will use Apple80211 instead.

## mvm
This folder contains the necessary code to interface and set up a MVM device. 
**NOTE: As such, this driver does NOT support DVM devices (i.e if your device does not support 802.11 AC)**

## nvm
This folder contains the necessary code/headers to interface and read from the device's fuses
(for hardware addresses, calibration, and regulatory information). 

**NOTE: Modifying this folder in order to attempt to spoof regulatory information will not work.
The regulatory information has been burnt into the device at mfg time, and cannot be changed through
our driver.**

## trans
This folder contains all of the code and headers to establish a transport layer with the device over PCIe.

