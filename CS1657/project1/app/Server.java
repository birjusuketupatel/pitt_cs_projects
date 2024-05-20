import java.io.*;
import java.net.*;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.Random;

public class Server {
  private DatagramSocket socket;
  private int port;
  public static final int PKTSIZE = 1024;
  public static final int MAXMSG = 512;
  public static final int SKEW = 7500000; // clock skew in nanoseconds
  public static final double AVG_LAT = 3000000.0; // average latency in nanoseconds
  public static final double STDDEV_LAT = 1000000.0;
  public static final long FLOOR_LAT = 500000;
  public static final boolean LATENCY = true;

  public static final String n = "1658f942ccdfa8f3";
  private static final String x = "14d2be72";

  public Server(int portNum) {
    // setup socket to receive requests
    try {
      port = portNum;
      socket = new DatagramSocket(port);
    } catch (SocketException e) {
      System.out.println("Error: Could not create socket on port " + port);
      System.exit(-1);
    }
  } //Server

  public static int extractMsgLen(byte[] receiveBytes) {
    int high = receiveBytes[2] >= 0 ? receiveBytes[2] : 256 + receiveBytes[2];
    int low = receiveBytes[1] >= 0 ? receiveBytes[1] : 256 + receiveBytes[1];
    return low | (high << 8);
  } //extractMsgLen

  public static void loadMsgLen(byte[] sendBytes, int len) {
    sendBytes[1] = (byte)(len & 0xFF);
    sendBytes[2] = (byte)((len >> 8) & 0xFF);
  } //loadMsgLen

  public static void extractMsg(byte[] receiveBytes, byte[] msgAsBytes, int offset, int len) {
    for(int i = 0; i < Math.min(len, MAXMSG); i++){
      msgAsBytes[i] = receiveBytes[i + offset];
    }
  } //extractMsg

  public static void loadMsg(byte[] sendBytes, byte[] responseAsBytes, int offset, int len) {
    for(int i = 0; i < Math.min(len, MAXMSG); i++){
      sendBytes[i + offset] = responseAsBytes[i];
    }
  } //extractMsg

  public static void loadTimestamp(byte[] sendBytes, byte[] timeAsBytes, int offset) {
    for(int i = 0; i < timeAsBytes.length; i++) {
      sendBytes[offset + 7 - i] = timeAsBytes[timeAsBytes.length - 1 - i];
    }
  } //loadTimestamp

  public static HeftyInteger hexStringToHefty(String hex) {
    return new HeftyInteger((new BigInteger(hex, 16)).toByteArray());
  } //asHeftyInteger

  public static String heftyToHexString(HeftyInteger target) {
    return (new BigInteger(target.getVal())).toString(16);
  } //heftyToHexString

  public static HeftyInteger modPow(HeftyInteger y, HeftyInteger x, HeftyInteger n) throws IllegalArgumentException {
    byte[] zero = {(byte) 0}, one = {(byte) 1};
    HeftyInteger Zero = new HeftyInteger(zero), One = new HeftyInteger(one);

    // run naive modular exponentiation algorithm to calculate y^x mod n
    HeftyInteger result = One;
    y = y.modulo(n);
    while(!x.equals(Zero)) {
      if(x.getBit(0)){
        result = result.multiply(y).modulo(n);
      }
      x = x.rightShift(1);
      y = y.multiply(y).modulo(n);
    }

    return result.prune();
  } //modPow

  private void run() throws Exception {
    InetAddress clientIP;
    int clientPort, reqType, msgLen, high, low;
    long unixMillis, unixNanos;
    long waitStart, waitDelay;
    DatagramPacket receivePkt, sendPkt;
    String msg = "", responseMsg = "";
    HeftyInteger yInt, nInt, xInt, result;
    Random r = new Random();

    // main control loop
    while(true) {
      byte[] receiveBytes = new byte[PKTSIZE];
      byte[] sendBytes = new byte[PKTSIZE];
      byte[] msgAsBytes = new byte[PKTSIZE];
      byte[] responseMsgAsBytes;
      receivePkt = new DatagramPacket(receiveBytes, receiveBytes.length);

      // wait for client request
      socket.receive(receivePkt);

      if(LATENCY) {
        // simulate latency by busy waiting
        waitStart = System.nanoTime();
        waitDelay = (long) (r.nextGaussian() * STDDEV_LAT + AVG_LAT);
        if(waitDelay < FLOOR_LAT) {
          waitDelay = FLOOR_LAT;
        }
        while(System.nanoTime() - waitStart < waitDelay);
      }

      // unpack request
      clientIP = receivePkt.getAddress();
      clientPort = receivePkt.getPort();
      receiveBytes = receivePkt.getData();
      reqType = receiveBytes[0];

      if(reqType == 0) {
        // ping request received
        sendBytes[0] = 0;
        loadMsgLen(sendBytes, 0);
        unixMillis = System.currentTimeMillis() + (SKEW / 1000000);
        unixNanos = System.nanoTime() + SKEW;
        loadTimestamp(sendBytes, BigInteger.valueOf(unixMillis).toByteArray(), 3);
        loadTimestamp(sendBytes, BigInteger.valueOf(unixNanos).toByteArray(), 11);

        sendPkt = new DatagramPacket(sendBytes, sendBytes.length, clientIP, clientPort);

        if(LATENCY) {
          // simulate latency by busy waiting
          waitStart = System.nanoTime();
          waitDelay = (long) (r.nextGaussian() * STDDEV_LAT + AVG_LAT);
          if(waitDelay < FLOOR_LAT) {
            waitDelay = FLOOR_LAT;
          }
          while(System.nanoTime() - waitStart < waitDelay);
        }

        socket.send(sendPkt);
      }
      else if(reqType == 1) {
        // decrypt request received
        msgLen = extractMsgLen(receiveBytes);
        extractMsg(receiveBytes, msgAsBytes, 3, msgLen);
        msg = new String(msgAsBytes, StandardCharsets.UTF_8).substring(0, msgLen);

        // calculating result
        yInt = hexStringToHefty(msg);
        nInt = hexStringToHefty(n);
        xInt = hexStringToHefty(x);

        result = modPow(yInt, xInt, nInt);

        // sending response
        responseMsg = heftyToHexString(result);
        responseMsgAsBytes = responseMsg.getBytes();

        loadMsg(sendBytes, responseMsgAsBytes, 19, responseMsgAsBytes.length);
        loadMsgLen(sendBytes, responseMsgAsBytes.length);
        sendBytes[0] = 1;
        unixMillis = System.currentTimeMillis() + (SKEW / 1000000);
        unixNanos = System.nanoTime() + SKEW;
        loadTimestamp(sendBytes, BigInteger.valueOf(unixMillis).toByteArray(), 3);
        loadTimestamp(sendBytes, BigInteger.valueOf(unixNanos).toByteArray(), 11);

        sendPkt = new DatagramPacket(sendBytes, sendBytes.length, clientIP, clientPort);

        if(LATENCY) {
          // simulate latency by busy waiting
          waitStart = System.nanoTime();
          waitDelay = (long) (r.nextGaussian() * STDDEV_LAT + AVG_LAT);
          if(waitDelay < FLOOR_LAT) {
            waitDelay = FLOOR_LAT;
          }
          while(System.nanoTime() - waitStart < waitDelay);
        }

        socket.send(sendPkt);
      }
      else {
        // malformed request received
        System.out.println("Error: received malformed request from " + clientIP.toString() + ":" + clientPort);
      }
    }
  } //run

  public static void main(String args[]) {
    int portNum;

    if(args.length != 1) {
      System.err.println("Error: Argument required - port");
      System.exit(-1);
    }

    portNum = Integer.parseInt(args[0]);

    // start server at given port
    Server server = new Server(portNum);
    try {
      server.run();
    } catch (Exception e) {
      e.printStackTrace(System.out);
      System.exit(-1);
    }
  } //main
} //Server
