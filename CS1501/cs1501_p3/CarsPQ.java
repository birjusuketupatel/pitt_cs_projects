package cs1501_p3;

import java.io.File;
import java.io.IOException;
import java.util.NoSuchElementException;
import java.util.Scanner;


public class CarsPQ implements CarsPQ_Inter {
	/*
	 * QueuePairs are put in a hash table
	 * first key is "general", all other keys are in the form "make,model"
	 * "general": PQ that contains all cars
	 * "make,model": PQ that contains all cars of a specific make and model
	 * table entry is deleted if PQ has 0 items
	 */
	LinearProbingHashST<String,QueuePair> table;
	
	public CarsPQ() {
		table = new LinearProbingHashST<String,QueuePair>();
		QueuePair general = new QueuePair();
		table.put("general", general);
	}//CarsPQ
	
	public CarsPQ(String file) throws IllegalStateException{
		table = new LinearProbingHashST<String,QueuePair>();
		QueuePair general = new QueuePair();
		
		//scans all words from file to queue
		try (Scanner s = new Scanner(new File(file))) {
			s.nextLine(); //skips first line
			String line = "";
			String[] values;
			
			while (s.hasNext()) {
				//lines are in the format
				//VIN:make:model:price:mileage:color
				line = s.nextLine();
				values = line.split(":");
				Car car = new Car(values[0], values[1], values[2], Integer.parseInt(values[3]), Integer.parseInt(values[4]), values[5]);
				//key in format make,model
				String key = car.getMake() + "," + car.getModel();
				
				//if queue exists for car's make, model, set makeQueue to it
				//else create a new queue
				if(table.contains(key)) {
					QueuePair makeQueue = table.get(key);
					makeQueue.add(car);
					table.put(key, makeQueue);
				}
				else {
					QueuePair newQueue = new QueuePair();
					newQueue.add(car);
					table.put(key, newQueue);
				}
				//add car to general queue always
				general.add(car);
			}
			table.put("general", general);
		}
		catch (IOException e) {
			e.printStackTrace();
		}
		catch(IllegalStateException s) {
			throw s;
		}
	}//CarsPQ
	
	/*
	 * adds car to general queue and queue for its make and model
	 * creates new queue for make and model if one does not exist
	 */
	public void add(Car newCar) throws IllegalStateException {
		String key = newCar.getMake() + "," + newCar.getModel();
		
		//always add car to general queue
		QueuePair general = table.get("general");
		
		QueuePair makeQueue = new QueuePair();
		
		//if queue exists for car's make, model, set makeQueue to it
		if(table.contains(key)) {
			makeQueue = table.get(key);
		}
		
		try {
			general.add(newCar);
			makeQueue.add(newCar);
		}
		catch(IllegalStateException e) {
			throw e;
		}
		
		table.put("general", general);
		table.put(key, makeQueue);
		
		return;
	}//add
	
	/*
	 * returns car associated with the vin given
	 * throws exception if vin is not in queue
	 */
	public Car get(String vin) throws NoSuchElementException {
		//check general queue for vin
		QueuePair general = table.get("general");
		Car car;
		
		try {
			car = general.get(vin);
		}
		catch(NoSuchElementException e) {
			throw e;
		}
		
		return car;
	}//get
	
	/*
	 * updates price of car in general queue and queue for car's make, model
	 */
	public void updatePrice(String vin, int newPrice) throws NoSuchElementException {
		Car car = this.get(vin);
		String key = car.getMake() + "," + car.getModel();
		
		QueuePair general = table.get("general");
		QueuePair makeQueue = table.get(key);
		
		try {
			general.updatePrice(vin, newPrice);
			makeQueue.updatePrice(vin, newPrice);
		}
		catch(NoSuchElementException e) {
			throw e;
		}
		
		table.put("general", general);
		table.put(key, makeQueue);
		
		return;
	}//updatePrice
	
	/*
	 * updates mileage of car in general queue and queue for car's make, model
	 */
	public void updateMileage(String vin, int newMileage) throws NoSuchElementException {
		Car car = this.get(vin);
		String key = car.getMake() + "," + car.getModel();
		
		QueuePair general = table.get("general");
		QueuePair makeQueue = table.get(key);
		
		try {
			general.updateMileage(vin, newMileage);
			makeQueue.updateMileage(vin, newMileage);
		}
		catch(NoSuchElementException e) {
			throw e;
		}
		
		table.put("general", general);
		table.put(key, makeQueue);
		
		return;
	}//updateMileage
	
	/*
	 * updates mileage of car in general queue and queue for car's make, model
	 */
	public void updateColor(String vin, String newColor) throws NoSuchElementException {
		Car car = this.get(vin);
		
		String key = car.getMake() + "," + car.getModel();
		
		QueuePair general = table.get("general");
		QueuePair makeQueue = table.get(key);
		
		try {
			general.updateColor(vin, newColor);
			makeQueue.updateColor(vin, newColor);
		}
		catch(NoSuchElementException e) {
			throw e;
		}
		
		table.put("general", general);
		table.put(key, makeQueue);
		
		return;
	}//updateColor
	
	/*
	 * removes a car from general and specific queues
	 */
	public void remove(String vin) throws NoSuchElementException {
		Car car = this.get(vin);
		String key = car.getMake() + "," + car.getModel();
		
		QueuePair general = table.get("general");
		QueuePair makeQueue = table.get(key);
		
		try {
			general.remove(vin);
			makeQueue.remove(vin);
		}
		catch(NoSuchElementException e) {
			throw e;
		}
		
		//if the specific queue is now empty, removes from table
		if(makeQueue.getCount() == 0) {
			table.delete(key);
			table.put("general", general);
		}
		else {
			table.put("general", general);
			table.put(key, makeQueue);
		}
		
		return;
	}//remove
	
	/*
	 * gets car in general queue with the lowest price
	 */
	public Car getLowPrice() {
		QueuePair general = table.get("general");
		return general.getLowPrice();
	}//getLowPrice
	
	/*
	 * gets car in general queue with the lowest mileage
	 */
	public Car getLowMileage() {
		QueuePair general = table.get("general");
		return general.getLowMileage();
	}//getLowPrice
	
	/*
	 * gets car of specific make and model with lowest price
	 * if no such car exists, return null
	 */
	public Car getLowPrice(String make, String model) {
		String key = make + "," + model;
		
		//if there is no car of that make and model, return null
		if(table.contains(key) == false) {
			return null;
		}
		
		QueuePair makeQueue = table.get(key);
		
		return makeQueue.getLowPrice();
	}//getLowPrice
	
	/*
	 * gets car of specific make and model with lowest mileage
	 * if no such car exists, return null
	 */
	public Car getLowMileage(String make, String model) {
		String key = make + "," + model;
		
		//if there is no car of that make and model, return null
		if(table.contains(key) == false) {
			return null;
		}
		
		QueuePair makeQueue = table.get(key);
		
		return makeQueue.getLowMileage();
	}//getLowPrice
	
	public String toString() {
		QueuePair general = table.get("general");
		return general.toString();
	}//toString
}//CarsPQ
