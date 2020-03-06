sudo log stream --process=0 | grep 'AppleIntelWifiAdapterV2' | sed -E 's/^.*(AppleIntelWifiAdapter |\(AppleIntelWifiAdapterV2\) )//'
