============
Dependencies
============

Hunter package manager
======================

Hunter is *CMake-driven cross-platform package manager for C++ projects*.
See documentation at https://docs.hunter.sh.

Security aspects
----------------

A project which uses Hunter first downloads the Hunter client. This is
done with the help of the HunterGate CMake module. A copy of the HunterGate module
is included in the main project. The module is instructed to download a specified
Hunter release and verify the downloaded source matches the checksum provided to the HunterGate invocation.

A Hunter release contains a full list of (a specific version) of packages and checksums. Each downloaded package source checksum is compared against the corresponding checksum in the Hunter release.

The HunterGate and selected Hunter release can be audited to verify that
the expected dependency source code is downloaded. This audit must be repeated for each release in case the main project upgrades Hunter.

Hunter has an option to download package binaries from a specified cache server instead of building packages from source. This option is enabled by default. Because checksums of binaries are not known up front, a compromised cache server is an attack vector. In Aleth we decided to
use the cache server controlled by our team located here: https://github.com/ethereum/hunter-cache.

