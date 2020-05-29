package Helder;

import java.util.*;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.PreparedStatement;
import java.sql.ResultSet;

public class LocalDB {
    private static final String SQLuser = "central";
    private static final String SQLpass = "password";
    private static final String SQLPath = "jdbc:mysql://localhost/redeveicular?autoReconnect=true&useSSL=false&serverTimezone=Europe/Lisbon";
    public Statement Statment = null;
    private static Connection connec = null;
    private static PreparedStatement ps = null;
    private boolean checkInsert = false;

    public LocalDB() {
        try {
            Class.forName("com.mysql.cj.jdbc.Driver"); //Driver to connect to database
            connec = DriverManager.getConnection(SQLPath, SQLuser, SQLpass);
            connec.createStatement();
            System.out.println("Connection to Database");
        } catch (SQLException e) {
            System.out.println("SQLException: " + e.getMessage());
            System.out.println("SQLState: " + e.getSQLState());
            System.out.println("VendorError: " + e.getErrorCode());
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    // Query do delete client from fatabase, the client's tag UID is needed, because its the primary key
    public static void deleteBicycle(String idd){
        System.out.println("UID"+ idd);

        try {
            String deleteBicycle = "DELETE FROM bicycle "
                    + "WHERE id = ?"; // Query to delete the client

            ps = connec.prepareStatement(deleteBicycle); // create the statement
            ps.setString(1,idd); // passing the UID to the query
            ps.executeUpdate(); // executing the query

        } catch (Exception e) {
            e.printStackTrace();
        }
        System.out.println("Utilizador eliminado!!");

    }
    // Query do create client from in database, the client's tag UID is needed, and his name also. the float variable is only to put to 0 the debt field in the table. The String
    // 'list' is placed as 'green' by default
    public static boolean insertBicycle(String i, String n, float d, String list) {
        System.out.println("Inserir Bicicleta");
        try {

            String insere = "INSERT INTO bicycle(id,owner,debt,listType) values(?,?,?,?)";
            ps = connec.prepareStatement(insere);
            ps.setString(1,i); // place the UID
            ps.setString(2,n); // place the name
            ps.setFloat(3,d); // place the debt as 0
            ps.setString(4,list); // place the listType as 'green'
            ps.executeUpdate();

        } catch (SQLException e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    // Query to execute the payment of the debt on some client entry.
    public static void setDebt(String uid){ // The client's tag UID is needed in order to accomplish the payment
        String updateDebt= "UPDATE bicycle SET debt = 0, listType ='green' WHERE id = ?"; // Debt goes to 0 and listType, if black, is placed as green

        try {
            ps = connec.prepareStatement(updateDebt);
            ps.setString(1,uid);
            ps.executeUpdate();
        } catch (SQLException e) {
            e.printStackTrace();
        }
        System.out.println("Fatura Paga");
    }
    // Query to the server obtain all the clients that are authorized to use the system
    public static HashSet<String> getBicicletas(){
        String sql = "SELECT id FROM bicycle  where ListType LIKE 'green%'"; // SELECT all the bikes where the listType field is placed as green

        HashSet<String> bicicletas = new HashSet<>();
        Connection conn = null;
        PreparedStatement pstm = null;
        ResultSet result = null;

        try {
            pstm = connec.prepareStatement(sql);
            result = pstm.executeQuery();

            //While there are bikes with ListType field placed as green, they should see their ID added to the structure
            while(result.next()){

                bicicletas.add(result.getString("id"));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }finally{

            try{
                if(result != null){
                    result.close();
                }

                if(pstm != null){
                    pstm.close();
                }
                if(conn != null){
                    conn.close();
                }
            }catch(Exception e){
                e.printStackTrace();
            }
        }
        return bicicletas;
    }

    // Query that executes the update of the debt from each client
    public static String updateConsumption(HashMap<String,Float> update){

        float v=0, sum=0;
        String k = null;
        String res = null;
        String getBicycleUpdate = "SELECT debt from bicycle where id = ?"; // Query to get the bikes debt from the database

        String updateDebt= "UPDATE bicycle " // Query to place the updated debt of the bikes in the database
                + "SET debt = ? "
                + "WHERE id = ?";

        // Before execute the update is necessary to verify if the bike has any previous debt. If it has, then it must be incremented with the new debt
        try {
            for (HashMap.Entry<String, Float> entry : update.entrySet()) { // This structure is given by the server after one or more bikes finish their travel on the track
                // The HashMap contains the UID from the bike as Key, and the number of coils through which it received energy, as Value
                k = entry.getKey();
                v = entry.getValue();
                ps = connec.prepareStatement(getBicycleUpdate);
                ps.setString(1,k);
                //Obtaining all the values from the 'debt' field from all the bikes
                ResultSet aux = ps.executeQuery();

                if (aux.next()){
                    System.out.println("Valor do consumo anterior: "+ aux.getFloat(1));
                    //Add the previous value with the new one
                    sum = aux.getFloat(1) +v;
                    System.out.println("Valor do consumo total: "+ sum);

                    //Store the new value on the database
                    ps = connec.prepareStatement(updateDebt);
                    ps.setFloat(1, sum);
                    ps.setString(2,k);
                    ps.executeUpdate();
                    sum = 0;
                }
                v=0;
                aux.close();
            }
            res = "Consumos atualizados com sucesso";
        }catch (SQLException e) {
            System.out.println("Error: " + e);
        }
        return res;
    }
}




