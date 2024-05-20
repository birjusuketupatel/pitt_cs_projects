/*
 * must implement this interface to work with generic IndirectedPQ implementation
 * Value should contain the key value pair
 */

package cs1501_p3;

public interface KeyVal_Inter<Key, Value> {
	
	/*
	 * returns a deep copy of itself
	 */
	public Value deepCopy();
	
	/*
	 * values must be comparable
	 */
	public int compareTo(Value val);
	
	/*
	 * each value must have a specific key
	 */
	public Key getKey();
	
	/*
	 * each key must be associated with a value
	 */
	public Value getValue();
	
	/*
	 * that value must be modifiable
	 */
	public void setValue(Value newVal);
	
}//KeyVal_Inter
