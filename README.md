# hmpc

An HPC-inspired MPC library.

hmpc builds on concepts from [high performance computing (HPC)](docs/README.md#high-performance-computing) to enable efficient [multiparty computation (MPC)](docs/README.md#multiparty-computation) implementations.
The library includes communication and computing interfaces for parallel computations and asynchronous networking.
See the [documentation](docs/README.md) for the library concepts/design and the [changelog](CHANGELOG.md) for a history of additions and changes.

> ⚠️ This work is a work in progress and an academic prototype.
As such, some features are still missing (e.g., an MPC API built on lower-level primitives) and it is not meant to be used in production environments.


## Preparing a Build Environment 🧰

We tested the following containerized build environments for our software:
[Dev Containers](https://code.visualstudio.com/docs/devcontainers/tutorial) and
[Docker](https://www.docker.com/).
Note that our setup only accounts for CPU-based builds for Dev Containers.
We provide instructions for CPU-based and GPU-based builds using Docker.

The build environment builds a recent version of the [LLVM](https://llvm.org/)-based [oneAPI data parallel C++ compiler](https://github.com/intel/llvm) from source.
This might take some time.


### Dev Container

All files to create a build environment with Dev Containers are already set up.
Simply start Visual Studio Code and select "Reopen in Container" with the Dev Containers extension.
This basically automates the steps shown below for Docker.


### Docker

The following will create a Docker image called "hmpc" that has all required tools to build our software.

```bash
# build image
docker buildx build --tag hmpc --build-arg user_id="$(id -u)" --build-arg group_id="$(id -g)" --file .devcontainer/Containerfile .
```
After setting up the container, you can start it via
```bash
# run container
docker run --rm -it --mount type=bind,source="$(pwd)",target=/workspaces/hmpc hmpc
```

For CUDA-enabled containers (see the [Docker documentation](https://docs.docker.com/config/containers/resource_constraints/#gpu) and [Nvidia Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)), run
```bash
# Alternative:
# build image
docker buildx build --tag hmpc:cuda --build-arg user_id="$(id -u)" --build-arg group_id="$(id -g)" --build-context cuda="${CUDA_HOME:?}" --file .devcontainer/cuda/Containerfile .
# run container
docker run --rm -it --gpus all --mount type=bind,source="$(pwd)",target=/workspaces/hmpc --mount type=bind,source="${CUDA_HOME:?}",target="/opt/cuda" hmpc:cuda
```

Note:
For using CUDA, the "CUDA_HOME" environment variables needs to be set:
It should point to your installation of the CUDA toolkit.
Your installation path can vary based on your operating system and how you installed the CUDA toolkit.
You might have the "CUDA_PATH" variable set, then you can use the same value also for "CUDA_HOME".
It could be, for example: "/usr/local/cuda" or "/usr/local/cuda-11.6".


## Build 🏗

In the container or Dev Container, you can build our software using [CMake](https://cmake.org/).

```bash
# configure
cmake --preset default
# build
cmake --build --preset default
```

*Or*, if you want to build our software with CUDA support, use the following to configure and build.

```bash
# Alternative:
# configure
cmake --preset cuda
# build
cmake --build --preset cuda
```


## Running Tests 🧪

In the container or Dev Container, you can test our software using `ctest` (part of [CMake](https://cmake.org/)).

```bash
# test
ctest --preset default
```

*Or*, if you built our software with CUDA support, use the following to test after the build.

```bash
# Alternative:
# test
ctest --preset cuda
```
