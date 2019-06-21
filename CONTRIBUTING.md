# Aleth Contributing and Review Guidelines

1. All submitted **Pull Requests** are assumed to be ready for review 
   unless they are labeled with `[in progress]` or have "[WIP]" in title.

2. All submitted **Pull Requests** which change Aleth in a way that is visible to users (e.g. new features, bug fixes, significant logging changes) should include an entry in [`CHANGELOG.md`](CHANGELOG.md) in the unreleased version section. Please categorize your entry appropriately and group it with other entries of the same category. Please see the following for details on categories and how to make a good changelog entry: https://keepachangelog.com/en/1.0.0/

2. **Code formatting** rules are described by the [Clang-Format Style Options] file [.clang-format].
   Please use the [clang-format] (version 5.0 or higher recommended) tool to format your code _changes_ accordingly.
   [git-clang-format] tool is useful to limit reformatting to your changes only:

       git clang-format          # to reformat the changes in the staging area and put the result into working directory
       git clang-format -f       # to reformat the changes in the working directory
       git clang-format <commit> # to reformat commits between specified commit and HEAD


[Clang-Format Style Options]: https://clang.llvm.org/docs/ClangFormatStyleOptions.html
[clang-format]:               https://clang.llvm.org/docs/ClangFormat.html
[.clang-format]:              .clang-format
[git-clang-format]:           https://llvm.org/svn/llvm-project/cfe/trunk/tools/clang-format/git-clang-format

