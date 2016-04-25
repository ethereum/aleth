# Former and future home of Ethereum C++ client

Significant effort was put into new
[Ethereum C++ Documentation](http://ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/)
for the [Homestead release](https://blog.ethereum.org/2016/02/29/homestead-release/)
in March 2016.

The C++ codebase was developed in this repository from its inception in December 2013
until October 2015, at which stage it was split into multiple repositories, all 
gathered as git sub-modules under the
[webthree-umbrella](https://github.com/ethereum/webthree-umbrella/) repository.

Those changes were part of an attempt at rebranding the C++ code-base to align with
[Gavin Wood's Web3 vision](https://www.youtube.com/watch?v=TGD7-rfdXDU), but it left
us with a rather bewildering number of names for things.
We [simplified all our naming](https://github.com/ethereum/webthree-umbrella/issues/250)
just prior to Homestead.

We are about to [re-consolidate the repositories](https://github.com/ethereum/webthree-umbrella/issues/251),
which should make the structure of the codebase much easier to understand, and allow us to
make independent releases of cpp-ethereum, solidity and mix.

A dry run for the reorganization is underway at
[bobsummerwill/cpp-ethereum](https://github.com/bobsummerwill/cpp-ethereum/tree/merge_repos)

Current dependencies:

![webthree](http://doublethinkco.github.io/cpp-ethereum-cross/images/dependency_graph.svg)

Target dependencies:

![cpp-ethereum](http://doublethinkco.github.io/cpp-ethereum-cross/images/target_dependency_graph.svg)
