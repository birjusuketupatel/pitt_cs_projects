/*
 * hash table implementation from Algorithms, 4th edition
 * Authors: Robert Sedgewick, Kevin Wayne
 * https://algs4.cs.princeton.edu/30searching/
 */

package cs1501_p3;

@SuppressWarnings("unchecked")

public class LinearProbingHashST<Key,Value>
{
   private int N;         // number of key-value pairs in the table
   private int M = 16;    // size of linear-probing table
   private Key[] keys;    // the keys
   private Value[] vals;  // the values

   public LinearProbingHashST()
   {
      keys = (Key[])    new Object[M];
      vals = (Value[]) new Object[M];
   }
   
   public LinearProbingHashST(int cap) {
	   M = cap;
	   keys = (Key[])    new Object[M];
	   vals = (Value[]) new Object[M];
   }

   private int hash(Key key)
   {  return (key.hashCode() & 0x7fffffff) % M; }

   private void resize(int cap)
   {
       LinearProbingHashST<Key, Value> t;
       t = new LinearProbingHashST<Key, Value>(cap);
       for (int i = 0; i < M; i++)
          if (keys[i] != null)
              t.put(keys[i], vals[i]);
       keys = t.keys;
       vals = t.vals;
       M    = t.M;
   }

   public void put(Key key, Value val)
   {
      if (N >= M/2) resize(2*M);  // double M (see text)
      int i;
      for (i = hash(key); keys[i] != null; i = (i + 1) % M)
         if (keys[i].equals(key)) { vals[i] = val; return; }
      keys[i] = key;
      vals[i] = val;
      N++;
   }

   public Value get(Key key)
   {
      for (int i = hash(key); keys[i] != null; i = (i + 1) % M)
         if (keys[i].equals(key))
              return vals[i];
      return null;
   }
   
   public boolean contains(Key key) {
       if (key == null) throw new IllegalArgumentException("argument to contains() is null");
       return get(key) != null;
   }
   
   public void delete(Key key)
   {
      if (!contains(key)) return;
      int i = hash(key);
      while (!key.equals(keys[i]))
         i = (i + 1) % M;
      keys[i] = null;
      vals[i] = null;
      i = (i + 1) % M;
      while (keys[i] != null)
      {
         Key   keyToRedo = keys[i];
         Value valToRedo = vals[i];
         keys[i] = null;
         vals[i] = null;
         N--;
         put(keyToRedo, valToRedo);
         i = (i + 1) % M;
      }
      N--;
      if (N > 0 && N == M/8)
         resize(M/2);
   }
   
   public String toString() {
	   String str = "[";
	   boolean empty = true;
	   
	   for(int i = 0; i < M; i++) {
		   if(keys[i] != null) {
			   str = str + "(" + keys[i] + "," + get(keys[i]) + "), ";
			   empty = false;
		   }
	   }
	   
	   if(empty == false) {
		   str = str.substring(0, str.length() - 2);
	   }
	   str = str + "]";
	   
	   return str;
   }
}