# LIBXSMM-DNN

[![BSD 3-Clause License](https://img.shields.io/badge/license-BSD3-blue.svg "BSD 3-Clause License")](LICENSE.md) [![Correctness](https://badge.buildkite.com/0cffdb31ced5ab25ab3973c65d8f9130772d35d5813afb11ac.svg?branch=main "Correctness")](https://buildkite.com/intel/libxsmm-dnn) [![Performance](https://badge.buildkite.com/4b3cfd12f50569db8d82e35b273d147cffa056a863f8e16c0e.svg?branch=performance "Performance")](https://buildkite.com/intel/tpp-libxsmm) [![Read the Docs](https://readthedocs.org/projects/libxsmm/badge/?version=latest "Read the Docs")](https://libxsmm.readthedocs.io/#deep-learning)

LIBXSMM-DNN is a reference implementation of Deep Neural Network primitives using [LIBXSMM](https://libxsmm.readthedocs.io/#deep-learning)'s Tensor Processing Primitives (TPP).

To checkout the source code:

```bash
git pull --recurse-submodules
git submodule update --init --recursive
```

To build from source:

```bash
make -j $(nproc)
```

> **NOTE**: To rely on a specific version of LIBXSMM (Git submodule not required), set `LIBXSMMROOT` environment/make variable and rely on a [separately built](https://libxsmm.readthedocs.io/#build-instructions) LIBXSMM.

To mimic the performance of ResNet-50 inference:

```bash
tests/conv/run_convs.sh 1 1000 -1 f32 F 0 0 64 64 1 | tee output.log
```

> **NOTE**: To mimic performance, FLOPS are shown and the time spent for convolutional layers is measured. This is different from running inference end-to-end using a real framework such as PyTorch).
