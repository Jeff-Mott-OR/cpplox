# Nystrom's [Crafting Interpreters](http://www.craftinginterpreters.com/): An Implementation in C++

## Build

    mkdir build
    cd build

    cmake ..
    cmake --build .

Or, to optionally build and run tests and benchmarking, use this cmake command instead:

    cmake .. -DENABLE_TESTING=TRUE

## Vagrant

This project comes with vagrant files to make it easier to build on a variety of platforms with a variety of compilers.

### Build with g++-7 or clang-5 on Ubuntu

    cd vagrant/ubuntu-xenial64-g++-7
    vagrant up

Or.

    cd vagrant/ubuntu-xenial64-clang-5
    vagrant up

#### Build

In the VM, run:

    mkdir build
    cd build

    cmake /cpplox
    cmake --build .

### Build with MSVC 2017

    cd vagrant/w16s-vs17c
    vagrant up

#### First time boot manual steps

1. Install CMake. The installer will be on the desktop. Select the option to add cmake to the system PATH.
2. Install Visual Studio's _Desktop Development with C++_. Navigate to _Control Panel_ > _Programs and Features_. Select Microsoft Visual Studio and press Change. Update the Visual Studio installer, then update Visual Studio itself. Finally, modify the installation and tick the box to install Desktop Development with C++.

#### Build

In the VM, open a command prompt and run:

    pushd \\vboxsvr\cpplox

    mkdir build
    cd build

    cmake .. -A x64
    cmake --build . --config Release

### Proxy

Are you behind a proxy? I feel your pain. Start by running:

    vagrant plugin install vagrant-proxyconf

Then in the `Vagrantfile`s, uncomment the `config.proxy` lines and enter your real proxy values.

## Copyright

Copyright 2018 Jeff Mott. [MIT License](https://opensource.org/licenses/MIT).
