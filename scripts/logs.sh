log show --last boot --predicate 'process == "kernel"' | grep 'AppleIntelWifi' | sed -E 's/^.*(AppleIntelWifiAdapter |\(AppleIntelWifiAdapterV2\) )//' > log.txt
