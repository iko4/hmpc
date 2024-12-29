# hmpc Tests

> ⚠️ When writing tests, make sure to carefully use `sycl::queue::submit`.
> We experienced failed test cases caused by reused kernels across tests.

> ⚠️ When writing tests, make sure to use distinct party ids across tests.
> Otherwise, running tests in parallel could cause one test to intercept messages of another test, resulting in deadlocks.
> See [hmpc-rs](../hmpc-rs/tests/README.md).

