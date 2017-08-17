============
Dependencies
============

Hunter package manager
======================

Hunter is *CMake-driven cross-platform package manager for C++ projects*.
See documentation at https://docs.hunter.sh.

Security aspects
----------------

A project that uses Hunter first downloads the hunter package itself. This is
done with a help of HunterGate CMake module. A copy of the HunterGate module
is included in the main project. The HunterGate is ordered to download specified
release of Hunter and to check whenever the downloaded sources match the
checksum provided to the HunterGate invocation.

A Hunter release contains a full list of provided packages and their versions,
also with checksums. Every downloaded source code of a package is verified with
the checksum.

The HunterGate and selected release of Hunter can be audited to check that
the expected source code of dependencies is downloaded. The audit of any new
Hunter release has to be repeated in case the main project is to upgrade the
Hunter version.

Hunter has an option to download cached binaries from specified cache server
instead of building packages from source. This option is enabled by default.
Because checksums of binaries are not know up front the main project can be at
risk in case the cache server is compromised. In cpp-ethereum we decided to
use only the cache server controlled by Ethereum C++ team located in
https://github.com/ethereum/hunter-cache.

