public class HeftyInteger {

	private final byte[] ONE = {(byte) 1};

	private byte[] val;

	/**
	 * Construct the HeftyInteger from a given byte array
	 * @param b the byte array that this HeftyInteger should represent
	 */
	public HeftyInteger(byte[] b) {
		val = b;
	}//HeftyInteger

	/**
	 * Return this HeftyInteger's val
	 * @return val
	 */
	public byte[] getVal() {
		return val;
	}//getVal

	/**
	 * Return the number of bytes in val
	 * @return length of the val byte array
	 */
	public int length() {
		return val.length;
	}//length

	/**
	 * Add a new byte as the most significant in this
	 * @param extension the byte to place as most significant
	 */
	public void extend(byte extension) {
		byte[] newv = new byte[val.length + 1];
		newv[0] = extension;
		for (int i = 0; i < val.length; i++) {
			newv[i + 1] = val[i];
		}
		val = newv;
	}//extend

	/**
	 * If this is negative, most significant bit will be 1 meaning most
	 * significant byte will be a negative signed number
	 * @return true if this is negative, false if positive
	 */
	public boolean isNegative() {
		return (val[0] < 0);
	}//isNegative

	/**
	 * Computes the sum of this and other
	 * @param other the other HeftyInteger to sum with this
	 */
	public HeftyInteger add(HeftyInteger other) {
		byte[] a, b;
		// If operands are of different sizes, put larger first ...
		if (val.length < other.length()) {
			a = other.getVal();
			b = val;
		}
		else {
			a = val;
			b = other.getVal();
		}

		// ... and normalize size for convenience
		if (b.length < a.length) {
			int diff = a.length - b.length;

			byte pad = (byte) 0;
			if (b[0] < 0) {
				pad = (byte) 0xFF;
			}

			byte[] newb = new byte[a.length];
			for (int i = 0; i < diff; i++) {
				newb[i] = pad;
			}

			for (int i = 0; i < b.length; i++) {
				newb[i + diff] = b[i];
			}

			b = newb;
		}

		// Actually compute the add
		int carry = 0;
		byte[] res = new byte[a.length];
		for (int i = a.length - 1; i >= 0; i--) {
			// Be sure to bitmask so that cast of negative bytes does not
			//  introduce spurious 1 bits into result of cast
			carry = ((int) a[i] & 0xFF) + ((int) b[i] & 0xFF) + carry;

			// Assign to next byte
			res[i] = (byte) (carry & 0xFF);

			// Carry remainder over to next byte (always want to shift in 0s)
			carry = carry >>> 8;
		}

		HeftyInteger res_li = new HeftyInteger(res);

		// If both operands are positive, magnitude could increase as a result
		//  of addition
		if (!this.isNegative() && !other.isNegative()) {
			// If we have either a leftover carry value or we used the last
			//  bit in the most significant byte, we need to extend the result
			if (res_li.isNegative()) {
				res_li.extend((byte) carry);
			}
		}
		// Magnitude could also increase if both operands are negative
		else if (this.isNegative() && other.isNegative()) {
			if (!res_li.isNegative()) {
				res_li.extend((byte) 0xFF);
			}
		}

		// Note that result will always be the same size as biggest input
		//  (e.g., -127 + 128 will use 2 bytes to store the result value 1)
		return res_li;
	}//add

	/**
	 * Negate val using two's complement representation
	 * @return negation of this
	 */
	public HeftyInteger negate() {
		byte[] neg = new byte[val.length];
		int offset = 0;

		// Check to ensure we can represent negation in same length
		//  (e.g., -128 can be represented in 8 bits using two's
		//  complement, +128 requires 9)
		if (val[0] == (byte) 0x80) { // 0x80 is 10000000
			boolean needs_ex = true;
			for (int i = 1; i < val.length; i++) {
				if (val[i] != (byte) 0) {
					needs_ex = false;
					break;
				}
			}
			// if first byte is 0x80 and all others are 0, must extend
			if (needs_ex) {
				neg = new byte[val.length + 1];
				neg[0] = (byte) 0;
				offset = 1;
			}
		}

		// flip all bits
		for (int i  = 0; i < val.length; i++) {
			neg[i + offset] = (byte) ~val[i];
		}

		HeftyInteger neg_li = new HeftyInteger(neg);

		// add 1 to complete two's complement negation
		return neg_li.add(new HeftyInteger(ONE));
	}//negate

	/**
	 * Implement subtraction as simply negation and addition
	 * @param other HeftyInteger to subtract from this
	 * @return difference of this and other
	 */
	public HeftyInteger subtract(HeftyInteger other) {
		return this.add(other.negate());
	}//subtract

	/**
	 * Compute the product of this and other
	 * @param other HeftyInteger to multiply by this
	 * @return product of this and other
	 */
	public HeftyInteger multiply(HeftyInteger other) {
		HeftyInteger x,y;

		//x set to longer number, y set to shorter number
		if(this.length() > other.length()) {
			x = this;
			y = other;
		}
		else {
			x = other;
			y = this;
		}

		boolean ansNegative = x.isNegative() ^ y.isNegative(); //determines if answer should be negative given signs of inputs

		//both inputs must be positive to run grade school multiplication algorithm
		if(x.isNegative()) {
			x = x.negate();
		}
		if(y.isNegative()) {
			y = y.negate();
		}

		//calculates multiplication using grade school algorithm
		byte[] zero = {(byte) 0};
		HeftyInteger result = new HeftyInteger(zero), temp;
		for(int i = 0; i < 8 * y.length(); i++) {
			//gets the ith bit in y
			//if bit is 1, shifts x by i and adds to running total
			//if bit is 0, does nothing
			boolean currBit = y.getBit(i);

			if(currBit) {
				temp = x.leftShift(i);
				result = result.add(temp);
			}
		}

		//negates result if answer should be negative
		if(ansNegative) {
			result = result.negate();
		}

		return result;
	}//multiply

	/*
	 * divides this by other, returns result as a HeftyInteger
	 * based on division algorithm presented in https://en.wikipedia.org/wiki/Division_algorithm
	 * modified to work with HeftyIntegers
	 */
	public HeftyInteger divide(HeftyInteger divisor) throws IllegalArgumentException {
		//if attempting to divide by 0, throw exception
		byte[] zero = {(byte) 0};
		HeftyInteger Zero = new HeftyInteger(zero);
		if(divisor.equals(Zero)) {
			throw new IllegalArgumentException("cannot divide by 0");
		}

		HeftyInteger dividend = this;
		boolean dividendNegative = dividend.isNegative(), divisorNegative = divisor.isNegative();
		boolean ansNegative = dividendNegative ^ divisorNegative; //determines if answer should be negative given signs of inputs

		//makes both inputs positive
		if(dividend.isNegative()) {
			dividend = dividend.negate();
		}
		if(divisor.isNegative()) {
			divisor = divisor.negate();
		}

		//quotient can at most be the length of the dividend
		//quotient is initially all zeros
		byte[] quotientArr = new byte[dividend.length()];
		for(int i = 0; i < quotientArr.length; i++) {
			quotientArr[i] = (byte) 0;
		}
		HeftyInteger quotient = new HeftyInteger(quotientArr);

		HeftyInteger remainder = new HeftyInteger(zero);

		//implements long division algorithm
		for(int i = 8 * dividend.length() - 1; i >= 0; i--) {
			//set first bit of remainder to ith bit of dividend
			boolean currBit = dividend.getBit(i);
			remainder = remainder.leftShift(1);
			remainder = remainder.setBit(0, currBit);

			//if remainder > divisor, set ith bit of quotient to 1
			//and subtract divisor from remainder
			HeftyInteger difference = remainder.subtract(divisor);
			if(!difference.isNegative()) {
				remainder = difference;
				quotient = quotient.setBit(i, true);
			}
		}

		//prunes unnecessary zeros from the front of the quotient
		quotient = quotient.prune();

		//resets input to original sign
		if(dividendNegative && !dividend.isNegative()) {
			dividend = dividend.negate();
		}
		if(divisorNegative && !divisor.isNegative()) {
			divisor = divisor.negate();
		}

		if(ansNegative) {
			quotient = quotient.negate();
		}

		return quotient;
	}//divide

	/*
	 * implements mod
	 * same functionality as BigInteger mod
	 * cannot mod by a negative number or by 0
	 */
	public HeftyInteger modulo(HeftyInteger other) throws IllegalArgumentException {
		//throws exception if attempting to mod by a negative number or 0
		if(other.isNegative()) {
			throw new IllegalArgumentException("cannot mod by a negative number");
		}

		byte[] zero = {(byte) 0};
		HeftyInteger Zero = new HeftyInteger(zero);
		if(other.equals(Zero)) {
			throw new IllegalArgumentException("cannot mod by 0");
		}

		//a = (a / b) * b + a % b
		HeftyInteger result = this.divide(other);
		result = result.multiply(other);
		result = this.subtract(result);

		//makes negative result positive
		if(result.isNegative()) {
			result = result.add(other);
		}

		result = result.prune();

		return result;
	}//modulo

	/**
	 * Run the extended Euclidean algorithm on this and other
	 * @param other another HeftyInteger
	 * @return an array structured as follows:
	 *   0:  the GCD of this and other
	 *   1:  a valid x value
	 *   2:  a valid y value
	 * such that this * x + other * y == GCD in index 0
	 */
	 public HeftyInteger[] XGCD(HeftyInteger other) throws IllegalArgumentException {
		 //does not accept negative input
		 if(this.isNegative() || other.isNegative()) {
			 throw new IllegalArgumentException("XGCD does not accept negative input");
		 }

		HeftyInteger a, b;
		boolean swapped;

		//sets larger number as a, smaller as b
		if(this.length() < other.length()) {
			a = other;
			b = this;
			swapped = true;
		}
		else {
			a = this;
			b = other;
			swapped = false;
		}

		HeftyInteger[] results = recXGCD(a, b);

		if(swapped) {
			HeftyInteger temp = results[1];
			results[1] = results[2];
			results[2] = temp;
		}

		return results;
	 }//XGCD

	 /*
	  * recursively calculates gcd and x,y values
	  * based on implementation discussed in recitation
	  */
	 private HeftyInteger[] recXGCD(HeftyInteger a, HeftyInteger b) {
		 byte[] zero = {(byte) 0}, one = {(byte) 1};
		 HeftyInteger Zero = new HeftyInteger(zero), One = new HeftyInteger(one);

		 //base case: b = 0, gcd = a
		 //build x and y from bottom up, initially x = 1, y = 0
		 if(b.equals(Zero)) {
			 HeftyInteger[] base = {a, One, Zero};
			 return base;
		 }

		 //recursion case: b > 0
		 //recurse on input gcd(b, b%a)
		 HeftyInteger[] result = recXGCD(b, a.modulo(b));

		//build up result
		HeftyInteger gcd = result[0];
		HeftyInteger x = result[1];
		HeftyInteger y = result[2];

		//new x = prev y
		//new y = prev x - (a / b) * prev y
		HeftyInteger newX = y;
		HeftyInteger newY = a.divide(b);
		newY = newY.multiply(y);
		newY = x.subtract(newY);

		HeftyInteger[] newResult = {gcd, newX, newY};
		return newResult;
	 }//recXGCD

	 /*
	  * returns copy of HeftyInteger that is arithmetically right shifted by given number of bits
	  * same functionality as java >> operator
	  * corresponds to floor division by 2^n
	  */
	 public HeftyInteger rightShift(int bits) throws IllegalArgumentException {
		 if(bits < 0) {
			 throw new IllegalArgumentException("shift must be > 0");
		 }

		 byte[] currValue = this.getVal();
		 byte[] shiftedValue = new byte[currValue.length];

		 //separates shift into a shift of bytes and a shift of bits
		 //i.e. rightShift(23) -> 2 bytes and 7 bits of shifting
		 int bitShift = bits % 8;
		 int byteShift = (bits - bitShift) / 8;

		 byte overflowMask = (byte) ~(-1 << bitShift), byteMask = (byte) 0xFF, keepMask = (byte) ~(-1 << (8 - bitShift));
		 byte overflow = 0, shifted, toShift;

		 //preserve sign when shifting first byte
		 toShift = currValue[0];
		 shifted = (byte) (toShift >> bitShift);

		 overflow = (byte) (toShift & overflowMask);
		 overflow = (byte) ((overflow << (8 - bitShift)) & byteMask);

		 shiftedValue[0] = shifted;

		 for(int i = 1; i < currValue.length; i++) {
			 toShift = currValue[i];

			 shifted = (byte) (toShift >> bitShift);
			 shifted = (byte) (shifted & keepMask);

			 shifted = (byte) (shifted | overflow);

			 overflow = (byte) (toShift & overflowMask);
			 overflow = (byte) ((overflow << (8 - bitShift)) & byteMask);

			 shiftedValue[i] = shifted;
		 }

		 HeftyInteger shiftedInt = new HeftyInteger(shiftedValue);

		 if(byteShift > 0) {
			 shiftedInt = shiftedInt.byteRightShift(byteShift);
		 }

		 return shiftedInt;
	 }//rightShift

	 /*
	  * returns true if two HeftyIntegers are equal
	  * false if they differ
	  */
	 public boolean equals(HeftyInteger other) {
		 HeftyInteger thisInt = this;

		 //removes leading 0s and -1s from both HeftyIntegers
		 thisInt = thisInt.prune();
		 other = other.prune();

		 //if different lengths, not equal
		 if(thisInt.length() != other.length()) {
			 return false;
		 }

		 //checks if every byte is identical between both
		 boolean difference = false;

		 for(int i = 0; i < thisInt.length(); i++) {
			 if(thisInt.getVal()[i] != other.getVal()[i]) {
				 difference = true;
				 break;
			 }
		 }

		 if(difference) {
			 return false;
		 }
		 else {
			 return true;
		 }
	 }//equals

	 /*
	  * prunes leading 0s and -1s from given HeftyInteger
	  */
	 public HeftyInteger prune() {
		 HeftyInteger oldInt = this;
		 boolean negative = oldInt.isNegative();

		 if(oldInt.isNegative()) {
			 oldInt = oldInt.negate();
		 }

		 byte [] oldArr = oldInt.getVal();
		 int cut = 0;

		 while(cut < oldArr.length - 1 && oldArr[cut] == (byte) 0) {
			 if(oldArr[cut+1] == (byte) 0) {
				 cut++;
			 }
			 else {
				 break;
			 }
		 }

		 byte[] newArr;

		 if(cut > 0) {
			newArr = new byte[oldArr.length-cut];
		 	for(int i = cut; i < oldArr.length; i++) {
			 	newArr[i-cut] = oldArr[i];
		 	}
		 }
		 else {
			 newArr = oldArr;
		 }

		 HeftyInteger newInt = new HeftyInteger(newArr);

		 if(negative && !newInt.isNegative()) {
			 newInt = newInt.negate();
		 }

		 return newInt;
	 }//prune

	 /*
	  * returns a copy of HeftyInteger right shifted by a given number of bytes
	  * simply deletes the last n bytes from the array
	  */
	 private HeftyInteger byteRightShift(int bytes) throws IllegalArgumentException {
		 if(bytes < 0) {
			 throw new IllegalArgumentException("shift must be > 0");
		 }

		 byte[] currValue = this.getVal();
		 boolean isNegative = this.isNegative();

		 //if all bytes are shifted off, number becomes either 0 or -1 depending on sign
		 if(bytes >= currValue.length && !isNegative) {
			 byte[] finalValue = {(byte) 0};
			 return new HeftyInteger(finalValue);
		 }
		 else if(bytes >= currValue.length && isNegative) {
			 byte[] finalValue = {(byte) -1};
			 return new HeftyInteger(finalValue);
		 }

		 //chops off the last n least significant bytes
		 byte[] shiftedValue = new byte[currValue.length-bytes];
		 for(int i = 0; i < shiftedValue.length; i++) {
			 shiftedValue[i] = currValue[i];
		 }

		 return new HeftyInteger(shiftedValue);
	 }//byteRightShift

	 /*
	  * returns copy of HeftyInteger left shifted by given number of bits
	  * same functionality as java << operator
	  * corresponds to multiplying by 2^n
	  */
	 public HeftyInteger leftShift(int bits) throws IllegalArgumentException {
		 if(bits < 0) {
			 throw new IllegalArgumentException("shift must be > 0");
		 }

		 byte[] currValue = this.getVal();
		 byte[] shiftedValue = new byte[currValue.length];
		 boolean isNegative = this.isNegative();

		 //separates shift into a shift of bytes and a shift of bits
		 //i.e. leftShift(23) -> 2 bytes and 7 bits of shifting
		 int bitShift = bits % 8;
		 int byteShift = (bits - bitShift) / 8;

		 //shifts current number by between 1-7 bits
		 int byteMask = 0xFF;
		 int overflowMask = ~(-1 << bitShift);

		 int overflow = 0, shifted;
		 for(int i = currValue.length-1; i >= 0; i--) {
			 shifted = currValue[i];
			 shifted <<= bitShift; //shifts current byte
			 shifted |= overflow; //adds overflow from previous shift
			 shiftedValue[i] = (byte) (shifted & byteMask); //adds shifted byte to new number
			 overflow = (shifted >> 8) & overflowMask; //stores the bits pushed beyond the 8 bit limit
		 }

		 HeftyInteger shiftedInt = new HeftyInteger(shiftedValue);

		//adds final overflow
		 if(overflow != (byte) 0) {
			 shiftedInt.extend((byte) overflow);
		 }

		 //special case 1:
		 //positive number becomes negative
		 //extends by 1 byte of zeros to make positive
		 //special case 2:
		 //negative number becomes positive
		 //makes leading zeros of the MSB ones
		 if(!isNegative && shiftedInt.isNegative()) {
			 shiftedInt.extend((byte) 0);
		 }
		 else if(isNegative && !shiftedInt.isNegative()) {
			 int negativeMask = (-1 << bitShift) & byteMask;
			 byte[] makeNegative = shiftedInt.getVal();
			 makeNegative[0] |= negativeMask;
			 shiftedInt = new HeftyInteger(makeNegative);
		 }

		 //shifts new number by implied number of bytes
		 shiftedInt = shiftedInt.byteLeftShift(byteShift);

		 return shiftedInt;
	 }//leftShift

	 /*
	  * returns copy of HeftyInteger left shifted by a given number of bytes
	  */
	 private HeftyInteger byteLeftShift(int bytes) throws IllegalArgumentException {
		 if(bytes < 0) {
			 throw new IllegalArgumentException("shift must be > 0");
		 }

		 byte[] currValue = this.getVal();
		 byte[] shiftedArr = new byte[currValue.length+bytes];

		 for(int i = 0; i < currValue.length; i++) {
			 shiftedArr[i] = currValue[i];
		 }

		 for(int i = currValue.length; i < shiftedArr.length; i++) {
			 shiftedArr[i] = (byte) 0;
		 }

		 return new HeftyInteger(shiftedArr);
	 }//byteLeftShift

	 /*
	  * finds bit at the nth position in the HeftyInteger
	  * least significant bit has index of 0
	  */
	 public boolean getBit(int bitPos) throws IllegalArgumentException {
		 if(bitPos < 0 || bitPos >= 8 * this.length()) {
			 throw new IllegalArgumentException("target bit out of bounds");
		 }

		 byte[] value = this.getVal();
		 int bits = bitPos % 8;
		 int bytes = (bitPos - bits) / 8;
		 byte targetByte = value[value.length-bytes-1];
		 boolean targetBit = ((targetByte >> bits) & 0x1) == 1;
		 return targetBit;
	 }//getBit

	 /*
	  * sets bit at nth position in the HeftyInteger
	  * returns copy of HeftyInteger
	  * least significant bit has index 0
	  */
	 public HeftyInteger setBit(int bitPos, boolean bitVal) throws IllegalArgumentException {
		 if(bitPos < 0 || bitPos >= 8 * this.length()) {
			 throw new IllegalArgumentException("target bit out of bounds");
		 }

		 byte[] value = this.getVal();
		 int bits = bitPos % 8;
		 int bytes = (bitPos - bits) / 8;
		 byte targetByte = value[value.length-bytes-1];
		 byte bitMask = (byte) (0x1 << bits);

		 if(bitVal) {
			targetByte |= bitMask;
		 }
		 if(!bitVal) {
			 targetByte &= ~bitMask;
		 }

		 value[value.length-bytes-1] = targetByte;

		 return new HeftyInteger(value);
	 }//setBit

	 /*
	  * returns integer as a binary string
	  */
	 public String toString() {
		 String out = "";
		 String next;
		 int mask = 1, currByte, currBit;

		 for(int i = 0; i < this.length(); i++) {
			 next = "";
			 currByte = val[i];
			 mask = 1;

			 for(int j = 0; j < 8; j++) {
				 currBit = currByte & mask;
				 currBit >>= j;

				 if(currBit == 1) {
					 next = "1" + next;
				 }
				 else {
					 next = "0" + next;
				 }

				 mask <<= 1;
			 }

			 out = out + " " + next;
		 }

		 out = out.substring(1, out.length());

		 return out;
	 }//toString
}//HeftyInteger
