# System Reference Legacy Apps Build Overview

This page details how to setup and build the
[OpenAMP System Reference](https://github.com/OpenAMP/openamp-system-reference/) legacy apps
and test examples found in the examples/legacy_apps directory.
These applications were moved from the [OpenAMP repository](https://github.com/OpenAMP/open-amp).


## Initialization

The first step is to initialize the workspace folder (``my-workspace``) where
the examples and all Zephyr modules will be cloned. You can do
that by running:

```shell
# initialize my-workspace for the example-application (main branch)
west init -m https://github.com/OpenAMP/openamp-system-reference --mf examples/legacy_apps/west.yml --mr main my-workspace
# update modules
cd my-workspace
west update
```

## Build

The following steps are to build legacy apps on a host machine.

```shell
export PROJECT_ROOT=$PWD
```

### Build libmetal
```shell
cd $PROJECT_ROOT/libmetal
cmake . -Bbuild -DCMAKE_INSTALL_PREFIX=$PROJECT_ROOT/target
make -C build VERBOSE=1 install
```

### Build open_amp
```shell
cd $PROJECT_ROOT/open-amp
cmake . -Bbuild -DCMAKE_INCLUDE_PATH=$PROJECT_ROOT/libmetal/build/lib/include/   -DCMAKE_LIBRARY_PATH=$PROJECT_ROOT/libmetal/build/lib/ -DCMAKE_INSTALL_PREFIX=$PROJECT_ROOT/target
make -C build VERBOSE=1 install
```
### Build legacy Apps
```shell
cd $PROJECT_ROOT/openamp-system-reference/examples/legacy_apps
cmake -Bbuild \
-DCMAKE_INCLUDE_PATH="$PROJECT_ROOT/libmetal/build/lib/include/;$PROJECT_ROOT/open-amp/build/lib/include/" \
-DCMAKE_LIBRARY_PATH="$PROJECT_ROOT/libmetal/build/lib/;$PROJECT_ROOT/open-amp/build/lib/" \
-DCMAKE_INSTALL_PREFIX=$PROJECT_ROOT/target
make -C build VERBOSE=1 install
```
