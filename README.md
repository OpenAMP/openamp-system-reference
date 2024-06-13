# openamp-system-reference
End-to-end system reference material showcasing all the different aspects of OpenAMP, on multiple vendor platforms.

# Build legacy_apps

Legacy apps are moved from open_amp library repository.

## Build libmetal
```
    $ mkdir -p build-libmetal
    $ cd build-libmetal
    $ cmake <libmetal_source> -DCMAKE_INSTALL_PREFIX=<libmetal install dir>
    $ make VERBOSE=1 install
```

## Build open_amp
```
  $ mkdir -p build-openamp
  $ cd build-openamp
  $ cmake <openamp_source> -DCMAKE_INCLUDE_PATH=<libmetal_built_include_dir> \
        -DCMAKE_LIBRARY_PATH=<libmetal_built_lib_dir> \
	-DCMAKE_INSTALL_PREFIX=<open_amp install dir>
  $ make VERBOSE=1 install
```
## Build legacy Apps
```
  $ mkdir -p build-openamp-demos
  $ cd build-openamp-demos
  $ cmake <openamp_system_reference/examples/legacy_apps> \
        -DCMAKE_INCLUDE_PATH=<libmetal_built_include_dir> <openamp_built_include_dir> \
        -DCMAKE_LIBRARY_PATH=<libmetal_built_lib_dir> <openamp_built_lib_dir> \
	-DCMAKE_INSTALL_PREFIX=<open_amp_demos install dir>
  $ make VERBOSE=1 install
```

