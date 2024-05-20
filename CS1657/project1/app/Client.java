import java.net.*;
import java.io.*;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class Client {
  public static final String guessX = "be72";
  public static final String wrongGuessX = "3e72";
  public static final int guessBits = 1;
  public static final boolean useWrongGuess = false;
  public static final int EXPERIMENT_TYPE = 1; // measure clock skew -> 0, measure timing variance -> 1
  public static final int SAMPLES = 1000; // total number of experiment measurements
  public static final int NUMPINGS = 250; // number of measurements to calibrate clock skew
  public static final boolean NANO = true; // nanosecond or millisecond measurements

  /*
   * request format:
   * 0      : request type, ping = 0, decrypt = 1
   * 1-2    : length of message (if decrypt)
   * 3-1023 : message content (if decrypt)
   *
   * response format:
   * 0       : type
   * 1-2     : length of message (if decrypt)
   * 3-10    : timestamp in milliseconds
   * 11-18   : timestamp in nanoseconds
   * 19-1023 : message content (if decrypt)
   */
  public static DatagramPacket makeDecryptPkt(String msg, InetAddress ip, int port) {
    byte[] sendBytes = new byte[Server.PKTSIZE];
    byte[] msgAsBytes = msg.getBytes();

    sendBytes[0] = 1;
    sendBytes[1] = (byte)(msg.length() & 0xFF);
    sendBytes[2] = (byte)((msg.length() >> 8) & 0xFF);
    for(int i = 0; i < Math.min(msgAsBytes.length, Server.MAXMSG); i++) {
      sendBytes[i + 3] = msgAsBytes[i];
    }

    return new DatagramPacket(sendBytes, sendBytes.length, ip, port);
  } //makeDecryptPkt

  public static DatagramPacket makePingPkt(InetAddress ip, int port) {
    byte[] sendBytes = new byte[Server.PKTSIZE];
    sendBytes[0] = 0;
    sendBytes[1] = 0;
    sendBytes[2] = 0;

    return new DatagramPacket(sendBytes, sendBytes.length, ip, port);
  } //makeDecryptPkt

  public static long extractTimestamp(byte[] receiveBytes, int offset) {
    byte[] longAsBytes = new byte[8];
    for(int i = 0; i < 8; i++){
      longAsBytes[i] = receiveBytes[i + offset];
    }
    return (new BigInteger(longAsBytes)).longValue();
  } //extractTimestamp

  public static void main(String[] args) throws Exception {
    if(args.length != 1) {
      System.err.println("Error: Argument required - ip:port");
      System.exit(-1);
    }

    // get target IP and port for requests
    String[] parts = args[0].split(":");
    String ip = parts[0];
    int port = Integer.parseInt(parts[1]);

    int choice, respType, high, low, msgLen, sampleCount = 0;

    long unixMillis, unixNanos;
    long sendMillis, sendNanos, receiveMillis, receiveNanos, rttMillis, rttNanos, serverMillis, serverNanos;
    long startMillis, endMillis, startNanos, endNanos, runMillis, runNanos;
    long skewMillis, skewNanos, avgSkewNanos = 0, avgSkewMillis = 0;
    boolean flipped = false;

    String y = "", replyString;
    Random r = new Random();

    InetAddress ipAddress = InetAddress.getByName(ip);
    DatagramSocket clientSocket = new DatagramSocket();
    DatagramPacket sendPacket, receivePacket;

    choice = 0;

    while(sampleCount < SAMPLES || (sampleCount < (SAMPLES + NUMPINGS) && EXPERIMENT_TYPE == 1)) {
      if(EXPERIMENT_TYPE == 1 && sampleCount >= NUMPINGS && !flipped) {
        choice = 1;
        avgSkewNanos /= NUMPINGS;
        avgSkewMillis /= NUMPINGS;
        flipped = true;
      }

      if(choice == 0) {
        sendPacket = makePingPkt(ipAddress, port);
      }
      else {
        y = Server.heftyToHexString(Generator.generateHefty(Generator.Y_SIZE, r));
        sendPacket = makeDecryptPkt(y, ipAddress, port);
      }

      // send request
      sendMillis = System.currentTimeMillis();
      sendNanos = System.nanoTime();
      clientSocket.send(sendPacket);

      // receive response
      byte[] receiveBytes = new byte[Server.PKTSIZE];
      byte[] msgAsBytes = new byte[Server.PKTSIZE];
      receivePacket = new DatagramPacket(receiveBytes, receiveBytes.length);
      clientSocket.receive(receivePacket);

      receiveMillis = System.currentTimeMillis();
      receiveNanos = System.nanoTime();
      rttMillis = receiveMillis - sendMillis;
      rttNanos = receiveNanos - sendNanos;

      receiveBytes = receivePacket.getData();
      respType = receiveBytes[0];

      if(respType == 0){
        // ping response
        unixMillis = extractTimestamp(receiveBytes, 3);
        unixNanos = extractTimestamp(receiveBytes, 11);

        skewMillis = unixMillis - (sendMillis + (rttMillis / 2));
        skewNanos = unixNanos - (sendNanos + (rttNanos / 2));
        avgSkewNanos += skewNanos;
        avgSkewMillis += skewMillis;

        if(EXPERIMENT_TYPE == 0) {
          if(NANO) {
            System.out.println(sampleCount + ", " + skewNanos);
          }
          else {
            System.out.println(sampleCount + ", " + skewMillis);
          }
        }
      }
      else if(respType == 1){
        //decrypt response
        msgLen = Server.extractMsgLen(receiveBytes);
        Server.extractMsg(receiveBytes, msgAsBytes, 19, msgLen);
        unixMillis = extractTimestamp(receiveBytes, 3);
        unixNanos = extractTimestamp(receiveBytes, 11);
        replyString = new String(msgAsBytes, StandardCharsets.UTF_8).substring(0, msgLen);

        // calculates run time of modPow algo on first guessBits on local
        int bitCount = 0;
        byte[] zero = {(byte) 0}, one = {(byte) 1};
        HeftyInteger Zero = new HeftyInteger(zero), One = new HeftyInteger(one);

        HeftyInteger yInt = Server.hexStringToHefty(y);
        HeftyInteger xInt;
        HeftyInteger nInt = Server.hexStringToHefty(Server.n);

        // use either x with bit correctly or incorrectly guessed
        if(useWrongGuess) {
          xInt = Server.hexStringToHefty(wrongGuessX);
        }
        else {
          xInt = Server.hexStringToHefty(guessX);
        }

        startMillis = System.currentTimeMillis();
        startNanos = System.nanoTime();

        HeftyInteger rslt = One;
        yInt = yInt.modulo(nInt);
        while(!xInt.equals(Zero)) {
          if(bitCount >= guessBits){
            break;
          }

          if(xInt.getBit(0)){
            rslt = rslt.multiply(yInt).modulo(nInt);
          }
          xInt = xInt.rightShift(1);
          yInt = yInt.multiply(yInt).modulo(nInt);

          bitCount++;
        }

        endNanos = System.nanoTime();
        endMillis = System.currentTimeMillis();

        runMillis = endMillis - startMillis;
        runNanos = endNanos - startNanos;

        serverMillis = (unixMillis - avgSkewMillis) - sendMillis;
        serverNanos = (unixNanos - avgSkewNanos) - sendNanos;

        if(EXPERIMENT_TYPE == 1) {
          if(NANO) {
            System.out.println((sampleCount - NUMPINGS) + ", " + (serverNanos - runNanos));
          }
          else {
            System.out.println((sampleCount - NUMPINGS) + ", " + (serverMillis - runMillis));
          }
        }
      }
      else{
        //malformed response
        System.out.println("Error: received malformed response from server");
      }

      sampleCount++;
    }
  } //main
} //Client
