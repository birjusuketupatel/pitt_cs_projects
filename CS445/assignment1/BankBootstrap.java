/*
 * Birju Patel
 * CS 445
 * Dr. John Ramirez
 * 02/04/2020
 */

package assignment1;
import java.util.*;

public class BankBootstrap {
	
	public static void main(String[] args) {
		
		Scanner inScan = new Scanner(System.in);
		System.out.println("Welcome to the Bank Bootstrap Analysis!");
		System.out.println("We need some information to initialize your simulation...");
		System.out.print("How many tellers will you have? ");
		int ntell = Integer.parseInt(inScan.nextLine());
		System.out.print("How many hours will your bank be open? ");
		double hrs = Double.parseDouble(inScan.nextLine());
		System.out.print("How many customers do you expect per hour? ");
		double arr_rate = Double.parseDouble(inScan.nextLine());
		System.out.print("What is the average transaction time (in minutes) of a customer? ");
		double t_min = Double.parseDouble(inScan.nextLine());
		System.out.print("How many people can wait in your bank? ");
		int maxq = Integer.parseInt(inScan.nextLine());
		System.out.print("Enter a seed: ");
		long seed = Long.parseLong(inScan.nextLine());
		System.out.print("How many times do you want to run the simulation? ");
		int n = Integer.parseInt(inScan.nextLine());
		
		Random R = new Random(seed);
		int rand_seed = 0;
		double [] single_mean = new double[n];
		double [] multi_mean = new double[n];
		
		//runs n simulations of single queue bank
		for(int i = 0; i < n; i ++) {
			
			rand_seed = R.nextInt();
			SimBank temp_bank = new SimBank(ntell, true, hrs, arr_rate, t_min, maxq, rand_seed);
			temp_bank.runSimulation();
			single_mean[i] = temp_bank.getAvgWait();
		}
		
		//runs n simulations of multiple queue bank
		for(int i = 0; i < n; i ++) {
			
			rand_seed = R.nextInt();
			SimBank temp_bank = new SimBank(ntell, false, hrs, arr_rate, t_min, maxq, rand_seed);
			temp_bank.runSimulation();
			multi_mean[i] = temp_bank.getAvgWait();
		}
		
		double single_pop_mean = 0, multi_pop_mean = 0;
		
		for(int i = 0; i < n; i ++) {
			single_pop_mean = single_pop_mean + single_mean[i];
			multi_pop_mean = multi_pop_mean + multi_mean[i];
		}
		
		single_pop_mean = single_pop_mean / n;
		multi_pop_mean = multi_pop_mean / n;
		
		double stddev_single = 0, stddev_multi = 0;
		
		for(int i = 0; i < n; i ++) {
			
			stddev_single = (single_mean[i] - single_pop_mean) * (single_mean[i] - single_pop_mean) + stddev_single;
			stddev_multi = (multi_mean[i] - multi_pop_mean) * (multi_mean[i] - multi_pop_mean) + stddev_multi;
		}
		
		stddev_single = Math.sqrt(stddev_single / n);
		stddev_multi = Math.sqrt(stddev_multi / n);
		
		double c1_single = single_pop_mean - 2 * stddev_single;
		double c2_single = single_pop_mean + 2 * stddev_single;
		double c1_multi = multi_pop_mean - 2 * stddev_multi;
		double c2_multi = multi_pop_mean + 2 * stddev_multi;
		
		System.out.println("\nSYSTEM PARAMETERS:\n");
		System.out.println("number of tellers: " + ntell);
		System.out.printf("customer arrival rate (per hour): %.2f\n", arr_rate);
		System.out.printf("average length of service (mins): %.2f\n", t_min);
		System.out.println("number of simulations: " + n);
		
		System.out.println("\nSINGLE QUEUE:\n");
		System.out.printf("population mean of wait times: %.3f\n", single_pop_mean);
		System.out.printf("standard deviation in sample means: %.3f\n", stddev_single);
		System.out.printf("95 percent confidence interval: %.3f to %.3f\n", c1_single, c2_single);
		
		System.out.println("\nMULTIPLE QUEUES:\n");
		System.out.printf("population mean of wait times: %.3f\n", multi_pop_mean);
		System.out.printf("standard deviation in sample means: %.3f\n", stddev_multi);
		System.out.printf("95 percent confidence interval: %.3f - %.3f\n", c1_multi, c2_multi);
		
	}//main
	
}//BankBootstrap