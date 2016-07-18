# cpp-ethereum

This repository contains [cpp-ethereum](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/), the [Ethereum](http://ethereum.org) C++ client.

It is the third most popular of the Ethereum clients, behind [geth](https://github.com/ethereum/go-ethereum) (the [go](https://golang.org)
client) and [Parity](https://github.com/ethcore/parity) (the [rust](https://www.rust-lang.org/) client).

We are in the [process of re-licensing](https://bobsummerwill.com/2016/07/12/c-re-licensing-plan/) the codebase from the copyleft
[GPLv3](https://en.wikipedia.org/wiki/GNU_General_Public_License) license to the permissive [Apache 2.0](https://en.wikipedia.org/wiki/Apache_License)
licence, to enable Ethereum to be used as broadly as possible.

There is a long-form article - ["Ethereum Everywhere"](https://bobsummerwill.com/2016/07/12/ethereum-everywhere/) - which talks about
the rationale for the change and the history leading up to this proposed change of licensing.

![C++](http://www.ethdocs.org/en/latest/_images/cpp_35k9.png) 
![Ethereum](http://www.ethdocs.org/en/latest/_images/ETHEREUM-ICON_Black.png)

Read more about the cpp-ethereum project at **[https://ethdocs.org](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/)**.

[![Join the chat at https://gitter.im/ethereum/cpp-ethereum](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/ethereum/cpp-ethereum?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

                 | Status
-----------------|-----------
Ubuntu and macOS | [![Build Status](https://travis-ci.org/bobsummerwill/cpp-ethereum.svg?branch=merge_repos)](https://travis-ci.org/bobsummerwill/cpp-ethereum)
Windows          | [![Build Status](https://ci.appveyor.com/api/projects/status/ke9kvs3u1g18vf5t/branch/merge_repos)](https://ci.appveyor.com/project/bobsummerwill/cpp-ethereum)

The current codebase is the work of many, many hands, with probably close to 100 individual contributors over the course of its development:

- [cpp-ethereum](https://github.com/bobsummerwill/cpp-ethereum/graphs/contributors)

The following individuals are currently employed or contracted by the Ethereum Foundation for C++ client work:

- [Greg Colvin](https://github.com/gcolvin)
- [Liana Husikyan](https://github.com/LianaHus)
- [Dimitry Khoklov](https://github.com/winsvega)
- [Yann Levreau](https://github.com/yann300)
- [Christian Reitwiessner](https://github.com/chriseth) - C++ Lead
- [Bob Summerwill](https://github.com/bobsummerwill)

### Testing

To run the tests, make sure you clone https://github.com/ethereum/tests and point the environment variable
`ETHEREUM_TEST_PATH` to that path.

### Contributing

External contributions are more than welcome! We try to keep a list of tasks that are suitable for
newcomers under the tag [good first task](https://github.com/ethereum/webthree-umbrella/labels/good%20first%20task).
If you have any questions, please ask in our [gitter channel](https://gitter.im/ethereum/cpp-ethereum).

Please refer to the file [CONTRIBUTING.md](CONTRIBUTING.md) for some guidelines.

Please read [CodingStandards.txt](CodingStandards.txt) thoroughly before making alterations to the code base.
Please do *NOT* use an editor that automatically reformats whitespace away from astylerc or the formatting guidelines
as described in [CodingStandards.txt](CodingStandards.txt).

All development goes in develop branch - please don't submit pull requests to master.

### License

All contributions are made under the [GPLv3 license](http://www.gnu.org/licenses/gpl-3.0.en.html). See [LICENSE](LICENSE).
