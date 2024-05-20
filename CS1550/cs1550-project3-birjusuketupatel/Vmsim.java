import java.util.ArrayList;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;

public class Vmsim{
  public static void main(String[] args){
    String evictionAlgo = "", traceFile = "";
    int numFrames = -1, pageSize = -1;
    ArrayList<Integer> frameDistribution = new ArrayList<Integer>();

    //reads command line arguments
    String nextVal, attribute;
    for(int i = 0; i < args.length - 1; i++){
      if(args[i].charAt(0) == '-'){
        nextVal = args[i+1];
        attribute = args[i].substring(1);

        switch(attribute){
          case "a":
            evictionAlgo = nextVal;
            break;
          case "n":
            numFrames = Integer.parseInt(nextVal);
            break;
          case "p":
            pageSize = Integer.parseInt(nextVal);
            break;
          case "s":
            String[] vals = nextVal.split(":");
            int sum = 0, proportion;
            for(int j = 0; j < vals.length; j++){
              proportion = Integer.parseInt(vals[j]);
              frameDistribution.add(proportion);
              sum += proportion;
            }

            //calculates number of frames each process has given the proportions specified
            for(int j = 0; j < frameDistribution.size(); j++){
                proportion = frameDistribution.get(j);
                frameDistribution.set(j, (int) (numFrames * ((double) proportion / sum)));
            }
        }
      }
    }

    traceFile = args[args.length - 1];


    //initialize caches for each process
    ArrayList<Cache> caches = new ArrayList<Cache>();
    Cache nextCache;
    for(int i = 0; i < frameDistribution.size(); i++){
      nextCache = new Cache(evictionAlgo, frameDistribution.get(i), pageSize);
      caches.add(nextCache);
    }

    //if opt algorithm, reads file twice
    //once to compute access times for each frame, once to run algorithm
    if(evictionAlgo.equals("opt")){
      try{
        File trace = new File(traceFile);
        Scanner reader = new Scanner(trace);
        long time = 0;
        while(reader.hasNextLine()){
          String entry = reader.nextLine();
          String[] data = entry.split(" ");

          String accessType = data[0];
          long addr = Long.parseLong(data[1].substring(2), 16);
          int procId = Integer.parseInt(data[2]);

          caches.get(procId).initAccessTime(time, addr);
          time++;
        }
        reader.close();
      }
      catch(Exception e){
        e.printStackTrace();
      }
    }

    //run simulation on data from the given file
    try{
      File trace = new File(traceFile);
      Scanner reader = new Scanner(trace);
      while(reader.hasNextLine()){
        String entry = reader.nextLine();
        String[] data = entry.split(" ");

        String accessType = data[0];
        long addr = Long.parseLong(data[1].substring(2), 16);
        int procId = Integer.parseInt(data[2]);

        caches.get(procId).memAccess(accessType, addr);
      }
      reader.close();
    }
    catch(FileNotFoundException e){
      e.printStackTrace();
    }

    //collect and print summary statistics
    long totMemAccesses = 0, totFaults = 0, totWrites = 0;
    for(int i = 0; i < caches.size(); i++){
      nextCache = caches.get(i);
      totMemAccesses += nextCache.memAccesses;
      totFaults += nextCache.pageFaults;
      totWrites += nextCache.diskWrites;
    }

    if(evictionAlgo.equals("lru")){
      System.out.println("Algorithm: LRU");
    }
    else if(evictionAlgo.equals("opt")){
      System.out.println("Algorithm: OPT");
    }
    System.out.println("Number of frames: " + numFrames);
    System.out.println("Page size: " + pageSize + " KB");
    System.out.println("Total memory accesses: " + totMemAccesses);
    System.out.println("Total page faults: " + totFaults);
    System.out.println("Total writes to disk: " + totWrites);
  }//main
}//Vmsim
