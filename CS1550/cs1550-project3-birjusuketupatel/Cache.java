import java.util.HashMap;
import java.util.HashSet;
import java.util.NoSuchElementException;
import java.util.LinkedList;
import java.util.Iterator;

public class Cache{
  public String algo;
  public int numFrames, pageOffset;
  private final static int KB_OFFSET = 10;
  public long memAccesses, pageFaults, diskWrites;
  private HashMap<Long,Boolean> dirty;
  private LruCache lruCache;
  private OptCache optCache;

  /*
   * Defines specifications for this cache
   * Cache algorithm: lru or opt
   * Number of frames: positive non-zero integer, cache capacity
   * Page Size: positive non-zero integer that is a power of two, page size in kb
   */
  public Cache(String algoName, int frames, int pageSize) throws IllegalArgumentException{
    if(algoName.equals("lru")){
      lruCache = new LruCache();
    }
    else if(algoName.equals("opt")){
      optCache = new OptCache();
    }
    else{
      throw new IllegalArgumentException("Invalid algorithm.");
    }

    algo = algoName;

    if(frames > 0){
      numFrames = frames;
    }
    else{
      throw new IllegalArgumentException("Must have positive number of frames.");
    }

    if(isPowerOfTwo(pageSize)){
      pageOffset = logBase(pageSize, 2) + KB_OFFSET;
    }
    else{
      throw new IllegalArgumentException("Page size must be a positive power of two.");
    }

    memAccesses = 0;
    pageFaults = 0;
    diskWrites = 0;
    dirty = new HashMap<Long,Boolean>();
  }//Cache

  public void memAccess(String instType, long addr) throws IllegalArgumentException{
    if(!instType.equals("l") && !instType.equals("s")){
      throw new IllegalArgumentException("Invalid access type.");
    }

    long frame = addr >>> pageOffset;

    if(algo.equals("lru")){
      lruAccess(instType, frame);
    }
    else if(algo.equals("opt")){
      optAccess(instType, frame);
    }

    memAccesses++;
  }//memAccess

  private void lruAccess(String instType, long frame){
    //cache hit
    if(lruCache.contains(frame)){
      //if modifying frame in the cache, that frame is dirty
      if(instType.equals("s")){
        dirty.put(frame, true);
      }

      //given frame is now least recently used frame
      lruCache.updatePos(frame);

      return;
    }

    //cache miss
    if(lruCache.length < numFrames){
      //cache is below capacity
      //add frame to cache
      lruCache.add(frame);

      //set dirty bit based on type of instruction
      if(instType.equals("l")){
        dirty.put(frame, false);
      }
      else{
        dirty.put(frame, true);
      }

      pageFaults++;
    }
    else{
      //cache is at capacity
      //evict least recently used frame
      long evicted = lruCache.remove();

      //if evicted frame is dirty, disk write occurs
      if(dirty.get(evicted)){
        diskWrites++;
      }

      //remove frame from dirty table
      dirty.remove(evicted);

      //add frame to cache
      lruCache.add(frame);

      //set dirty bit based on type of instruction
      if(instType.equals("l")){
        dirty.put(frame, false);
      }
      else{
        dirty.put(frame, true);
      }

      pageFaults++;
    }
  }//lruAccess

  private void optAccess(String instType, long frame){
    //cache hit
    if(optCache.contains(frame)){
      //if modifying frame in the cache, that frame is dirty
      if(instType.equals("s")){
        dirty.put(frame, true);
      }

      //register cache hit
      optCache.hit(frame);

      return;
    }

    //cache miss
    if(optCache.size() < numFrames){
      //cache is below capacity
      //add frame to cache
      optCache.add(frame);

      //set dirty bit based on type of instruction
      if(instType.equals("l")){
        dirty.put(frame, false);
      }
      else{
        dirty.put(frame, true);
      }

      pageFaults++;
    }
    else{
      //cache is at capacity
      //evict frame that will not be used in the longest time
      long evicted = optCache.evict();

      //if evicted frame is dirty, disk write occurs
      if(dirty.get(evicted)){
        diskWrites++;
      }

      //remove frame from dirty table
      dirty.remove(evicted);

      //add frame to cache
      optCache.add(frame);

      //set dirty bit based on type of instruction
      if(instType.equals("l")){
        dirty.put(frame, false);
      }
      else{
        dirty.put(frame, true);
      }

      pageFaults++;
    }
  }//optAccess

  /*
   * Initializes table of access times for opt algorithm
   * May only be called if algorithm is opt
   */
  public void initAccessTime(long time, long addr) throws Exception {
    if(!algo.equals("opt")){
      throw new Exception("Only callable by an OPT cache.");
    }

    long frame = addr >>> pageOffset;
    optCache.initTable(time, frame);
  }//initAccessTime

  public String toString(){
    String out = "";
    out += "Algorithm: " + algo + "\n";
    out += "Capacity: " + numFrames + "\n";
    out += "Page Offset: " + pageOffset;

    return out;
  }//toString

  /*
   * given integer, determines if it is a power of two
   */
   private boolean isPowerOfTwo(int x){
     return (x != 0) && (x & (x - 1)) == 0;
   }//isPowerOfTwo

   /*
    * calculates log of number for given base
    */
    private int logBase(int num, int base){
      return (int) (Math.log(num) / Math.log(base));
    }//logBase
}//Cache

class Node{
  public long frame;
  public Node next, prev;

  public Node(){
    frame = 0;
    next = null;
    prev = null;
  }//Node

  public Node(long frameNum){
    frame = frameNum;
    next = null;
    prev = null;
  }//Node

  public String toString(){
    return "(" + frame + ")";
  }//toString
}//Node

/*
 * Implements an lru cache using the stack method
 */
class LruCache{
  private HashMap<Long,Node> table;
  private Node head, tail;
  public long length;

  public LruCache(){
    head = new Node();
    tail = new Node();

    head.next = tail;
    tail.prev = head;

    length = 0;
    table = new HashMap<Long,Node>();
  }//LruCache

  /*
   * Adds a frame to the front of the queue
   */
   public void add(long frameNum){
     Node nextNode = new Node(frameNum);

     Node front = head.next;

     nextNode.next = front;
     front.prev = nextNode;
     nextNode.prev = head;
     head.next = nextNode;

     length++;
     table.put(nextNode.frame, nextNode);
   }//add

   /*
    * Removes a frame at the rear of the queue
    */
    public long remove() throws NoSuchElementException{
      Node toRemove = tail.prev;

      //error, tries to remove from an empty list
      if(toRemove == head){
        throw new NoSuchElementException();
      }

      Node prevNode = toRemove.prev;
      prevNode.next = tail;
      tail.prev = prevNode;

      length--;
      table.remove(toRemove.frame);
      return toRemove.frame;
    }//remove

    /*
     * Moves given frame to the front of the queue
     */
     public void updatePos(long frameNum){
         Node toUpdate = table.get(frameNum);

         Node prevNode = toUpdate.prev;
         Node nextNode = toUpdate.next;

         prevNode.next = nextNode;
         nextNode.prev = prevNode;

         Node front = head.next;
         toUpdate.next = front;
         front.prev = toUpdate;
         toUpdate.prev = head;
         head.next = toUpdate;
     }//updatePos

     /*
      * Checks if cache contains a given frame
      */
      public boolean contains(long frameNum){
        return table.containsKey(frameNum);
      }//contains

      public String toString(){
        Node currNode = head.next;
        String out = "";
        while(currNode != tail && currNode != null){
          out += currNode + " ";
          currNode = currNode.next;
        }
        return out;
      }//toString
}//LruCache

/*
 * Implements an OPT cache using the table method discussed in class
 */
class OptCache{
  public HashSet<Long> cacheState;
  public HashMap<Long,LinkedList<Long>> accessTimes;
  public HashMap<Long,Long> lastAccess;
  public long time;

  public OptCache(){
    cacheState = new HashSet<Long>();
    accessTimes = new HashMap<Long,LinkedList<Long>>();
    lastAccess = new HashMap<Long,Long>();
    time = 0;
  }//OptCache

  public long size(){
    return cacheState.size();
  }//size

  /*
   * Determines if the frame is in the cache
   */
  public boolean contains(long frame){
    return cacheState.contains(frame);
  }//contains

  /*
   * On cache hit, remove record of this access in the table
   */
  public void hit(long frame){
    LinkedList<Long> times = accessTimes.get(frame);

    if(times.size() <= 0){
      return;
    }

    //update last access of this frame to current time
    long currTime = times.removeFirst();
    lastAccess.put(frame, currTime);

    return;
  }//hit

  /*
   * Add a new frame to the cache
   */
  public void add(long frame){
    LinkedList<Long> times = accessTimes.get(frame);

    if(times.size() <= 0){
      return;
    }

    //update last access of this frame to current time
    long currTime = times.removeFirst();
    lastAccess.put(frame, currTime);

    //put this frame in the cache
    cacheState.add(frame);
  }//add

  /*
   * Evict the best frame, return the frame number of the evicted frame
   */
   public long evict(){
     long toEvict = -1, currFrame;
     long highestNextUse = -1, lowestLruTime = Long.MAX_VALUE;

     //searches through all items in cache
     //finds item that will be used longest in the future
     //use lru to break tie
     Iterator<Long> iter = cacheState.iterator();
     while(iter.hasNext()){
       currFrame = iter.next();

       //gets time this frame is next used
       LinkedList<Long> times = accessTimes.get(currFrame);
       long nextUse, lruTime;

       if(times.size() <= 0){
         nextUse = Long.MAX_VALUE;
       }
       else{
         nextUse = times.get(0);
       }

       //if this frame is used later than the current highest frame
       //this frame is now the one to evict
       if(nextUse > highestNextUse){
         toEvict = currFrame;
         highestNextUse = nextUse;
         lowestLruTime = lastAccess.get(currFrame);
       }
       else if(nextUse == highestNextUse){
         //ties are broken with the lru algorithm
         lruTime = lastAccess.get(currFrame);

         if(lruTime < lowestLruTime){
           toEvict = currFrame;
           highestNextUse = nextUse;
           lowestLruTime = lruTime;
         }
       }
     }

     //evicts frame that will not be used in the longest time
     cacheState.remove(toEvict);

     return toEvict;
   }//evict

  /*
   * Keeps track of the times that each frame is accessed in a linked list
   */
  public void initTable(long time, long frame){
    LinkedList<Long> times;

    //case 1, table does not contain this frame
    if(!accessTimes.containsKey(frame)){
      //create new list
      times = new LinkedList<Long>();
      times.add(time);
      accessTimes.put(frame, times);

      //keeps track of the last time a given frame was seen for tie breaking purposes
      lastAccess.put(frame, Long.MAX_VALUE - 1);
    }
    else{
      //case 2, table contains this frame
      times = accessTimes.get(frame);
      times.add(time);
      accessTimes.put(frame, times);
    }
  }//initTable
}//OptCache
