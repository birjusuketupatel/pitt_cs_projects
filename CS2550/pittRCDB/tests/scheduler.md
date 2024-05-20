# Tests for Scheduler
> Below are descriptions of test cases contained within [Tests.cpp](./src/Tests.cpp)
>
## Concurrency
- Normal: This test case demonstrates PittRDBC's concurrency abilities. Namely, it shows the difference between Processes (which can read uncomitted data) and Transactions; It also demonstrates how operations can be re-ordered on-the-fly to meet ACID guarantees. For instance, one can examine that the "T" operation in transaction mode cannot read the table if another transaction is writing to it, whereas the same operation in Process mode can.
- Benchmark: This test case demonstrates PittRDBC's concurrency abilities by putting it through a stress test with 10 concurrent operations (5 process | 5 transaction). It shows that operations can be reordered in a way that enforces serizable properties of a transaction (for those in transaction mode).

## Deadlocks
- Normal: This test case demonstrates that PittRDBC is deadlock-free. Namely, it shows that transactions that create a deadlock is aborted. For instance, if transaction 1 and transaction 2 fall into a cyclical deadlock, then the later transaction is aborted, without restart.
- Benchmark: "This test case stress tests PittRDBC's deadlock-free property. Namely, it creates a 10-chain deadlock, but demonstrates that PittRDBC only aborts the minimum number of transaction to ensure high throughput.
