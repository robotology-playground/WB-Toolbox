![](http://drive.google.com/uc?export=view&id=0B6zDGh11iY6oc0gtM0lMdDNweWM)
# Whole Body Toolbox
<h3 style="text-align: center">A Simulink Toolbox for Whole Body Control</h3>

[![GitPitch](https://gitpitch.com/assets/badge.svg)](https://gitpitch.com/robotology/WB-Toolbox?grs=github)

### Main Goal

> The library should allow non-programming experts or those researchers just getting acquainted with Whole Body Control to more easily deploy controllers either on simulation or real YARP-based robotic platforms, and to analyze their performance taking advantage of the innumerable MATLAB and Simulink toolboxes. We like to call it "rapid controller prototyping" after which a proper YARP module should be made for hard real time performance and final deployment.

The following video shows CoDyCo's latest results on iCub in which the top level controller has been implemented with the [WB-Toolbox](https://github.com/robotology/WB-Toolbox) running at a 10ms rate!

<p align="center">
<a href="https://youtu.be/UXU3KSa201o
" target="_blank"><img src="http://img.youtube.com/vi/VrPBSSQEr3A/0.jpg"
alt="iCub balancing on one foot via external force control and interacting with humans" width="480" height="360" border="10" /></a>
</p>

- [Installation](https://github.com/robotology/WB-Toolbox#Installation)
- [Troubleshooting](https://github.com/robotology/WB-Toolbox#Troubleshooting)
- [Using the Toolbox](https://github.com/robotology/WB-Toolbox#using-the-toolbox)
- [Migrating from WBI-Toolbox](https://github.com/robotology/WB-Toolbox#migrating-from-wbi-toolbox)
- [Migrating from WB-Toolbox 2.0](https://github.com/robotology/WB-Toolbox#migrating-from-wbi-toolbox-2.0)
- [Modifying the Toolbox](https://github.com/robotology/WB-Toolbox#modifyng-the-toolbox)
- [Citing this work](https://github.com/robotology/WB-Toolbox#citing-this-work)

## Installation

### Requirements
* Matlab V. 7.1+ and Simulink (Tested with Matlab `R2015b`, `R2014a/b`, `R2013a/b`, `R2012a/b`)
* [`YARP`](https://github.com/robotology/yarp) (Compiled as **shared library**. Currently a default yarp configuration option)
* [`iDynTree`](https://github.com/robotology/idyntree)
* Supported Operating Systems: **Linux**, **macOS**,  **Windows**

**Note:** You can install the dependencies either manually or by using the [codyco-superbuild](https://github.com/robotology/codyco-superbuild).

### Optional requirements
* [`iCub`](https://github.com/robotology/icub-main) (needed for some blocks)
* [Gazebo Simulator](http://gazebosim.org/)
* [`gazebo_yarp_plugins`](https://github.com/robotology/gazebo_yarp_plugins).

**Note: The following instructions are for *NIX systems, but it works similarly on the other operating systems.**

### Compiling the C++ Code (Mex File)

**Prerequisite: Check the [Matlab configuration](https://github.com/robotology/WB-Toolbox#matlab-configuration) section.**

Before going ahead with the compilation of the library, make sure that you have MATLAB and Simulink properly installed and running. Then, check that the MEX compiler for MATLAB is setup and working. For this you can try compiling some of the MATLAB C code examples as described in the [mex official documentation](https://www.mathworks.com/help/matlab/ref/mex.html).

**If you installed Matlab in a location different from the default one, please set an environmental variable called either `MATLABDIR` or `MATLAB_DIR` with the root of your Matlab installation**, e.g. add a line to your `~/.bashrc` such as: `export MATLAB_DIR=/usr/local/bin/matlab`.

If you used the [codyco-superbuild](https://github.com/robotology/codyco-superbuild) you can skip this step and go directly to the Matlab configuration step.

Execute:

```sh
git clone https://github.com/robotology/WB-Toolbox.git
mkdir -p WB-Toolbox/build
cd WB-Toolbox/build
cmake ..
```

**Note:** We suggest to install the project instead of using it from the build directory. You should thus change the default installation directory by configuring the `CMAKE_INSTALL_PREFIX` variable. You can do it before running the first `cmake` command by calling `cmake .. -DCMAKE_INSTALL_PREFIX=/your/new/path`, or by configuring the project with `ccmake .`

Compile and install the toolbox as follows:

```sh
make
make install
```

### Matlab configuration

In the following section we assume the `install` folder is either the one you specified in the previous step by using `CMAKE_INSTALL_PREFIX` or if you used the `codyco-superbuild`, is `/path/to/superbuild/build_folder/install`.

In order to use the WB-Toolbox in Matlab you have to add the `mex` and `share/WB-Toolbox` to the Matlab path.

```bash
    addpath(['your_install_folder'  /mex])
    addpath(genpath(['your_install_folder'  /share/WB-Toolbox]))
```

You can also use (only once after the installation) the script `startup_wbitoolbox.m` that you can find in the `share/WB-Toolbox` subdirectory of the install folder to properly configure Matlab.
**Note:** This script configures Matlab to always add the WB-Toolbox to the path. This assumes Matlab is always launched from the `userpath` folder.

- **Launching Matlab** By default, Matlab has different startup behaviours depending on the Operating Systems and how it is launched. For Windows and macOS (if launched in the Finder) Matlab starts in the `userpath` folder. If this is your common workflow you can skip the rest of this note. Instead, if you launch Matlab from the command line (Linux and macOS users), Matlab starts in the folder where the command is typed, and thus the `path.m` file generated in the previous phase is no longer loaded. You thus have two options:
  -  create a Bash alias, such that Matlab is always launched in the `userpath`, e.g.
  ```
     alias matlab='cd ~/Documents/MATLAB && /path/to/matlab
  ```
    (add the above line in your `.bashrc`/`.bash_profile` file)
  -  create a `startup.m` file in your `userpath` folder and add the following lines
  ```
    if strcmp(userpath, pwd) == 0
        path(pathdef())
    end
  ```


- **Robots' configuration files** Each robot that can be used through the Toolbox has its own configuration file. WB-Toolbox uses the Yarp [`ResourceFinder`](http://www.yarp.it/yarp_resource_finder_tutorials.html). You should thus follow the related instruction to properly configure your installation (e.g. set the `YARP_DATA_DIRS` variable)

## Troubleshooting

### Problems finding libraries and libstdc++

In case Matlab has trouble finding a specific library, a workaround is to launch it preloading the variable `LD_PRELOAD` (or `DYLD_INSERT_LIBRARIES` on macOS) with the full path of the missing library. On Linux you might also have trouble with `libstdc++.so` since Matlab comes with its own. To use your system's `libstdc++` you would need to launch Matlab something like (replace with your system's `libstdc++` library):

`LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 matlab`

You could additionally create an alias to launch Matlab this way:

`alias matlab_codyco="cd ~/Documents/MATLAB && LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 matlab"`

A more permanent and scalable solution can be found in https://github.com/robotology/codyco-superbuild/issues/141#issuecomment-257892256.

### `YARP` not installed in the system default directory

In case you compiled `YARP` in a directory different from the system default one and you are not using RPATH, you need to tell to MATLAB the location in which to find the shared libraries for `YARP`. If you launch MATLAB from command line, this task is already done for you by `bash` (if you edited `.bashrc`). If you launch MATLAB from the UI (e.g. on macOS by double clicking the application icon) you need to further add the variables in `${MATLAB_ROOT}/bin/.matlab7rc.sh` by first doing

```bash
    chmod +w .matlab7rc.sh
```

Then looking for the variable `LDPATH_SUFFIX` and assign to every instance the contents of your `DYLD_LIBRARY_PATH`. Finally do:

```bash
    chmod -w .matlab7rc.sh
```

The error message you get in this case might look something like:

```bash
Library not loaded: libyarpwholeBodyinterface.0.0.1.dylib
Referenced from:
${CODYCO_SUPERBUILD_DIR}/install/mex/robotState.mexmaci64
```

## Using the Toolbox

The Toolbox assumes the following information / variables to be defined:

#### Environment variables

If you launch Matlab from command line, it inherits the configuration from the `.bashrc` or `.bash_profile` file. If you launch Matlab directly from the GUI you should define this variables with the Matlab function `setenv`.

- `YARP_ROBOT_NAME`
- `YARP_DATA_DIRS`

#### Creating a model

Before using or creating a new model keep in mind that WB-Toolbox is discrete in principle and your simulation should be discrete as well. By going to `Simulation > Configuration Parameters > Solver` you should change the solver options to `Fixed Step` and use a `discrete (no continuous states)` solver.
r
In order to start dragging and dropping blocks from the Toolbox, open the Simulink Library Browser and search for `Whole Body Toolbox` in the tree view.

## Migrating

#### From WBI-Toolbox (1.0)

Please see [here](doc/Migration_from_WBI-Toolbox_1.0.md).

#### Migrating from WB-Toolbox 2.0

Please see [here](doc/Migration_from_WB-Toolbox_2.0.md).

## Modifying the Toolbox

If you want to modify the toolbox, please check developers documentation:
- [Add a new C++ block](doc/dev/create_new_block.md)
- [Tip and tricks on creating simulink blocks](doc/dev/sim_tricks.md)

## Citing this work

Please cite the following publication if you are using WB-Toolbox for your own research and/or robot controllers

> Romano, F., Traversaro, S., Pucci, D., Del Prete, A., Eljaik, J., Nori, F. "A Whole-Body Software Abstraction layer for Control Design of free-floating Mechanical Systems", 1st IEEE International Conference on Robotic Computing, 2017.

Bibtex citation:
```
@INPROCEEDINGS{RomanoWBI17,
author={F. Romano and S. Traversaro and D. Pucci and A. Del Prete and J. Eljaik and F. Nori},
booktitle={2017 IEEE 1st International Conference on Robotic Computing},
title={A Whole-Body Software Abstraction layer for Control Design of free-floating Mechanical Systems},
year={2017},
}
```

#### Tested OS

Linux, macOS, Windows.
