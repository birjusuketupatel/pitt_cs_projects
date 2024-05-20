package assignment3;

public class Line {
	
	private int x_initial, y_initial;
	private int x_final, y_final;
	
	public Line() {
		x_initial = 0;
		y_initial = 0;
		
		x_final = 0;
		y_final = 0;
	}//Line
	
	public Line(int x_start, int y_start, int x_end, int y_end) {
		x_initial = x_start;
		y_initial = y_start;
		
		x_final = x_end;
		y_final = y_end;
	}//Line
	
	public void changeInitial(int x, int y) {
		x_initial = x;
		y_initial = y;
	}//editInitial
	
	public void changeFinal(int x, int y) {
		x_final = x;
		y_final = y;
	}//changeFinal
	
	public double getDistance() {
		return Math.sqrt(Math.pow(x_final - x_initial, 2) + Math.pow(y_final - y_initial, 2));
	}//getDistance
	
	public int xFinal() {
		return x_final;
	}//xFinal
	
	public int yFinal() {
		return y_final;
	}//yFinal
	
	public int xInitial() {
		return x_initial;
	}//xInitial
	
	public int yInitial() {
		return y_initial;
	}//yInitial
	
	public String toString() {
		return "[(" + y_initial + ", " + x_initial + "), (" + y_final + ", " + x_final + ")]";
	}
}//Line