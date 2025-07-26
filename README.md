# dedalo.cpp

A strongly opinionated, Clang-first build system for C++.
Build your C++ projects with just more C++.

---

Inspired by [Swift](https://www.swift.org/documentation/server/guides/building.html) and [Zig](https://ziglang.org/learn/build-system/).
All you need to get a C++ project up and running is a C++ compiler, no other languages, config formats, or 3rd party libraries needed.


## You're in control
A Makefile is provided for convenience (ironic, I know), but it's trivial to compile it yourself.
The whole project is a single cpp file, so it's easy to modify, and distribute it alongside your project's source.
The "canon" name of the executable is `ddl`, but you can change that too!


## Commands
- init
- build [TargetName]
- run [TargetName]
- clean


## Getting started
Create a new project like this:
```bash
$ mdkir my_project
$ cd my_project
$ ddl init
$ ddl build
```

This will generate the following tree:
```bash
.
├── build
│   ├── bin
│   │   ├── my_project
│   │   └── obj
│   │       └── main.cpp.o
│   ├── build_script.so
│   ├── dep
│   │   └── main.cpp.d
│   └── json
│       └── main.cpp.json
├── build.cpp
├── compile_commands.json
├── lib
└── src
    └── main.cpp
```
Running `build/bin/my_project` will print the usual "Hello, World!".

By default `ddl build` and `ddl run` will compile in Debug mode.
A `Release` and `Debug` targets are provided by default with the following configuration:
```c++
{
    .name               = "Debug",
    .optimization_level = 0,
    .sanitizers         = ASan | UBSan,
    .defines            = { "DEBUG" },
    .compiler_args      = {
        "g",
        "Wall",
        "Werror",
        "pedantic" }
},
{
    .name               = "Release",
    .optimization_level = 3,
    .sanitizers         = UBSan,
    .defines            = { "RELEASE" },
    .compiler_args      = {
        "Wall",
        "Werror",
        "pedantic" }
}
```

All source files added to `src/` will be compiled, but you can exclude paths adding them to the target's `ignored_paths`.
Only `src/` and `lib/` are added to the list of include paths. You'll have to add the whole path for any nested files you want to include.

You can add library' binaries (static or dynamic) and their headers to `lib/lib_name`, and then add them to the project with:
```c++
// build.cpp
project->add_dependency({ .name = "lib_name" });
```
System (aka installed) libraries don't need an directory in `lib`.


## Planned features
- [x] Single project config file, written in C++.
- [x] Auto-generated file structure.
- [x] Generation of `compile_commands.json`
- [ ] Incremental builds
- [ ] Multithreaded builds
- [ ] `test` command
- [ ] Local dependency management
- [ ] Remote depenency management and/or integration with Conan.
- [ ] Support for MSVC
- [ ] Optional support for [mold](https://github.com/rui314/mold)
- [ ] Optional support for [RAD linker](https://github.com/EpicGamesExt/raddebugger)
- [ ] Optional support for Make, Ninja, CMake and other build systems (for dependencies or as an alternative backend).

