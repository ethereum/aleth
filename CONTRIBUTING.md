# cpp-ethereum Contributing and Review Guidelines

1. All submitted **Pull Requests** are assumed to be ready for review 
   unless they are labeled with `[in progress]` or have "[WIP]" in title.

2. **Code formatting** rules are described by the [Clang-Format Style Options] file [.clang-format].
   Please use the [clang-format] tool to format your code _changes_ accordingly.
   
   Example:
   
       clang-format -i -lines=146:159 libevm/JitVM.cpp


[Clang-Format Style Options]: https://clang.llvm.org/docs/ClangFormatStyleOptions.html
[clang-format]:               https://clang.llvm.org/docs/ClangFormat.html
[.clang-format]:              .clang-format
