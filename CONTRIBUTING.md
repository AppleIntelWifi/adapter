# How do I contribute?
First of all, I'd like to thank you for even wanting to contribute on this project. 
I (@hatf0) have spent several restless nights working on this project, and appreciate any help.

Second of all, you can ALWAYS find me on the [/r/Hackintosh discord](https://discord.gg/u8V7N5C) in the #coding channel.
I'd be glad on how to guide you on how to contribute, and where we really need improvement.

## Resources
### Intel WiFi specific
[iwm openbsd source](https://github.com/openbsd/src/blob/master/sys/dev/pci/if_iwm.c)

[iwm netbsd source](https://github.com/NetBSD/src/blob/master/sys/dev/pci/if_iwm.c)

* Please note, the OpenBSD source is more feature-rich (containing initial 802.11n support). 
Both sources are provided here (as the NetBSD source is more compact, and helps with knowing how the device is initialized)

[zxystd/itlwm](https://github.com/zxystd/itlwm)

* A wonderful working implementation of the iwm OpenBSD source 
(does not support 802.11ac/n, and is locked to 802.11g however)

[backport-iwlwifi](https://git.kernel.org/pub/scm/linux/kernel/git/iwlwifi/backport-iwlwifi.git/)

* Official git repository containing the iwlwifi driver. This driver is INCREDIBLY difficult to understand at first glance,
and you should only refer to it when you are either attempting to debug a feature, or add in new functionality beyond what
is offered in iwm (802.11ac, WPA2 offload, ping me if you ever are adding something)

[rpeshkov/IntelWifi](https://github.com/rpeshkov/IntelWifi)

* A similar driver which targeted DVM devices. This serves as a good resource for everything that isn't related to the MVM
operation mode devices, such as transportation initialization, etc. As well, shows how some parts are integrated into Apple80211.

### 802.11 specification

[802.11-2016 IEEE](https://standards.ieee.org/standard/802_11-2016.html)

* This specification is the bible for implementing 802.11AC successfully.
I would HIGHLY recommend referring to this document any time you wish to contribute towards the MAC layer, and run into undocumented code.

### Apple80211 specific

[eapolclient](https://github.com/gfleury/eap8021x-debug/blob/master/eapolclient.tproj/wireless.c)

* This shows typical scan/association/authentication flow, and how the WPA2 negotiation is handled in userspace.

[comex gist](https://gist.github.com/comex/0c19c1b3fa569f549947) 

* This gist contains SEVERAL reverse-engineered gists, with some association flow that REALLY helps.

[Black80211-Catalina](https://github.com/AppleIntelWifi/Black80211-Catalina)

* This KEXT is what really got me into this. Tinkering with this, you can fake the stack into believing ANYTHING,
and it is an incredible research platform.

## Testing your changes
Ensure you are on a machine with a supported device, then `cd` to the project's root directory, and run:
`./scripts/build.sh && ./scripts/load.sh`.

If you are developing on a different machine, and loading on a VM, 
please contact me because I can fully detail my test setup and give you the scripts which have not been committed to the repo.

## Submitting your changes
Please make sure you run the linter and formatter on your code PRIOR to creating a pull request. 
These can be found in the scripts repository (and require `cpplint` and `clang-format` to be installed)

As well, please make sure to [squash](https://github.com/todotxt/todo.txt-android/wiki/Squash-All-Commits-Related-to-a-Single-Issue-into-a-Single-Commit) your commits.
This is merely done to save how many commits we have to wade through, and compacts all of your changes down to one single commit.

After such, create a [pull request](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/about-pull-requests?algolia-query=pull%20request)
on this repo with your changes clearly outlined.

## Our coding style
With exception to the IEEE80211 and Apple80211 files, the entire code-base is compliant to [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html).
We focus on readability, as it makes it easier for others to pick up code, and continue where we have left off.

As well, in conformance with the Google C++ style guide, tabs are 2 spaces.

---

Thank you, and we hope you contribute!
