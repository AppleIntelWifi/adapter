log show --last 5m | grep 'AppleIntelWifi' | sed -E 's/^.*(AppleIntelWifiAdapter |\(AppleIntelWifiAdapterV2\) )//' > log.txt
