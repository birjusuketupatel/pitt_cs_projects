/*
 * implements key value pair, key = String, value = CarPrice
 */

package cs1501_p3;

public class CarPrice extends Car implements KeyVal_Inter<String,CarPrice> {
	
	public CarPrice(Car myCar) {
		super(myCar);
	}//CarPrice

	public CarPrice deepCopy() {
		Car copyCar = new Car(this.getVIN(), this.getMake(), this.getModel(), this.getPrice(), this.getMileage(), this.getColor());
		CarPrice copy = new CarPrice(copyCar);
		return copy;
	}//deepCopy
	
	/*
	 * if price(car) < price(comp), return -1
	 * if price(car) = price(comp), return 0
	 * if price(car) > price(comp), return 1 
	 */
	public int compareTo(CarPrice comp) {
		if(this.getPrice() < comp.getPrice()) {
			return -1;
		}
		else if(this.getPrice() == comp.getPrice()) {
			return 0;
		}
		else {
			return 1;
		}
	}//compareTo
	
	/*
	 * Car's vin is the key
	 */
	public String getKey() {
		return this.getVIN();
	}//getKey
	
	/*
	 * CarPrice is the value
	 */
	public CarPrice getValue() {
		return this;
	}//getValue
	
	/*
	 * modifies color, mileage, and price to reflect that of new car
	 */
	public void setValue(CarPrice newCar) {
		this.setColor(newCar.getColor());
		this.setMileage(newCar.getMileage());
		this.setPrice(newCar.getPrice());
	}//setValue
}//CarPrice
