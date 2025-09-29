# Dark Plugins

This is a set of open-source audio plugins in LV2 format, based on other existing open-source plugins.

They have been tailored for use within Darkglass.
Check the header on each `plugin.cpp` file for a list of changes for each plugin.

The LV2 URIs have been changed as the plugins are no longer compatible with the original versions.

## Building

A C++ compiler is required, GCC is assumed but clang is also supported.

To build simply run `make`. Typical environment variables like `CXX` and `CXXFLAGS` are respected.

The plugin binaries will be built inside each individual LV2 bundle.
There is no separate build folder.

The only required dependency is `lv2`, its headers must be installed/available on the default compiler paths.

## License

There is no global license file on this repository, as each adapted plugin has its own license.  
We follow the intentions of the original authors and keep the license from the original source code.
