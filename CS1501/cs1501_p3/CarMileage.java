/*
 * implements key value pair, key = String, value = CarMileage
 */

package cs1501_p3;

public class CarMileage extends Car implements KeyVal_Inter<String,CarMileage> {
	
	public CarMileage(Car myCar) {
		super(myCar);
	}//CarMileage

	public CarMileage deepCopy() {
		Car copyCar = new Car(this.getVIN(), this.getMake(), this.getModel(), this.getPrice(), this.getMileage(), this.getColor());
		CarMileage copy = new CarMileage(copyCar);
		return copy;
	}//deepCopy
	
	/*
	 * if mileage(car) < mileage(comp), return -1
	 * if mileage(car) = mileage(comp), return 0
	 * if mileage(car) > mileage(comp), return 1 
	 */
	public int compareTo(CarMileage comp) {
		if(this.getMileage() < comp.getMileage()) {
			return -1;
		}
		else if(this.getMileage() == comp.getMileage()) {
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
	 * CarMileage is the value
	 */
	public CarMileage getValue() {
		return this;
	}//getValue
	
	/*
	 * modifies color, mileage, and price to reflect that of new car
	 */
	public void setValue(CarMileage newCar) {
		this.setColor(newCar.getColor());
		this.setMileage(newCar.getMileage());
		this.setPrice(newCar.getPrice());
	}//setValue
}//CarMileage