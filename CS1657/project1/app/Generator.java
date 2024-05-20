import java.math.*;
import java.util.*;

public class Generator {
  public static final int Y_SIZE = 8;
  public static final int X_SIZE = 4;

	public static void main(String[] args) {
		Random r = new Random();

		HeftyInteger yInt = generateHefty(Y_SIZE, r);
    HeftyInteger xInt = generateHefty(X_SIZE, r);
    HeftyInteger nInt = generateHefty(Y_SIZE, r);


    long start, finish, diff;
    start = System.nanoTime();

    HeftyInteger result = Server.modPow(yInt, xInt, nInt);

    finish = System.nanoTime();
    diff = finish - start;

    System.out.println("HeftyInteger:");
    System.out.println("size(y), size(n) = " + Y_SIZE * 8);
    System.out.println("size(x) = " + X_SIZE * 8);
    System.out.println("y: " + Server.heftyToHexString(yInt));
    System.out.println("x: " + Server.heftyToHexString(xInt));
    System.out.println("n: " + Server.heftyToHexString(nInt));
    System.out.println("y^x mod n: " + Server.heftyToHexString(result));
    System.out.println("runtime: " + diff);
	} //main

  public static HeftyInteger generateHefty(int size, Random r) {
    // generates a random positive HeftyInteger of the given length
    byte[] byteArr = new byte[size];
    for(int i = 0; i < size; i++) {
      byteArr[i] = (byte) (r.nextInt(256) - 127);
    }

    if(byteArr[0] < 0) {
      byteArr[0] = 0;
    }

    return new HeftyInteger(byteArr);
  } //generateHefty
} //Generator
