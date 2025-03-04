# hmpc Documentation

This document describes the basic concepts of [HPC](#high-performance-computing) and [MPC](#multiparty-computation).
Further details on the library can be found in the [library documentation](#library-documentation).


## High Performance Computing

*High performance computing* (HPC) [TODO]


## Multiparty Computation

*Multiparty computation* (MPC) is a field of cryptography with the goal of enabling parties to compute functions on distributed data without revealing information any about the inputs.
This enables private and secure computations; and additional properties like guarantees about the correctness of the result and guarantees towards external parties can be provided as well (depending on the used MPC protocol).

As an example, imagine you are at a dinner party with three other friends and you want to know who has the highest salary.
In a naive solution, all of you would tell their salary to each other and you can identify who earns most.
However, each of you reveals their salary to the others, which you might want to avoid.
With multiparty computation, you would only learn which one of you has the largest input value (their salary) and nothing more.

Naturally, private and secure computation through MPC can be very useful, for example, in medial use cases (where you want to keep patient data private) or machine learning (where the input data could be sensitive information; and the machine learning model could be secret intellectual property).
However, MPC adds overhead to a computation; in general, more overhead for MPC protocols with stronger security guarantees.
Therefore, efficient MPC implementations are essential to bring this technology to more use cases.

Note: Sometimes you can still learn some information about the inputs of parties from the output.
This depends on the inputs and the function you want to compute.
One solution to avoid this is [differential privacy (DP)](https://en.wikipedia.org/wiki/Differential_privacy), which can also be combined with MPC.

### Secret-Sharing

*Secret-sharing* is a technique to allow multiple parties to perform some computations on data, without knowing the underlying data.
Only if enough parties come together, they can reconstruct a secret together.
We use simple full-threshold secret-sharing, where all parties have to be involved to reconstruct the secret.
For a secret $x$ and $n$ parties, we write $[x]_i$ for party $P_i$'s share.
Reconstruction is simply the sum of all shares:

```math
x = \sum_{i = 0}^{n} [x]_i
```

By selecting all shares $[x]_i$ uniformly at random (under the condition that they sum up to a specific secret), a party that shares a secret can ensure that a single share, or even up to $n - 1$ shares, reveal no information about the secret.

### Homomorphic Encryption

[TODO]


## Library Documentation

See the corresponding subsections for documentation on

- [computing](computing.md)
- [networking](networking.md)
- [optional features](features.md)
