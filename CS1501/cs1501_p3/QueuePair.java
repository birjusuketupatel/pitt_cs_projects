/*
 * stores one queue prioritizing price, one queue prioritizing mileage
 * implements all methods of CarsPQ_Inter except those based on make and model
 */
package cs1501_p3;

import java.util.NoSuchElementException;

public class QueuePair {
	private IndirectedPQ<String,CarPrice> priceQueue;
	private IndirectedPQ<String,CarMileage> mileageQueue;
	private int count; //tracks the number of cars either list
	
	public QueuePair() {
		priceQueue = new IndirectedPQ<String,CarPrice>();
		mileageQueue = new IndirectedPQ<String,CarMileage>();
		count = 0;
	}//CarsPQ
	
	/*
	 * adds cars to both lists
	 * if car is already in one of the lists, throws exception
	 */
	public void add(Car newCar) throws IllegalStateException{
		if(priceQueue.contains(newCar.getVIN()) || mileageQueue.contains(newCar.getVIN())) {
			throw new IllegalStateException("duplicate VIN not allowed");
		}
		else {
			CarPrice pCar = new CarPrice(newCar);
			CarMileage mCar = new CarMileage(newCar);
			
			priceQueue.add(pCar);
			mileageQueue.add(mCar);
			count++;
		}
	}//add
	
	/*
	 * gets car from priceQueue
	 * if car is not in one of the lists, throws exception
	 */
	public Car get(String vin) throws NoSuchElementException {
		if(priceQueue.contains(vin) == false || mileageQueue.contains(vin) == false) {
			throw new NoSuchElementException("no car of that VIN exists");
		}
		else {
			Car car = priceQueue.get(vin);
			return car;
		}
	}//get
	
	/*
	 * updates price in both queues
	 * if car is not in one of the lists, throws exception
	 */
	public void updatePrice(String vin, int newPrice) throws NoSuchElementException {
		if(priceQueue.contains(vin) == false || mileageQueue.contains(vin) == false) {
			throw new NoSuchElementException("no car of that VIN exists");
		}
		else {
			Car car = priceQueue.get(vin);
			car.setPrice(newPrice);
			CarPrice pCar = new CarPrice(car);
			CarMileage mCar = new CarMileage(car);
			
			priceQueue.update(vin, pCar);
			mileageQueue.update(vin, mCar);
		}
	}//updatePrice
	
	/*
	 * updates mileage in both queues
	 * if car is not in one of the lists, throws exception
	 */
	public void updateMileage(String vin, int newMileage) throws NoSuchElementException {
		if(priceQueue.contains(vin) == false || mileageQueue.contains(vin) == false) {
			throw new NoSuchElementException("no car of that VIN exists");
		}
		else {
			Car car = priceQueue.get(vin);
			car.setMileage(newMileage);
			CarPrice pCar = new CarPrice(car);
			CarMileage mCar = new CarMileage(car);
			
			priceQueue.update(vin, pCar);
			mileageQueue.update(vin, mCar);
		}
	}//updatePrice
	
	/*
	 * updates color in both queues
	 * if car is not in one of the lists, throws exception
	 */
	public void updateColor(String vin, String newColor) throws NoSuchElementException {
		if(priceQueue.contains(vin) == false || mileageQueue.contains(vin) == false) {
			throw new NoSuchElementException("no car of that VIN exists");
		}
		else {
			Car car = priceQueue.get(vin);
			car.setColor(newColor);

			CarPrice pCar = new CarPrice(car);
			CarMileage mCar = new CarMileage(car);

			priceQueue.update(vin, pCar);
			mileageQueue.update(vin, mCar);
		}
	}//updatePrice
	
	/*
	 * removes car with the given vin from both queues
	 * updates count
	 */
	public void remove(String vin) throws NoSuchElementException {
		if(priceQueue.contains(vin) == false || mileageQueue.contains(vin) == false) {
			throw new NoSuchElementException("no car of that VIN exists");
		}
		else {
			priceQueue.remove(vin);
			mileageQueue.remove(vin);
			count--;
		}
	}//remove
	
	/*
	 * returns lowest value in price queue
	 * if empty, returns null
	 */
	public Car getLowPrice() {
		Car car = priceQueue.get_min();
		return car;
	}//getLowPrice
	
	/*
	 * returns lowest value in mileage queue
	 * if empty, returns null
	 */
	public Car getLowMileage() {
		Car car = mileageQueue.get_min();
		return car;
	}//getLowPrice
	
	/*
	 * shows count to user
	 */
	public int getCount() {
		return count;
	}//getCount
	
	public String toString() {
		String str = "Price Queue: " + priceQueue.toString() + "\n";
		str = str + "Mileage Queue: " + mileageQueue.toString() + "\n";
		str = str + "Total Elements: " + count;
		
		return str;
	}//toString
}//QueuePair
