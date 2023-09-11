# CBuild
A simple build system for c programs. I would recommend against using this for literally anything because it is just something I am making for fun. This is not meant to be better than any other build system, I just felt like making this. Feel free to check it out if you'd like, but please don't expect it to be anything amazing.

## Features (listed in order of priority)
- [x] Build multi-file c projects
- [x] Simple configuration language
- [x] Incremental compilation
- [x] Compilation rules
- [x] Linux support?
- [ ] Library support

## Use
The repo comes with `bootstrap.exe` (`bootstrap.out` on linux), a precompiled version of cbuild that can be used to compile itself. `bootstrap.exe` is stable build of cbuild so it is recommended to run it to compile the latest version of cbuild. After that, simply run `cbuild-debug.exe` or (`cbuild-release` if you built a release version, which you probably should) in a directory with a cbuild config file to build your project. The first argument passed to `cbuild.exe` is the target rule to be followed. If no argument is provided, the default rule will be built.

## Configuration  

### `source` (Required)
Path to the directory containing all source files for the project.  

### `project` (Required)
Name of the executable file to output (no extension).  

### `cache` (Optional; Default: Off; Options: On | \[Off\])
Whether or not to cache the executable file to the `.cbuild` folder. This option is really only useful for building `cbuild` using itself on windows, as it must edit `cbuild.exe` while it is running.

### `rule` (Optional; Options: Any literal with no whitespace)
Any configuration directives between a `rule` and `endrule` pair will be ignored if the rule is not specified. If no rule is specified in the build command, then the first rule declared in the `cbuild` file will be assumed to be the default.

### `endrule` (Optional; Options: None)
Ends a `rule` block.

### `define` (Optional; Options: Any literal with no whitespace)
Sets the given preprocessor macro to be defined.

### `flag` (Optional; Options: Any literal with no whitespace)
Sets the given compiler flag. A `-` or `--` must be included, as this simply passes the given literal as an argument to the compiler with no processing.

### Example Config
The [cbuild](cbuild) file in the project root.
