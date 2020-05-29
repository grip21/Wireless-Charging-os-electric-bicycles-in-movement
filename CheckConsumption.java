package Helder;

import java.sql.SQLException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

public class CheckConsumption implements Runnable {

    static HashMap<String,Float> update = new HashMap<>();

    @Override
    public void run() {
        try {
            while (true) {
                synchronized (ListenSocket.consumption) {
                    // Every 15 seconds, the server iterates through all the bikes which have consumption process open
                    long t = System.currentTimeMillis(); // timestamp of the consumption package arrival

                    Iterator<Consumption> iterator = ListenSocket.consumption.iterator();
                    while (iterator.hasNext()) {
                        Consumption c = iterator.next();
                        if ((t- c.getTime()) > 15000) { // if the bikes last consumption packet arrived more than 15 seconds, its considered that it abandoned the track
                            // and must see her UID and nr of coils from which has received energy saved on 'update' Hashmap
                            float cost = (float) (c.getNrCoils()*1); // Calculation of the cost-> 1 coil = 1 cent (Its very expensive). The conversion is only representative, doesn't have any relevance.
                            update.put(c.getUid(),cost);
                            System.out.println("Consumo da bicicleta "+c.getUid()+ "->  Valor: "+cost);
                            iterator.remove(); // remove from the consumption List, the bikes process
                        }
                    }
                    if(update.size()>0){
                        LocalDB.updateConsumption(update); // Call the query to do the consumption update on the database
                    }
                    update.clear();
                    Thread.sleep(15000);
                }
            }
        } catch (InterruptedException e) {
            System.out.println("Erro: "+e);
        }
    }
}