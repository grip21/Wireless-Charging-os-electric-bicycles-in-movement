package Helder;
import com.mysql.cj.x.protobuf.MysqlxDatatypes;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.util.HashSet;
import java.util.Set;

public class GreenList implements Runnable{

    public static HashSet<String> UIDs = new HashSet<>();
    public static Set<String> greenListOLD = new HashSet<>();
    public static Set<String> uidBroadcast = new HashSet<>();
    public static String green ="(";
    public static String black =")";

    public void run(){
        try {
            InetAddress iA = InetAddress.getByName("192.168.1.255");
            int porta = 11111;
            while (true){

                UIDs= LocalDB.getBicicletas(); //Execute the query on DB to search the bicycles which listType = 'green'
                // Compare the UID list with the greenListOLD
                // Conditions:
                //1- If UID exists on both lists, broadcast must not be done
                //2- If UID exists on greenListOLD, and not UIDs HashSet, broadcast must be done passed from Green to Black
                //3- Is UID exists on UIDs HashSet and not on greenListOLD, its to do broadcast, passed from Black to green
                if (greenListOLD.size()>0){
                    for (String uidsTemp : UIDs){
                        if (greenListOLD.contains(uidsTemp)){
                            continue;
                        }// Passed from black to green
                        else { uidBroadcast.add(green.concat(uidsTemp)); }
                    }
                    for (String oldTemp : greenListOLD ){
                        if (UIDs.contains(oldTemp)){
                            continue;
                        }//Passed from green to black
                        else uidBroadcast.add(black.concat(oldTemp));
                    }
                }
                //When greenList its empty
                if ((greenListOLD.size()==0) && (UIDs.size()>0)){
                    for (String uidgreen: UIDs){
                        uidBroadcast.add(green.concat(uidgreen));
                    }
                }
                //System.out.println("Lista de UIDs alterados:"+uidBroadcast);
                greenListOLD.clear();
                greenListOLD.addAll(UIDs);
                //System.out.println("GreenListOLD"+greenListOLD);

                for(String aux1: uidBroadcast){ //Do the broadcast of the new greenList
                    DatagramPacket updateList = new DatagramPacket(aux1.getBytes(), aux1.length(), iA, porta);
                    Main.socket.send(updateList);
                }
                uidBroadcast.clear(); //Dump the list
                Thread.sleep(30000);
            }

        } catch (InterruptedException e) {
            System.out.println("Erro: "+e);
        }catch (IOException e) {
            e.printStackTrace();
        }
    }
    //Check if the UID coming from the bikes is valid. If true, returns true, and in the ListenSocket class the apropriated packet is sent
    public static boolean checkID(String uid){
        if (UIDs.contains(uid)){ return true; }
        else { return false;}
    }
}