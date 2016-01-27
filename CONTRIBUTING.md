## How to Contribute

Help from externals in whatever form is always more than welcome. You can start
by just trying out the various products of the C++ codebase (our main focus
is now on the language Solidity, the IDE Mix and the Ethereum node implementation
eth). If something strange happens, please report an issue (see below).
Once you get to know the technology, you can try to answer questions from
other people (we do not always have time for that) either on [gitter](https://gitter.im/ethereum/cpp-ethereum),
[stackexchange](http://ethereum.stackexchange.com/) or just comment on [issues](http://github.com/ethereum/webthree-umbrella/issues).

If you know C++, you can help by submitting pull requests (see below).
We try to keep a list of [good tasks to start with](https://github.com/ethereum/webthree-umbrella/labels/good%20first%20task).
Please get in contact on gitter if you have any questions or suggestions.

We keep backlogs of tasks in different sites, depending on the subproject:

 * [Solidity Backlog](https://www.pivotaltracker.com/n/projects/1189488)
 * [Mix Backlog](https://github.com/ethereum/mix/issues)
 * [Eth Backlog](https://www.pivotaltracker.com/n/projects/1510366)


### How to Report Issues

Please report issues to the project they appear in, either
[Solidity](https://github.com/ethereum/solidity/issues),
[Mix](https://github.com/ethereum/mix/issues) or
[eth / anything else](https://github.com/ethereum/webthree-umbrella/issues).

Try to mention which version of the software you used and on which platform (Windows, MacOS, Linux, ...),
how you got into the situation (what did you do), what did you expect to happen
and what actually happened.

### How to Submit Pull Requests / Workflow

Please see the [wiki](https://github.com/ethereum/webthree-umbrella/wiki) for how
to set up your work environment and for build instructions. There, you can
also find an [overview](https://github.com/ethereum/webthree-umbrella/wiki/Overview)
of the various repositories and directories.
Please also repect the [Coding Style](CodingStandards.txt).

If you encounter any problems, please ask on gitter.

If you are satisfied with your work and all tests pass,
fork the sub-repository you want to change and register your fork
as a remote in the sub-repository you recursively cloned from webthree-umbrella.
Create pull requests against the `develop` branch of the repository you
made changes in. Try not to include any merges with the pull request and rebase
if necessary. If you can set labels on a pull request, set it to `please review`
and also ask for a review in [gitter](http://gitter.im/ethereum/cpp-ethereum).
You can also do reviews on others' pull requests. In this case either comment
with "looks good" or set the label if you can. If at least one core developer
apart from the author is confident about the change, it can be merged.
If the reviewer thinks that corrections are necessary, they put he label `got issues`.
If the author addressed all comments, they again put `please review` or comment
appropriately.

Our CI system will run tests against the subproject and notify about their satus on the pull request.

Thanks for helping and have fun!
