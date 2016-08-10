# webthree-umbrella

This repository is an umbrella repository with git sub-module references to all of the components
of [cpp-ethereum](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/), the
[Ethereum](http://ethereum.org) C++ client.
The project was initiated by [Gavin Wood](http://gavwood.com/>), the
[former CTO](<https://blog.ethereum.org/2016/01/11/last-blog-post/>) of the
[Ethereum Foundation](http://www.ethdocs.org/en/latest/introduction/foundation.html),
in December 2013.   It is the second most popular of the clients, trailing a long way behind
the dominant [geth](https://github.com/ethereum/go-ethereum) client, also built by the
Ethereum Foundation.

![C++](http://www.ethdocs.org/en/latest/_images/cpp_35k9.png) 
![Ethereum](http://www.ethdocs.org/en/latest/_images/ETHEREUM-ICON_Black.png)

Read more about the project at the **[cpp-ethereum Homepage](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/)**.

[![cpp-ethereum](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/ethereum/cpp-ethereum?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

See [Contributors](https://github.com/ethereum/webthree-umbrella/wiki/Contributors) for the
full list of everybody who has worked on the code.

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
