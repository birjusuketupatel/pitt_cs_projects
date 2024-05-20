#include "include/Scheduler.h"
#include <cstring>
#include <string>

void testDeadlockNormal()
{
	std::cout << "===Running deadlock (1) test===" << std::endl;
	Scheduler scheduler = Scheduler();
	// Create table `Deadlock`
	scheduler.schedule(0, "B 1");
	scheduler.schedule(0, "I Deadlock (0, Apple, 5, PGH)");
	scheduler.schedule(0, "C"); // Commit

	// Create two new transaction
	scheduler.schedule(1, "B 1");
	scheduler.schedule(2, "B 1");
	/**
	 * Inserting into table `Deadlock` only requires a record-granularity lock
	 * since the table already exists (due to previous committed transaction)
	 */
	scheduler.schedule(1, "I Deadlock (1, Banana, 5, NYC)");
	scheduler.schedule(2, "I Deadlock (2, Orange, 5, LA)");
	// Transaction 1 is blocked since it needs to wait for transaction 2 to release the lock
	scheduler.schedule(1, "R Deadlock 2");
	// Transaction 2 is blocked since it needs to wait for transaction 1 to release the lock
	// Deadlock occurs here - scheduler should detect this and abort one of the transactions
	scheduler.schedule(2, "R Deadlock 1");

	scheduler.schedule(2, "C");
	scheduler.schedule(1, "C");

	scheduler.schedule(1, "B 1");
	scheduler.schedule(2, "B 1");
	scheduler.schedule(3, "B 1");
	scheduler.schedule(4, "B 1");
	/**
	 * Inserting into table `Deadlock` only requires a record-granularity lock
	 * since the table already exists (due to previous committed transaction)
	 */
	scheduler.schedule(3, "I Deadlock (3, Banana, 5, NYC)");
	scheduler.schedule(4, "I Deadlock (4, Orange, 5, LA)");
	scheduler.schedule(1, "C Deadlock");
	scheduler.schedule(2, "C Deadlock");
	// Transaction 1 is blocked since it needs to wait for transaction 2 to release the lock
	scheduler.schedule(3, "T Deadlock");
	// Transaction 2 is blocked since it needs to wait for transaction 1 to release the lock
	// Deadlock occurs here - scheduler should detect this and abort one of the transactions
	scheduler.schedule(4, "T Deadlock");

	scheduler.schedule(4, "C");
	scheduler.schedule(3, "C");
	scheduler.schedule(2, "C");
	scheduler.schedule(1, "C");

	scheduler.printStat();
	scheduler.closeAllTables();

}


void testDeadlockBenchmark()
{
	std::cout << "===Running deadlock (2) test===" << std::endl;
	Scheduler scheduler = Scheduler();
	// Create table `Deadlock`
	scheduler.schedule(0, "B 1");
	scheduler.schedule(0, "D LockedDB");
	scheduler.schedule(0, "I LockedDB (0, Apple, 5, PGH)");
	scheduler.schedule(0, "C"); // Commit

	scheduler.schedule(1, "B 1");
	scheduler.schedule(2, "B 1");
	scheduler.schedule(3, "B 1");
	scheduler.schedule(4, "B 1");
	scheduler.schedule(5, "B 1");
	scheduler.schedule(6, "B 1");
	scheduler.schedule(7, "B 1");
	scheduler.schedule(8, "B 1");
	scheduler.schedule(9, "B 1");
	scheduler.schedule(10, "B 1");

	scheduler.schedule(1, "I LockedDB (1, Banana, 5, NYC)");
	scheduler.schedule(2, "I LockedDB (2, Orange, 5, LA)");
	scheduler.schedule(3, "I LockedDB (3, Banana, 5, NYC)");
	scheduler.schedule(4, "I LockedDB (4, Orange, 5, LA)");
	scheduler.schedule(5, "I LockedDB (5, Banana, 5, NYC)");
	scheduler.schedule(6, "I LockedDB (6, Orange, 5, LA)");
	scheduler.schedule(7, "I LockedDB (7, Banana, 5, NYC)");
	scheduler.schedule(8, "I LockedDB (8, Orange, 5, LA)");
	scheduler.schedule(9, "I LockedDB (9, Banana, 5, NYC)");
	scheduler.schedule(10, "I LockedDB (10, Orange, 5, LA)");

	scheduler.schedule(10, "R LockedDB 9");
	scheduler.schedule(9, "R LockedDB 8");
	scheduler.schedule(8, "R LockedDB 7");
	scheduler.schedule(7, "R LockedDB 6");
	scheduler.schedule(6, "R LockedDB 5");
	scheduler.schedule(5, "R LockedDB 4");
	scheduler.schedule(4, "R LockedDB 3");
	scheduler.schedule(3, "R LockedDB 2");
	scheduler.schedule(2, "R LockedDB 1");
	scheduler.schedule(1, "T LockedDB"); // Only this should deadlock

	scheduler.schedule(10, "C");
	scheduler.schedule(8, "C");
	scheduler.schedule(6, "C");
	scheduler.schedule(4, "C");
	scheduler.schedule(2, "C");
	scheduler.schedule(1, "C");
	scheduler.schedule(3, "C");
	scheduler.schedule(5, "C");
	scheduler.schedule(7, "C");
	scheduler.schedule(9, "C");

	scheduler.schedule(0, "B 1");
	scheduler.schedule(0, "D LockedDB");
	scheduler.schedule(0, "C"); // Commit
	scheduler.printStat();
	exit(0);
}


void testConcurrencyBenchMark()
{
	Scheduler scheduler = Scheduler();
	scheduler.schedule(0, "B 1");
	scheduler.schedule(0, "I Concurrent (0, Apple, 5, PGH)");
	scheduler.schedule(0, "C");

	scheduler.schedule(1, "B 1"); // Transaction
	scheduler.schedule(2, "B 0"); // Process
	scheduler.schedule(3, "B 1"); // Transaction
	scheduler.schedule(4, "B 0"); // Process
	scheduler.schedule(5, "B 1"); // Transaction
	
	scheduler.schedule(1, "I Concurrent (1, Apple, 5, PGH)");
	scheduler.schedule(2, "I Concurrent (2, Apple, 5, PGH)");	
	scheduler.schedule(3, "I Concurrent (3, Apple, 5, PGH)");
	scheduler.schedule(4, "I Concurrent (4, Apple, 5, PGH)");
	scheduler.schedule(5, "I Concurrent (5, Apple, 5, PGH)");
	
	
	scheduler.schedule(5, "T Concurrent");
	scheduler.schedule(3, "T Concurrent");
	scheduler.schedule(4, "T Concurrent");
	scheduler.schedule(2, "T Concurrent");	
	scheduler.schedule(1, "T Concurrent");
	scheduler.schedule(1, "C");
	scheduler.schedule(2, "C");
	scheduler.schedule(3, "C");
	scheduler.schedule(4, "C");
	scheduler.schedule(5, "C");

	scheduler.schedule(6, "B 0"); 
	scheduler.schedule(7, "B 1");
	scheduler.schedule(8, "B 0");
	scheduler.schedule(9, "B 1");
	scheduler.schedule(10, "B 0");
	scheduler.schedule(6, "I Concurrent (6, Apple, 5, PGH)");
	scheduler.schedule(7, "I Concurrent (7, Apple, 5, PGH)");
	scheduler.schedule(8, "I Concurrent (8, Apple, 5, PGH)");
	scheduler.schedule(9, "I Concurrent (9, Apple, 5, PGH)");
	scheduler.schedule(10, "I Concurrent (10, Apple, 5, PGH)");
	scheduler.schedule(10, "T Concurrent");
	scheduler.schedule(10, "C");
	scheduler.schedule(9, "T Concurrent");
	scheduler.schedule(8, "T Concurrent");
	scheduler.schedule(7, "R Concurrent 1");
	scheduler.schedule(6, "R Concurrent 1");
	scheduler.schedule(6, "C");
	scheduler.schedule(7, "C");
	scheduler.schedule(8, "C");
	scheduler.schedule(9, "C");
		
	scheduler.printStat();
	exit(0);
}

void testConcurrency()
{
	Scheduler scheduler = Scheduler();
	scheduler.schedule(0, "B 1");
	scheduler.schedule(1, "B 1"); // Transaction
	scheduler.schedule(2, "B 0"); // Process

	scheduler.schedule(0, "I Concurrent (0, Apple, 5, PGH)");
	scheduler.schedule(1, "T Concurrent ");
	scheduler.schedule(2, "T Concurrent ");

	scheduler.schedule(0, "C");
	scheduler.schedule(1, "C");
	scheduler.schedule(2, "C");
	scheduler.printStat();
	scheduler.closeAllTables();
}

void runTests(string arg)
{
	if(arg == "deadlock-normal")
	{
		std::cout << "This test case demonstrates that PittRDBC is deadlock-free. Namely, it shows that transactions that create a deadlock is aborted." << std::endl;
		testDeadlockNormal();
	}
	else if(arg == "deadlock-benchmark")
	{
		std::cout << "This test case stress tests PittRDBC's deadlock-free property. Namely, it creates a 10-chain deadlock, but demonstrates that PittRDBC only aborts the minimum number of transaction to ensure high throughput." << std::endl;
		testDeadlockBenchmark();
	}
	else if(arg == "concurrency-normal")
	{
		std::cout << "This test case demonstrates PittRDBC's concurrency abilities. Namely, it shows the difference between Processes (which can read uncomitted data) and Transactions; It also demonstrates how operations can be re-ordered on-the-fly to meet ACID guarantees." << std::endl;
		testConcurrency();
	}
	else if(arg == "concurrency-benchmark")
	{
		std::cout << "This test case demonstrates PittRDBC's concurrency abilities by putting it through a stress test with 10 concurrent operations (5 process | 5 transaction)" << std::endl;
		testConcurrencyBenchMark();
	}
	else
	{
		std::cerr << "Invalid test name:\t" << arg << std::endl;
		std::cerr << "Available Tests:\t deadlock-normal deadlock-benchmark concurrency-normal concurrency-benchmark" << std::endl;
		exit(1);
	}
}