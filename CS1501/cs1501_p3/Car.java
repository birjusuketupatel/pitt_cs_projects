package cs1501_p3;

public class Car implements Car_Inter {
	private String vin;
	private String make;
	private String model;
	private int price;
	private int mileage;
	private String color;
	
	public Car(String Vin, String Make, String Model, int Price, int Mileage, String Color) throws IllegalArgumentException {
		if(Vin.contains(":") || Make.contains(":") || Model.contains(":") || Color.contains(":")) {
			throw new IllegalStateException("illegal character (:) in input");
		}
		
		vin = Vin;
		make = Make;
		model = Model;
		price = Price;
		mileage = Mileage;
		color = Color;
	}//Car
	
	public Car(Car myCar) {
		vin = new String(myCar.getVIN());
		make = myCar.getMake();
		model = myCar.getModel();
		price = myCar.getPrice();
		mileage = myCar.getMileage();
		color = myCar.getColor();
	}//Car
	
	public Car() {
		vin = "";
		make = "";
		model = "";
		price = 0;
		mileage = 0;
		color = "";
	}//Car
	
	public String getVIN() {
		return vin;
	}//getVin
	
	public String getMake() {
		return make;
	}//getMake
	
	public String getModel() {
		return model;
	}//getModel
	
	public int getPrice() {
		return price;
	}//getPrice
	
	public int getMileage() {
		return mileage;
	}//getMileage
	
	public String getColor() {
		return color;
	}//getColor
	
	public void setPrice(int newPrice) {
		price = newPrice;
		return;
	}//setPrice
	
	public void setMileage(int newMileage) {
		mileage = newMileage;
		return;
	}//setMileage
	
	public void setColor(String newColor) throws IllegalArgumentException{
		if(newColor.contains(":")) {
			throw new IllegalArgumentException("illegal character (:) in input");
		}
		color = newColor;
	}//setColor
	
	public String toString() {
		return vin + ":" + make + ":" + model + ":" + price + ":" + mileage + ":" + color;
	}//toString
}//Car
