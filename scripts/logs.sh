log show --last boot | grep 'AppleIntelWifiAdapterV2' | sed -E 's/^.*(AppleIntelWifiAdapter |\(AppleIntelWifiAdapterV2\) )//' > log.txt
