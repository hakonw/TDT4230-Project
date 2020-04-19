# TDT 4130 Graphics Project

The code repository for my project in TDT 4130, for Spring 2020.

For more information, see the report.

## Cloning and building

Requirements: Cmake, a C++ compiler like GCC (which must handle posix threads).


To fetch the project and all submodules use: 
```
git clone --recursive https://github.com/hakonw/TDT4230-Project.git
```
Alternatively after pulling the repository:
```
git submodule update --init
```

### Building

To run the project, use `cmake` followed by `make`.

For linux, package dependencies are available in `./lib/ubuntu_debian_install_dependencies.sh`.

## Controls
 
* Movement:      WASD
* Up/Down:       E/Q
* Toggle box:    B
* Pause:         Mouse click 2
* Force shoot:   Mouse click 1
* Toggle Gimbal lock:   G (default off)
* Toggle Multi thread:  M (default off)
* Help:          F1
* Print Camera pos:    F3
* Show status:   F4
* Disable mouse: K
* Add/Subtract ships: F8/F7