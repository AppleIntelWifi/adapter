# adapter
## Purpose
This is a kext which aims to provide support for Intel wireless devices on MacOS. It is intended to provide similar support with MacOS
as a native AirPort device (via the IO80211Controller private IOKit class).

This kext does not function yet. 

As of today (03/04/20), the most that it can do is upload firmware to the WiFi card and initialize scanning bands. It will take significantly more work to make it function, and integrate with the IOKit 802.11 controller class. I (@hatf0) am a full time student with very limited resources and can only spend so much time on this.

**Update**: As of tody (03/27/20), the most it can do is scan for wireless networks. More to come.

## Supported devices
Generally any device that is not a gen2 device (AX-series/Wi-Fi 6 not supported) will work.

If your card does not, please open an issue here: https://github.com/AppleIntelWifi/device-logs

## Build instructions
Ensure `xcpretty` has been installed on your machine.

```
./build.sh
sudo chown -R root:wheel ./build/Build/Products/Debug/AppleIntelWifiAdapterV2.kext/
sudo kextload -v ./build/Build/Products/Debug/AppleIntelWifiAdapterV2.kext/
```
