# webthree-umbrella

This repository is an umbrella repository with git sub-module references to all of the components of [cpp-ethereum](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/), the [Ethereum](http://ethereum.org) C++ client.  The project was initiated by [Gavin Wood](http://gavwood.com/>), the [former CTO](<https://blog.ethereum.org/2016/01/11/last-blog-post/>) of the [Ethereum Foundation](http://www.ethdocs.org/en/latest/introduction/foundation.html), in December 2013.   It is the second most popular of the clients with around [5% of 'market share'](http://ethernodes.org/>), trailing a long way behind
[go-ethereum](https://github.com/ethereum/go-ethereum).

![C++](http://www.ethdocs.org/en/latest/_images/cpp_35k9.png) 
![Ethereum](http://www.ethdocs.org/en/latest/_images/ETHEREUM-ICON_Black.png)

Read more about the project at **[our swanky new website](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/)**.

[![Join the chat at https://gitter.im/ethereum/cpp-ethereum](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/ethereum/cpp-ethereum?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

          | Status
----------|-----------
develop   | [![Build Status](http://52.28.164.97/buildStatus/icon?job=ethbinaries-develop)](http://52.28.164.97/job/ethbinaries-develop/)
release   | [![Build Status](http://52.28.164.97/buildStatus/icon?job=ethbinaries-release)](http://52.28.164.97/job/ethbinaries-release/)

The current codebase is the work of many, many hands, with probably close to 100 individual contributors over the course of its development.   Perhaps we will write a script to maintain a 'credits list' at some stage?   In the meantime, here are all of the per-repo contributions:

- [alethzero](https://github.com/ethereum/alethzero/graphs/contributors)
- [cpp-dependencies](https://github.com/ethereum/cpp-dependencies/graphs/contributors)
- [cpp-ethereum](https://github.com/ethereum/cpp-ethereum/graphs/contributors)
- [evmjit](https://github.com/ethereum/evmjit/graphs/contributors)
- [homebrew-ethereum](https://github.com/ethereum/homebrew-ethereum/graphs/contributors)
- [libethereum](https://github.com/ethereum/libethereum/graphs/contributors)
- [libweb3core](https://github.com/ethereum/libweb3core/graphs/contributors)
- [solidity](https://github.com/ethereum/solidity/graphs/contributors)
- [web3.js](https://github.com/ethereum/web3.js/graphs/contributors)
- [webthree](https://github.com/ethereum/webthree/graphs/contributors)
- [webthree-helpers](https://github.com/ethereum/webthree-helpers/graphs/contributors)
- [webthree-umbrella](https://github.com/ethereum/webthree-umbrella/graphs/contributors)

The following individuals are currently employed or contracted by the Ethereum Foundation for C++ client work:

- [Greg Colvin](https://github.com/gcolvin)
- [Liana Husikyan](https://github.com/LianaHus)
- [Dimitry Khoklov](https://github.com/winsvega)
- [Yann Levreau](https://github.com/yann300)
- [Christian Reitwiessner](https://github.com/chriseth) - C++ Lead
- [Bob Summerwill](https://github.com/bobsummerwill)

### Testing

To run the tests, make sure you clone github.com/ethereum/tests and point the environment variable
`ETHEREUM_TEST_PATH` to that path.

### Contributing

External contributions are more than welcome! We try to keep a list of tasks that are suitable for
newcomers under the tag [good first task](https://github.com/ethereum/webthree-umbrella/labels/good%20first%20task).
If you have any questions, please ask in our [gitter channel](https://gitter.im/ethereum/cpp-ethereum).

Please refer to the file [CONTRIBUTING.md](CONTRIBUTING.md) for some guidelines.

All new contributions are added under the MIT License. Please refer to the `LICENSE` file in the root directory.
To state that you accept this fact for all of your contributions please add yourself to the list of external contributors like in the example below.

### License

All new contributions are under the [MIT license](http://opensource.org/licenses/MIT).
See [LICENSE](LICENSE). Some old contributions are under the [GPLv3 license](http://www.gnu.org/licenses/gpl-3.0.en.html). See [GPLV3_LICENSE](GPLV3_LICENSE).

#### External Contributors

I hereby place all my contributions in this codebase under an MIT
licence, as specified [here](http://opensource.org/licenses/MIT).
- *Name Surname* (**email@domain**)

Please add yourself in the `@author` doxygen  section of the file your are adding/editing
with the same wording as the one you listed yourself in the external contributors section above,
only replacing the word **contribution** by **file**

All development goes in develop branch - please don't submit pull requests to master.

Please read [CodingStandards.txt](CodingStandards.txt) thoroughly before making alterations to the code base. Please do *NOT* use an editor that automatically reformats whitespace away from astylerc or the formatting guidelines as described in [CodingStandards.txt](CodingStandards.txt).

libweb3jsonrpc/abstractwebthreestubserver.h is autogenerated from the jsonrpcstub executable that comes with the libjsonrpc library (json-rpc-cpp project). It shouldn't be maually altered.

```bash
jsonrpcstub spec.json --cpp-server=AbstractWebThreeStubServer
```
