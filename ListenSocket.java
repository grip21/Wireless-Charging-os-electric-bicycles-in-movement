package Helder;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.util.*;

public class ListenSocket implements Runnable {

    private float debt = 0.0f;
    private String listType = "green";
    static Set<Consumption> consumption = new HashSet<>();
    protected long time;

    public void run() {

        int port = 11111; // port where the server is listening
        DatagramPacket packet;
        InetAddress IPAddress;
        boolean flagExists = false;
        byte[] buf = new byte[256];

        packet = new DatagramPacket(buf, buf.length);
        int counter =0;
        String split[],id, name; // Packet with the name and the bicycle ID
        String splitBill[], uidBill;
        String splitDelete[], del;
        String splitSet[], s, setDebt = "Fatura paga";
        String splitIP[];
        String splitcheck[], idCheck, accepted = "?", rejected = "/", deleteBicy = "Conta eliminada";
        String successRegistration = "Registado com sucesso", unsuccess = "Erro, UID já existe";

        while (true) {
            try {
                InetAddress iA = InetAddress.getByName("192.168.1.255"); // Broadcast UID

                Main.socket.receive(packet);
                IPAddress = packet.getAddress();
                String received = new String(packet.getData(), 0, packet.getLength());

                //System.out.print("PACOTE recebido:" +received); System.out.println("  IP de origem: "+IPAddress);

                //Authorization Packet
                if (received.startsWith("#")){ // Detection of the packet type character
                    System.out.println("Pacote de autorização");
                    splitcheck = received.split("#"); //Parsing the packet
                    idCheck = splitcheck[1];
                    System.out.println("Pacote: "+counter++);
                    // If the UID that arrives on the packet its on greenList, then its sent the authorization packet and the consumption is accounted.
                    if(GreenList.checkID(idCheck)) {
                        //System.out.println("UID está autorizado");
                        //System.out.println("Fazer broadcast do UID:"+idCheck);
                        String messageGreen = accepted.concat(idCheck);// ?UID is the message sent to the ESP32 modules
                        // Broadcast UID to all the ESP32 which IP address is on the broadcast IP range
                        DatagramPacket confirmPacket = new DatagramPacket(messageGreen.getBytes(), messageGreen.length(), iA, port);
                        Main.socket.send(confirmPacket);

                        time = System.currentTimeMillis();//Timestamp from the moment that the packet arrived at server, only if bicycle is authorized

                        faturacao(idCheck,flagExists);// Check if thie bicycle already has the consumption process initialized.
                    }
                    // Broadcast UID for the black List to all the ESP32 modules which IP address is in the broadcast range
                    else {
                        //System.out.println("UID não está autorizado");
                        //System.out.println("Fazer broadcast do UID:"+idCheck);
                        String messageBlack = rejected.concat(idCheck);// The UID is the message sent to the ESP32 modules
                        DatagramPacket confirmPacket = new DatagramPacket(messageBlack.getBytes(), messageBlack.length(), iA, port);
                        Main.socket.send(confirmPacket);
                    }
                }
                //Consumption Packet
                if(received.startsWith("%")){ // Detection of the packet type character
                    System.out.println("Pacote de contagem do consumo");
                    time = System.currentTimeMillis();//timestamp of the moment that the packege arrived at server, only taken if the bike is valid
                    splitBill = received.split("%"); //Parsing the packet
                    uidBill = splitBill[1];
                    faturacao(uidBill,flagExists);
                }
                //User registration Packet
                if (received.startsWith("&")) { // Detection of the packet type character
                    split = received.split("&|#"); //Parsing the packet
                    name = split[1];
                    id = split[2];
                    System.out.println("ID:"+id);

                    if(LocalDB.insertBicycle(id, name,debt, listType)){
                        DatagramPacket registrationPackage = new DatagramPacket(successRegistration.getBytes(),successRegistration.length(),IPAddress,port);
                        // Warn all the ESP32 modules that a new client is authorized to use the system
                        Main.socket.send(registrationPackage);
                        GreenList.UIDs = LocalDB.getBicicletas();//After registration package, the GreenList must be updated imediately
                    }
                    else{
                        DatagramPacket registrationPackage = new DatagramPacket(unsuccess.getBytes(),unsuccess.length(),IPAddress,port);
                        // Warn all the ESP32 modules that the client is not authorized to use the system
                        Main.socket.send(registrationPackage);
                    }
                }

                //Package to delete user from system
                if (received.startsWith("@")){ // Detection of the packet type character
                    splitDelete = received.split("@"); //Parsing the packet
                    del = splitDelete[1];
                    LocalDB.deleteBicycle(del); // Call the query
                    DatagramPacket deletebicycle = new DatagramPacket(deleteBicy.getBytes(),deleteBicy.length(),IPAddress,port);
                    Main.socket.send(deletebicycle);
                    GreenList.UIDs = LocalDB.getBicicletas();
                }
                //Payment Packet
                if (received.startsWith("!")){ // Detection of the packet type character
                    splitSet = received.split("!"); //Parsing the packet
                    s = splitSet[1];
                    LocalDB.setDebt(s);  // Call the query
                    DatagramPacket updateDebt = new DatagramPacket(setDebt.getBytes(),setDebt.length(),IPAddress,port);
                    Main.socket.send(updateDebt);
                    GreenList.UIDs = LocalDB.getBicicletas();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public void faturacao(String checkid,boolean existe ){

        // if hashset is empty
        if (consumption.size()==0){
            // Create consumption process to the bike
            Consumption e = new Consumption(1, checkid,time);
            consumption.add(e);
            //System.out.println("Criar processo de contagem de consumo para a Bicicleta");
            System.out.println(consumption);
        }
        // If the hashset isn't empty
        else {
            for (Consumption c: consumption) {
                existe = false;
                // Check if the bike has consumption process created and if the bike' UID on the hashset iteration matches
                // with the UID from the packet
                if (c.getUid().equals(checkid)){
                    existe = true;
                    c.addCoil(); c.setTime(time);
                    //System.out.println("Já existe processo de contagem do consumo.");
                    //System.out.println("Adiciona Bobina primária");
                    break;
                }
            }
            System.out.println(consumption);
            if (!existe){
                // If the bike doesn't have consumption process initiated
                //System.out.println("Criar processo de contagem de consumo para a Bicicleta");
                Consumption e = new Consumption(1, checkid,time);
                consumption.add(e);
                System.out.println(consumption);
            }
        }
    }
}