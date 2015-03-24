## Ethereum C++ Client.

[![Join the chat at https://gitter.im/ethereum/cpp-ethereum](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/ethereum/cpp-ethereum?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

By Gav Wood et al, 2013, 2014, 2015.

          | Linux   | OSX | Windows
----------|---------|-----|--------
develop   | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=Linux%20C%2B%2B%20develop%20branch)](https://build.ethdev.com/builders/Linux%20C%2B%2B%20develop%20branch/builds/-1) | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=OSX%20C%2B%2B%20develop%20branch)](https://build.ethdev.com/builders/OSX%20C%2B%2B%20develop%20branch/builds/-1) | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=Windows%20C%2B%2B%20develop%20branch)](https://build.ethdev.com/builders/Windows%20C%2B%2B%20develop%20branch/builds/-1)
master    | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=Linux%20C%2B%2B%20master%20branch)](https://build.ethdev.com/builders/Linux%20C%2B%2B%20master%20branch/builds/-1) | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=OSX%20C%2B%2B%20master%20branch)](https://build.ethdev.com/builders/OSX%20C%2B%2B%20master%20branch/builds/-1) | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=Windows%20C%2B%2B%20master%20branch)](https://build.ethdev.com/builders/Windows%20C%2B%2B%20master%20branch/builds/-1)
evmjit    | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=Linux%20C%2B%2B%20develop%20evmjit)](https://build.ethdev.com/builders/Linux%20C%2B%2B%20develop%20evmjit/builds/-1) | [![Build+Status](https://build.ethdev.com/buildstatusimage?builder=OSX%20C%2B%2B%20develop%20evmjit)](https://build.ethdev.com/builders/OSX%20C%2B%2B%20develop%20evmjit/builds/-1) | N/A

[![Stories in Ready](https://badge.waffle.io/ethereum/cpp-ethereum.png?label=ready&title=Ready)](http://waffle.io/ethereum/cpp-ethereum)

Ethereum is based on a design in an original whitepaper by Vitalik Buterin. This implementation is based on the formal specification of a refinement of that idea detailed in the 'yellow paper' by Gavin Wood. Contributors, builders and testers include Alex Leverington (Clang & Mac building, client multiplexing), Tim Hughes (MSVC compilation & Dagger testing), Caktux (ongoing CI), Christoph Jentzsch (tests), Christian Reissweiner (Solidity), Marek Kotewicz (external JS & JSON-RPC), Eric Lombrozo (MinGW32 cross-compilation), Marko Simovic (original CI), and several others.

### Building

See the [Wiki](https://github.com/ethereum/cpp-ethereum/wiki) for build instructions, compatibility information and build tips. 

### Testing

To run the tests, make sure you clone the tests repository from github.com/ethereum to tests as a sibling to cpp-ethereum.

### Yet To Do

See [TODO](https://github.com/ethereum/cpp-ethereum/wiki/TODO)

### License

See [LICENSE](LICENSE)

### Contributing

All development goes in develop branch - please don't submit pull requests to master.

Please read [CodingStandards.txt](CodingStandards.txt) thoroughly before making alterations to the code base. Please do *NOT* use an editor that automatically reformats whitespace away from astylerc or the formatting guidelines as described in [CodingStandards.txt](CodingStandards.txt).

libweb3jsonrpc/abstractwebthreestubserver.h is autogenerated from the jsonrpcstub executable that comes with the libjsonrpc library (json-rpc-cpp project). It shouldn't be maually altered.

Contributor License Agreement

By contributing your code to cpp-ethereum you grant  Gavin Wood a non-exclusive, irrevocable, worldwide, royalty-free, sublicenseable, transferable license under all of Your relevant intellectual property rights (including copyright, patent, and any other rights), to use, copy, prepare derivative works of, distribute and publicly perform and display the Contributions on any licensing terms, including without limitation: (a) open source licenses like the MIT license; and (b) binary, proprietary, or commercial licenses. Except for the licenses granted herein, You reserve all right, title, and interest in and to the Contribution.

You confirm that you are able to grant us these rights. You represent that You are legally entitled to grant the above license. If Your employer has rights to intellectual property that You create, You represent that You have received permission to make the Contributions on behalf of that employer, or that Your employer has waived such rights for the Contributions.

You represent that the Contributions are Your original works of authorship, and to Your knowledge, no other person claims, or has the right to claim, any right in any invention or patent related to the Contributions. You also represent that You are not legally obligated, whether by entering into an agreement or otherwise, in any way that conflicts with the terms of this license.

Gavin Wood acknowledges that, except as explicitly described in this Agreement, any Contribution which you provide is on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.

```bash
jsonrpcstub spec.json --cpp-server=AbstractWebThreeStubServer
```
