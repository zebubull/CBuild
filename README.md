# CBuild
A simple build system for c programs. I would recommend against using this for 
literally anything because it is just something I am making for fun. This is 
not meant to be better than any other build system, I just felt like making 
this. Feel free to check it out if you'd like, but please don't expect it 
to be anything amazing.

## Features (listed in order of priority)
- [x] Build multi-file c projects
- [x] Simple configuration language
- [ ] Incremental compilation
- [ ] Library support
- [ ] Linux support

## Use
The repo comes with `bootstrap.exe`, a precompiled version of cbuild that can 
be used to compile itself. `bootstrap.exe` is likely an old version of cbuild 
so it is recommended to run it to compile the latest version of cbuild. After 
that, simply run `cbuild` in a directory with a cbuild config file to build 
your project.

## Configuration  

### `source` (Required)
Path to the directory containing all source files for the project.  

### `project` (Required)
Name of the executable file to output (no extension).  

### `cache` (Optional; Default: False)
Whether or not to cache the executable file to the `.cbuild` folder. This option 
is really only useful for building `cbuild` using itself on windows, as it must 
edit `cbuild.exe` while it is running.

### Example Config
The [cbuild](cbuild) file in the project root.
