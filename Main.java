package Helder;


import java.io.IOException;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.HashSet;
import java.util.Set;

public class Main {

    public static DatagramSocket socket;
    private static LocalDB db = new LocalDB();


    public static void main(String[] args) throws IOException {

        socket = new DatagramSocket(11111);
        System.out.println("**SERVER Running**");
        new Thread(new ListenSocket()).start();
        new Thread(new GreenList()).start();
        new Thread(new CheckConsumption()).start();
    }
}