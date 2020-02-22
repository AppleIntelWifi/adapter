# adapter
## Purpose
This is a kext which aims to provide support for Intel wireless devices on MacOS. It is intended to provide similar support with MacOS
as a native AirPort device (via the IO80211Controller private IOKit class).

## Supported devices
Generally any device that is not a gen2 device (AX-series/Wi-Fi 6 not supported) will work.

If your card does not, please open an issue here: https://github.com/AppleIntelWifi/device-logs

## Build instructions
```
sudo chown -R root:wheel AppleIntelWifiAdapterV2.kext/
sudo kextload -v AppleIntelWifiAdapterV2.kext/
```
