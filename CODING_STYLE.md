# Aleth Coding Style

## Code Formatting

Use clang-format tool to format your changes, see [CONTRIBUTING](CONTRIBUTING.md) for details.


## Namespaces

1. No `using namespace` declarations in header files.
2. All symbols should be declared in a namespace except for final applications.
3. Preprocessor symbols should be prefixed with the namespace in all-caps and an underscore.

```cpp
       // WRONG:
       #include <cassert>
       using namespace std;
       tuple<float, float> meanAndSigma(vector<float> const& _v);

       // CORRECT:
       #include <cassert>
       std::tuple<float, float> meanAndSigma(std::vector<float> const& _v);
```

## Preprocessor

1. File comment is always at top, and includes:
   - Copyright.
   - License.
   
2. Never use `#ifdef`/`#define`/`#endif` file guards. Prefer `#pragma` once as first line below file comment.
3. Prefer static const variable to value macros.
4. Prefer inline constexpr functions to function macros.


## Capitalization

GOLDEN RULE: Preprocessor: ALL_CAPS; C++: camelCase.

1. Use camelCase for splitting words in names, except where obviously extending STL/boost functionality in which case follow those naming conventions.
2. The following entities' first alpha is upper case:
   - Type names.
   - Template parameters.
   - Enum members.
   - static const variables that form an external API.
3. All preprocessor symbols (macros, macro arguments) in full uppercase with underscore word separation.


All other entities' first alpha is lower case.


## Variable prefixes

1. Leading underscore `_` to parameter names.
   - Exception: `o_parameterName` when it is used exclusively for output.
     See also Declarations.5.
   - Exception: `io_parameterName` when it is used for both input and output.
     See also Declarations.5.
2. Leading `c_` to const variables (unless part of an external API).
3. Leading `g_` to global (non-const) variables.
4. Leading `s_` to static (non-const, non-global) variables.



## Error reporting

Prefer exception to bool/int return type.


## Declarations

1. {Typename} + {qualifiers} + {name}. (**TODO**: Against [NL.26](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl26-use-conventional-const-notation))
2. Only one per line.
3. Favour declarations close to use; don't habitually declare at top of scope ala C.
4. Always pass non-trivial parameters with a const& suffix.
5. To return multiple "out" values, prefer returning a tuple or struct.
   See [F.21].
6. Never use a macro where adequate non-preprocessor C++ can be written.
7. Make use of auto whenever type is clear or unimportant:
   - Always avoid doubly-stating the type.
   - Use to avoid vast and unimportant type declarations.
   - However, avoid using auto where type is not immediately obvious from the context, and especially not for arithmetic expressions.
8. Don't pass bools: prefer enumerations instead.
9. Prefer enum class to straight enum.

```cpp
       // WRONG:
       const double d = 0;
       int i, j;
       char *s;
       float meanAndSigma(std::vector<float> _v, float* _sigma, bool _approximate);
       Derived* x(dynamic_cast<Derived*>(base));
       for (map<ComplexTypeOne, ComplexTypeTwo>::iterator i = l.begin(); i != l.end(); ++l) {}

       // CORRECT:
       enum class Accuracy
       {
           Approximate,
           Exact
       };
       double const d = 0;
       int i;
       int j;
       char* s;
       std::tuple<float, float> meanAndSigma(std::vector<float> const& _v, Accuracy _a);
       auto x = dynamic_cast<Derived*>(base);
       for (auto i = x.begin(); i != x.end(); ++i) {}
```

## Structs & classes

1. Structs to be used when all members public and no virtual functions.
   - In this case, members should be named naturally and not prefixed with `m_`
2. Classes to be used in all other circumstances.



## Members

1. One member per line only.
2. Private, non-static, non-const fields prefixed with m_.
3. Avoid public fields, except in structs.
4. Use `override`, `final` and `const` as much as possible.
5. No implementations with the class declaration, except:
   - template or force-inline method (though prefer implementation at bottom of header file).
   - one-line implementation (in which case include it in same line as declaration).
6. For a property `foo`
   - Member: `m_foo`;
   - Getter: `foo()`; also: for booleans, `isFoo()`
   - Setter: `setFoo()`;


## Naming

1. Collection conventions:
   - `...s` means `std::vector` e.g. `using MyTypes = std::vector<MyType>`
   - `...Set` means `std::set` e.g. `using MyTypeSet = std::set<MyType>`
   - `...Hash` means `std::unordered_set` e.g. `using MyTypeHash = std::unordered_set<MyType>`
2. Class conventions:
   - `...Face` means the interface of some shared concept. (e.g. `FooFace` might be a pure virtual class.)
3. Avoid unpronounceable names:
   - If you need to shorten a name favour a pronouncable slice of the original to a scattered set of consonants.
   - e.g. `Manager` shortens to `Man` rather than `Mgr`.
4. Avoid prefixes of initials (e.g. DON'T use `IMyInterface`, `CMyImplementation`)
5. Find short, memorable & (at least semi-) descriptive names for commonly used classes or name-fragments.
   - A dictionary and thesaurus are your friends.
   - Spell correctly.
   - Think carefully about the class's purpose.
   - Imagine it as an isolated component to try to decontextualise it when considering its name.
   - Don't be trapped into naming it (purely) in terms of its implementation.



## Type-definitions

1. Prefer `using` to `typedef`. E.g. `using ints = std::vector<int>` rather than
   `typedef std::vector<int> ints`.
2. Generally avoid shortening a standard form that already includes all important information:
   - e.g. stick to `shared_ptr<X>` rather than shortening to `ptr<X>`.
3. Where there are exceptions to this (due to excessive use and clear meaning), note the change prominently and use it consistently.
   - e.g. 
   ```cpp
   using Guard = std::lock_guard<std::mutex>; ///< Guard is used throughout the codebase since it's clear in meaning and used commonly.
   ```
4. In general expressions should be roughly as important/semantically meaningful as the space they occupy.



## Commenting

1. Comments should be doxygen-compilable, using @notation rather than \notation.
2. Document the interface, not the implementation.
   - Documentation should be able to remain completely unchanged, even if the method is reimplemented.
   - Comment in terms of the method properties and intended alteration to class state (or what aspects of the state it reports).
   - Be careful to scrutinise documentation that extends only to intended purpose and usage.
   - Reject documentation that is simply an English transaction of the implementation.



## Logging

Logging should be performed at appropriate verbosities depending on the logging message.
The more likely a message is to repeat (and thus cause noise) the higher in verbosity it should be.
Some rules to keep in mind:

 - Verbosity == 0 -> Reserved for important stuff that users must see and can understand.
 - Verbosity == 1 -> Reserved for stuff that users don't need to see but can understand.
 - Verbosity >= 2 -> Anything that is or might be displayed more than once every minute
 - Verbosity >= 3 -> Anything that only a developer would understand
 - Verbosity >= 4 -> Anything that is low-level (e.g. peer disconnects, timers being cancelled)


## Recommended reading

Herb Sutter and Bjarne Stroustrup
- "C++ Core Guidelines" (https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md)

Herb Sutter and Andrei Alexandrescu
- "C++ Coding Standards: 101 Rules, Guidelines, and Best Practices"

Scott Meyers
- "Effective C++: 55 Specific Ways to Improve Your Programs and Designs (3rd Edition)"
- "More Effective C++: 35 New Ways to Improve Your Programs and Designs"
- "Effective Modern C++: 42 Specific Ways to Improve Your Use of C++11 and C++14"


[F.21]: https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f21-to-return-multiple-out-values-prefer-returning-a-tuple-or-struct
