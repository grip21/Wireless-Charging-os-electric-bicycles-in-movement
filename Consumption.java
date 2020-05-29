package Helder;
// Class used to represent bikes travel and its costs. It holds the nr of coils from which the bike has received energy on the travel,
// the bikes UID and the timestamp of the reception of the last consumption packet
public class Consumption {

    protected int nrCoils =1;
    protected String uid;
    protected long time;

    protected Consumption(int nrCoils, String uid, long time){
        this.nrCoils = nrCoils;
        this.uid = uid;
        this.time = time;
    }

    public boolean checkID(String uid) {

        if (uid.equals(uid)) {
            return true;
        } else {
            return false;
        }
    }

    public  int getNrCoils() {
        return nrCoils;
    }

    public void setNrCoils(int nrCoils) {
        this.nrCoils = nrCoils;
    }

    public String getUid() {
        return this.uid;
    }

    public void setUid(String uid) {
        this.uid = uid;
    }

    public long getTime() {
        return time;
    }

    protected void setTime(long time) {
       this.time = time;
    }

    public  int addCoil(){

        return nrCoils++;
    }

    public String toString(){
        return "UID: "+uid+"  Nrcoils: "+nrCoils;
    }
}

