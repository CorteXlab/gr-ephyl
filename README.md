# GR-EPHYL: GRC resources used in the EPHYL project

## What is it ?

- Here you can find all GNU Radio blocks and designs used in [EPHYL project](https://project.inria.fr/ephyl/) _(Enhanced Physical Layer for Cellular IoT)_.
- The blocks are developed in Python _(gr_modtool)_.
- After properly installing the module, have a look at the examples/ directory


## Requirements

- Ubuntu 16, Debian 8.10 or higher
- GNU Radio v3.7.10.1 or higher
- Matlab Runtime 2014a (MCR v8.3). Unless you don't need TurboFSK blocks, in this case have a look at the branch "no_turbofsk"
- Some basic knowledge of GNU Radio


## Installation

- Be sure to install resources where GNU Radio can find them (a.k.a <INSTALL_DIR>)
- Then run this:

```
git clone git://github.com/CorteXlab/gr-ephyl.git
# If you want to install from specific branch
# git clone -b <branch> git://github.com/CorteXlab/gr-ephyl.git

cd gr-ephyl
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> ..
make
make install
```

- Then you have to generate the 2 hierarchical blocks corresponding to the 2 classes of nodes of the design:
  - Go to _examples/_, open *hier_sensor.grc* and *hier_bs.grc* and generate their respective python files.
  - Reload GRC blocks 


## How to use

# Design

As mentioned before, there is two main classes/types of nodes:
- Base Station (BS) emulator _(hier_bs.grc)_
- IoT Sensor emulator, which is a network user in fact _(hier_sensor.grc)_


# Documentation

- In progress

