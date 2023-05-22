# litebrowser-stereokit

A simple browser based on the [litehtml](https://github.com/litehtml/litehtml) engine for StereoKit.

## Linux pre-requisites

Linux users will need to install some pre-requisites.

```shell
sudo apt-get update
sudo apt-get install build-essential cmake unzip libfontconfig1-dev libgl1-mesa-dev libvulkan-dev libx11-xcb-dev libxcb-dri2-0-dev libxcb-glx0-dev libxcb-icccm4-dev libxcb-keysyms1-dev libxcb-randr0-dev libxrandr-dev libxxf86vm-dev mesa-common-dev libjsoncpp-dev libxfixes-dev libglew-dev
```

### Building

You can build litebrowser with CMake/Visual Studio 2022 and newer. **Note**: this project contains the git submodule:

  * [litehtml](https://github.com/bclnet/litehtml) - html rendering engine

Please be sure the submodule is fetched, or download from github.

Here's a quick example of how to compile and build this using the CLI. If something is going wrong, sometimes adding in a `-v` for verbose will give you some additional info you might not see from VS Code.

```shell
# From the project root directory

# Make a folder to build in
mkdir build
cd build

# Configure the build
cmake .. -DCMAKE_BUILD_TYPE=Debug
# Build
cmake --build . -j8 --config Debug

# Run the app
./litebrowser
```


### Using litebrowser

Currently the address bar of the litebrowser is *fake*. Run litebrowser with command line parameter:
```
litebrowser.exe http://www.litehtml.com
```

If you run litebrowser without parameter, the dmoz-odp.org will be opened.