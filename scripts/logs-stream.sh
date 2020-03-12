sudo log stream --process=0 | grep 'AppleIntelWifi' | sed -E 's/^.*(AppleIntelWifiAdapter |\(AppleIntelWifiAdapterV2\) )//'
