lttng-parallel-analyses
=======================

This repository contains a parallel implementation of some of the lttng kernel
trace analyses found in the [LTTng Analyses](https://github.com/lttng/lttng-analyses)
repository.

# Requirements

- Qt >= 5.2.1
- Glib2.0 >=  2.40.2
- Scons >= 2.3.0


# Build

On Ubuntu:
```
sudo add-apt-repository -y ppa:boost-latest/ppa
sudo apt-add-repository -y ppa:lttng/ppa
sudo apt-get update
# Common dependencies
sudo apt-get install -y build-essential git subversion
# Babeltrace
sudo apt-get install -y libtool autoconf libglib2.0-dev bison flex uuid-dev libpopt-dev
# libdelorean + tigerbeetle
sudo apt-get install -y g++ pkg-config libcppunit-dev boost1.55 libboost1.55-dev scons
# lttng-parallel-analyses
sudo apt-get install -y qt5-default

git clone --recursive https://github.com/TheZorg/lttng-parallel-analyses.git
cd lttng-parallel-analyses
./bootstrap.sh
cd build
```
The executable will be in `lttng-parallel-analyses/build`.

If you did not include `--recursive` when doing `git clone`, you can use the following
command:

```
git clone https://github.com/TheZorg/lttng-parallel-analyses.git```
cd lttng-parallel-analyses
git submodule update --init --recursive
```

# Usage

The currently implemented analyses are:
- Count analysis: simple event count
- CPU analysis: % CPU usage per-CPU and per-TID
- I/O analysis: bytes read and written per-TID

Example usage:
```
./lttng-parallel-analyses --analysis cpu --thread 8 my-trace/kernel
```

Note: the `--balanced` parameter should be used whenever possible, but is not yet
implemented for the I/O analysis.
