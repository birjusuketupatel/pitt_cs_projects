package menu;

import java.text.SimpleDateFormat;
import java.time.LocalTime;
import java.util.*;
import java.sql.*;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Properties;


public class HW5 {
    public static void main(String[] args) throws SQLException {
        try {
            Class.forName("org.postgresql.Driver");
        }
        catch (ClassNotFoundException e) {
            System.out.println("postgres driver not installed");
            return;
        }

        String url = "jdbc:postgresql://localhost:5432/CS1555_HW5";
        Properties props = new Properties();
        props.setProperty("user", "postgres");
        props.setProperty("password", "");
        Connection conn = DriverManager.getConnection(url, props);

        boolean run = true;
        Scanner in = new Scanner(System.in);
        
        //tells scanner to stop on newline characters
    	//may be incompatible with different versions of java
	//in.useDelimiter("\n");
        in.useDelimiter(System.lineSeparator());

        //creating menu
        while (run) {
            System.out.println();
            System.out.println("To add forest, Enter 1");
            System.out.println("To add worker, Enter 2");
            System.out.println("To add sensor, Enter 3");
            System.out.println("To switch worker duties, Enter 4");
            System.out.println("To update sensor status, Enter 5");
            System.out.println("To update forest covered area, Enter 6");
            System.out.println("To find top k busiest workers, Enter 7");
            System.out.println("To display sensors ranking, Enter 8");
            System.out.println("To exit, Enter 9");

            //valid choices are between 1 and 9
            System.out.println("Enter choice:");
            int choice;
            try { choice = in.nextInt(); } catch (InputMismatchException e) { choice = -1; in.next(); }

            switch (choice) {
                case 1:
                    add_forest(conn, in);
                    break;
                case 2:
                    add_worker(conn, in);
                    break;
                case 3:
                    add_sensor(conn, in);
                    break;
                case 4:
                    switch_duties(conn, in);
                    break;
                case 5:
                    update_sensor_status(conn, in);
                    break;
                case 6:
                    update_coverage(conn, in);
                    break;
                case 7:
                    top_k_workers(conn, in);
                    break;
                case 8:
                    display_sensor_ranking(conn);
                    break;
                case 9:
                    System.out.println("exiting from the menu");
                    run = false;
                    break;
                default:
                    System.out.println("incorrect input, please enter a choice from the menu");
            }
        }

        in.close();
        conn.close();
    }//main

    /*
     * task 6
     * update or add a record to reflect changes to a forests coverage of a state
     */
    private static void update_coverage(Connection conn, Scanner in) {
        String error_msg = "incorrect input type, expected a number";
        double cover_area;
        String state, forest_name;

        System.out.println("give the name of the forest you want to update");
        forest_name = in.next();
        System.out.println("give the state this forest covers");
        state = in.next();
        if(state.length() != 2) {
            System.out.println("expected 2 character state name abbreviation (i.e. PA)");
            return;
        }
        System.out.println("give the new area of this forest that lies in that state");
        try{ cover_area = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        if(cover_area <= 0) {
            System.out.println("forest must have area greater than 0");
            return;
        }

        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;
        try {
            if(!state_exists(conn, state)) {
                System.out.println("invalid state");
                return;
            }

            //get the forest_no given the name
            int forest_no = -1;
            sql = "SELECT forest_no FROM FOREST WHERE name = ?";
            pst = conn.prepareStatement(sql);
            pst.setString(1, forest_name);
            rslt = pst.executeQuery();

            if(!rslt.isBeforeFirst()) {
                System.out.println("no forest named " + forest_name + " exists");
                return;
            }

            while(rslt.next()) {
                forest_no = rslt.getInt("forest_no");
            }

            //check if we are updating an existing record in coverage
            //or adding a new record
            sql = "SELECT * FROM COVERAGE WHERE state = ? and forest_no = ?";
            pst = conn.prepareStatement(sql);
            pst.setString(1, state);
            pst.setInt(2, forest_no);
            rslt = pst.executeQuery();

            if(!rslt.isBeforeFirst()) {
                //add new coverage
                sql = "INSERT INTO COVERAGE (forest_no, state, area) VALUES (?, ?, ?)";
                pst = conn.prepareStatement(sql);
                pst.setInt(1, forest_no);
                pst.setString(2, state);
                pst.setDouble(3, cover_area);
                pst.executeUpdate();
            }
            else {
                //update existing coverage
                sql = "UPDATE COVERAGE "
                        + "SET area = ? "
                        + "WHERE forest_no = ? AND state = ?";
                pst = conn.prepareStatement(sql);
                pst.setDouble(1, cover_area);
                pst.setInt(2, forest_no);
                pst.setString(3, state);
                pst.executeUpdate();
            }
        }
        catch (SQLException e) {
            //display errors
            System.out.println("Error");
            while (e != null) {
                System.out.println("Message = " + e.getMessage());
                e = e.getNextException();
            }
        }
    }//update_coverage

    /*
     * task 8
     * displays all sensors in order of the number of reports they have created
     */
    private static void display_sensor_ranking(Connection conn) {
        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;

        try {
            sql = "SELECT SENSOR.sensor_id AS sensor_id, COUNT(REPORT.sensor_id) AS activity "
                    + "FROM SENSOR "
                    + "JOIN REPORT ON REPORT.sensor_id = SENSOR.sensor_id "
                    + "GROUP BY SENSOR.sensor_id "
                    + "ORDER BY activity DESC";
            pst = conn.prepareStatement(sql);

            rslt = pst.executeQuery();

            int sensor_id;
            int activity;
            System.out.println("(sensor id, activity)");

            if(!rslt.isBeforeFirst()) {
                System.out.println("no sensors found");
            }

            while (rslt.next()) {
                sensor_id = rslt.getInt("sensor_id");
                activity = rslt.getInt("activity");

                System.out.println(sensor_id + ", " + activity);
            }
        }
        catch (SQLException e) {
            //display errors
            System.out.println("Error");
            while (e != null) {
                System.out.println("Message = " + e.getMessage());
                e = e.getNextException();
            }
        }
    }//display_sensor_ranking

    /*
     * task 7
     * asks user for the number of workers they want to see
     * finds the top k busiest workers, where workload is defined
     * as the number of sensors with energy less than 2 that are maintained
     * by this worker
     */
    private static void top_k_workers(Connection conn, Scanner in) {
        String error_msg = "incorrect input type, expected a number";
        int k;

        System.out.println("give the number of workers to be displayed");
        try{ k = in.nextInt(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }

        if(k <= 0) {
            System.out.println("must get a positive, non-zero number of workers");
            return;
        }

        //gets top k busiest workers
        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;
        try {
            sql = "SELECT WORKER.name AS worker, WORKER.ssn AS ssn, COUNT(SENSOR.sensor_id) AS jobs "
                    + "FROM WORKER "
                    + "JOIN SENSOR ON SENSOR.maintainer = WORKER.ssn "
                    + "WHERE SENSOR.energy <= 2 "
                    + "GROUP BY WORKER.ssn "
                    + "ORDER BY jobs DESC "
                    + "LIMIT ?";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, k);
            rslt = pst.executeQuery();

            String name, ssn;
            int jobs;

            System.out.println("(name, ssn, number of jobs)");

            if (!rslt.isBeforeFirst() ) {
                System.out.println("no busy workers found");
            }

            while (rslt.next()) {
                name = rslt.getString("worker");
                ssn = rslt.getString("ssn");
                jobs = rslt.getInt("jobs");

                System.out.println(name + ", " + ssn + ", " + jobs);
            }
        } catch (SQLException e) {
            //display errors
            System.out.println("Error");
            while (e != null) {
                System.out.println("Message = " + e.getMessage());
                e = e.getNextException();
            }
        }
    }//top_k_workers

    /*
     * task 5
     * updates the energy level of sensor and creates report a of temperature reading
     * asks for x and y coordinates to identify sensor and date and time to create report
     */
    private static void update_sensor_status(Connection conn, Scanner in) throws SQLException {
        String date, time, date_time, error_msg = "incorrect input type, expected a number";
        double x, y, temperature;
        int energy;

        System.out.println("give the x coordinate of the sensor");
        try{ x = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give the y coordinate of the sensor");
        try{ y = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give the new energy level of the sensor");
        try{ energy = in.nextInt(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        if(energy < 0 || energy > 100) {
            System.out.println("invalid energy level, valid range is between 0 and 100");
            return;
        }
        System.out.println("give the new temperature reading of the sensor");
        try{ temperature = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give the date this sensor was updated (mm/dd/yyyy)");
        date = in.next();
        if(!valid_date(date)) {
            System.out.println("date does not match the expected format");
            return;
        }
        System.out.println("give the time this sensor was updated (hh24:mi)");
        time = in.next();
        if(!valid_time(time)) {
            System.out.println("time does not match expected format");
        }
        date_time = date + " " + time;

        //insert new sensor
        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;
        try {
            //checks if sensor exists at this coordinate
            sql = "SELECT * FROM SENSOR WHERE x = ? AND y = ?";
            pst = conn.prepareStatement(sql);
            pst.setDouble(1, x);
            pst.setDouble(2, y);
            rslt = pst.executeQuery();

            if(!rslt.next()) {
                System.out.println("no sensor exists at this coordinate");
                return;
            }

            conn.setAutoCommit(false);
            sql = "UPDATE SENSOR "
                    + "SET energy = ?, last_charged = TO_TIMESTAMP(?, 'mm/dd/yyyy hh24:mi'), last_read = TO_TIMESTAMP(?, 'mm/dd/yyyy hh24:mi') "
                    + "WHERE x = ? AND y = ?";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, energy);
            pst.setString(2, date_time);
            pst.setString(3, date_time);
            pst.setDouble(4, x);
            pst.setDouble(5, y);
            pst.executeUpdate();

            //gets id of new sensor
            int sensor_id = -1;
            sql = "SELECT * FROM SENSOR WHERE x = ? AND y = ?";
            pst = conn.prepareStatement(sql);
            pst.setDouble(1, x);
            pst.setDouble(2, y);
            rslt = pst.executeQuery();

            while(rslt.next()) {
                sensor_id = rslt.getInt("sensor_id");
            }

            //display confirmation message
            System.out.println("sensor was updated, sensor id = " + sensor_id);

            //add report of temperature reading
            sql = "INSERT INTO REPORT VALUES "
                    + "(?, TO_TIMESTAMP(?, 'mm/dd/yy hh24:mi'), ?)";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, sensor_id);
            pst.setString(2, date_time);
            pst.setDouble(3, temperature);
            pst.executeUpdate();

            conn.commit();
            conn.setAutoCommit(true);

            //check if emergency occured at this sensor
            sql = "SELECT * FROM EMERGENCY WHERE sensor_id = ? AND report_time = TO_TIMESTAMP(?, 'mm/dd/yy hh24:mi')";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, sensor_id);
            pst.setString(2, date_time);
            rslt = pst.executeQuery();

            if(rslt.next()) {
                System.out.println("ALERT, this sensor has reported an emergency");
            }
        }
        catch (SQLException e) {
            //rollback transaction
            conn.rollback();
            conn.setAutoCommit(true);

            //display errors
            System.out.println("Error");
            while (e != null) {
                System.out.println("Message = " + e.getMessage());
                e = e.getNextException();
            }
        }
    }//update_sensor

    /*
     * task 4
     * asks user for names of two workers
     * sets maintainer of worker 1's sensors to worker 2
     * sets maintainer of worker 2's sensors to worker 1
     */
    private static void switch_duties(Connection conn, Scanner in) throws SQLException {
        //ask the user to supply the two worker's names
        System.out.println("give the name of the first worker");
        String name1 = in.next();
        System.out.println("give the name of the second worker");
        String name2 = in.next();

        //swaps sensor's maintainers
        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;
        try {
            //check if both workers exist in database
            sql = "SELECT * FROM WORKER WHERE name = ?";
            pst = conn.prepareStatement(sql);
            pst.setString(1, name1);
            rslt = pst.executeQuery();

            if(!rslt.next()) {
                System.out.println("worker " + name1 + " not found");
                return;
            }

            sql = "SELECT * FROM WORKER WHERE name = ?";
            pst = conn.prepareStatement(sql);
            pst.setString(1, name2);
            rslt = pst.executeQuery();

            if(!rslt.next()) {
                System.out.println("worker " + name2 + " not found");
                return;
            }

            //do nothing if same name entered twice
            if(name1.equals(name2)) {
                System.out.println("swithed duties of " + name1 + " and " + name2);
                return;
            }

            //start transaction
            conn.setAutoCommit(false);

            //ignore constraints until commit
            sql = "SET CONSTRAINTS ALL DEFERRED";
            pst = conn.prepareStatement(sql);
            pst.executeUpdate();

            //set maintainer of worker 1's sensors to a dummy value
            sql = "UPDATE SENSOR "
                    + "SET maintainer = '-1' "
                    + "WHERE maintainer = (SELECT ssn FROM WORKER WHERE name = ?)";
            pst = conn.prepareStatement(sql);
            pst.setString(1, name1);
            pst.executeUpdate();

            //set maintainer of worker 1's sensor to worker 2
            sql = "UPDATE SENSOR "
                    + "SET maintainer = (SELECT ssn FROM WORKER WHERE name = ?) "
                    + "WHERE maintainer = (SELECT ssn FROM WORKER WHERE name = ?)";
            pst = conn.prepareStatement(sql);
            pst.setString(1, name1);
            pst.setString(2, name2);
            pst.executeUpdate();

            //set maintainer of sensors with dummy value to worker 2
            sql = "UPDATE SENSOR "
                    + "SET maintainer = (SELECT ssn FROM WORKER WHERE name = ?) "
                    + "WHERE maintainer = '-1'";
            pst = conn.prepareStatement(sql);
            pst.setString(1, name2);
            pst.executeUpdate();

            //commit transaction
            conn.commit();
            conn.setAutoCommit(true);

            System.out.println("swithed duties of " + name1 + " and " + name2);
        }
        catch (SQLException e) {
            //rollback transaction
            conn.rollback();
            conn.setAutoCommit(true);

            //display errors
            System.out.println("Error");
            while (e != null) {
                System.out.println("Message = " + e.getMessage());
                e = e.getNextException();
            }
        }
    }//switch_duties

    /*
     * task 3
     * asks user for x and y coordinate, maintainer, and energy level of a sensor
     * also asks for the current date and time and initial temperature reading
     * issues an initial report
     */
    private static void add_sensor(Connection conn, Scanner in) throws SQLException {
        String date, time, date_time, maintainer, error_msg = "incorrect input type, expected a number";
        double x, y, temperature;
        int energy;

        System.out.println("give the x coordinate of the new sensor");
        try{ x = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give the y coordinate of the new sensor");
        try{ y = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give the ssn of the maintainer of this sensor");
        maintainer = in.next();
        if(maintainer.length() != 9) {
            System.out.println("social security number must be 9 characters long");
            return;
        }
        System.out.println("give the initial energy level of the sensor");
        try{ energy = in.nextInt(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        if(energy < 0 || energy > 100) {
            System.out.println("invalid energy level, valid range is between 0 and 100");
            return;
        }
        System.out.println("give the initial temperature reading of the sensor");
        try{ temperature = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give the date this sensor was installed (mm/dd/yyyy)");
        date = in.next();
        if(!valid_date(date)) {
            System.out.println("date does not match the expected format");
            return;
        }
        System.out.println("give the time this sensor was installed (hh24:mi)");
        time = in.next();
        if(!valid_time(time)) {
            System.out.println("time does not match expected format");
        }
        date_time = date + " " + time;
        
        int dummy_id = -1;

        //insert new sensor
        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;
        try {
            //checks if sensor with same coordinates already exists
            sql = "SELECT * FROM SENSOR WHERE x = ? AND y = ?";
            pst = conn.prepareStatement(sql);
            pst.setDouble(1, x);
            pst.setDouble(2, y);
            rslt = pst.executeQuery();

            if(rslt.next()) {
                System.out.println("sensor already exists at this coordinate");
                return;
            }

            conn.setAutoCommit(false);
            sql = "INSERT INTO SENSOR (sensor_id,x, y, last_charged, maintainer, last_read, energy) VALUES "
                    + "(?,?, ?, TO_TIMESTAMP(?, 'mm/dd/yy hh24:mi'), ?, TO_TIMESTAMP(?, 'mm/dd/yy hh24:mi'), ?)";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, dummy_id);
            pst.setDouble(2, x);
            pst.setDouble(3, y);
            pst.setString(4, date_time);
            pst.setString(5, maintainer);
            pst.setString(6, date_time);
            pst.setInt(7, energy);
            pst.executeUpdate();

            //gets id of new sensor
            int sensor_id = -1;
            sql = "SELECT * FROM SENSOR WHERE x = ? AND y = ?";
            pst = conn.prepareStatement(sql);
            pst.setDouble(1, x);
            pst.setDouble(2, y);
            rslt = pst.executeQuery();

            while(rslt.next()) {
                sensor_id = rslt.getInt("sensor_id");
            }

            //display confirmation message
            System.out.println("new sensor was inserted, sensor id = " + sensor_id);

            //add initial report from sensor
            sql = "INSERT INTO REPORT VALUES "
                    + "(?, TO_TIMESTAMP(?, 'mm/dd/yy hh24:mi'), ?)";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, sensor_id);
            pst.setString(2, date_time);
            pst.setDouble(3, temperature);
            pst.executeUpdate();

            conn.commit();
            conn.setAutoCommit(true);
        }
        catch (SQLException e) {
            //rollback transaction
            conn.rollback();
            conn.setAutoCommit(true);

            //display errors
            System.out.println("Error");
            while (e != null) {
                System.out.println("Message = " + e.getMessage());
                e = e.getNextException();
            }
        }
    }//add_sensor

    /*
     * returns whether or not a string is in the correct time format
     */
    private static boolean valid_time(String time) {
        try {
            LocalTime.parse(time);
            return true;
        } catch (Exception e) {
            return false;
        }
    }//valid_time

    /*
     * returns whether or not a string is in the correct date format
     */
    private static boolean valid_date(String date) {
        try {
            SimpleDateFormat df = new SimpleDateFormat("mm/dd/yyyy");
            df.setLenient(false);
            df.parse(date);
            return true;
        }
        catch (Exception e) {
            return false;
        }
    }//valid_date

    /*
     * task 2
     * asks user for ssn, name, rank, and state of a worker
     * inserts new worker into database and confirms insertion
     */
    private static void add_worker(Connection conn, Scanner in) throws SQLException {
        String error_msg = "incorrect input type, expected a number";
        String ssn, name, state;
        int rank;

        System.out.println("give ssn of the worker");
        ssn = in.next();
        if(ssn.length() != 9) {
            System.out.println("social security number must be 9 characters long");
            return;
        }
        System.out.println("give name of the worker");
        name = in.next();
        if(name.length() > 30) {
            System.out.println("must limit name to 30 characters");
            return;
        }
        System.out.println("give rank of the worker");
        try{ rank = in.nextInt(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give employing state of the worker");
        state = in.next();
        if(state.length() != 2) {
            System.out.println("expected 2 character state name abbreviation (i.e. PA)");
            return;
        }

        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;
        try {
            if(!state_exists(conn, state)) {
                System.out.println("invalid state");
                return;
            }

            //start transaction
            conn.setAutoCommit(false);

            System.out.println("adding new worker");

            //insert new worker into db
            sql = "INSERT INTO worker VALUES (?, ?, ?, ?)";
            pst = conn.prepareStatement(sql);
            pst.setString(1, ssn);
            pst.setString(2, name);
            pst.setInt(3, rank);
            pst.setString(4, state);

            pst.executeUpdate();

            sql = "SELECT name FROM WORKER WHERE name = ?";
            pst = conn.prepareStatement(sql);
            pst.setString(1, name);

            rslt = pst.executeQuery();
            while (rslt.next()) {
                String new_worker_name = rslt.getString("name");
                System.out.println("worker was inserted, name = " + new_worker_name);
            }

            //commit transaction
            conn.commit();
            conn.setAutoCommit(true);
        } catch (SQLException e1) {
            //rollback transaction
            conn.rollback();
            conn.setAutoCommit(true);

            //display errors
            System.out.println("Error");
            while (e1 != null) {
                System.out.println("Message = " + e1.getMessage());

                e1 = e1.getNextException();
            }
        }
    }//add_worker

    /*
     * task 1
     * asks user for name, area, acid level, coordinates, and state of a forest
     * inserts forest into database and confirms insertion
     */
    private static void add_forest(Connection conn, Scanner in) throws SQLException {
        String error_msg = "incorrect input type, expected a number";
        String name, state;
        double area, acid_level, mbr_xmin, mbr_xmax, mbr_ymin, mbr_ymax;

        //get forest details and state forest is in from user
        System.out.println("give name of the forest");
        name = in.next();
        if(name.length() > 30) {
            System.out.println("must limit name to 30 characters");
            return;
        }
        System.out.println("give area of the forest");
        try{ area = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        if(area <= 0) {
            System.out.println("forest must have area greater than 0");
            return;
        }
        System.out.println("give acid_level of the forest");
        try{ acid_level = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        if(acid_level < 0 || acid_level > 1) {
            System.out.println("acid level must be between 0 and 1");
            return;
        }
        System.out.println("give mbr_xmin of the forest");
        try{ mbr_xmin = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give mbr_xmax of the forest");
        try{ mbr_xmax = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        if(mbr_xmin >= mbr_xmax) {
        	System.out.println("forest's minimum coordinate must be smaller than it's maximum coordinate");
        	return;
        }
        System.out.println("give mbr_ymin of the forest");
        try{ mbr_ymin = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        System.out.println("give mbr_ymax of the forest");
        try{ mbr_ymax = in.nextDouble(); } catch (InputMismatchException e) { System.out.println(error_msg); in.next(); return; }
        if(mbr_ymin >= mbr_ymax) {
        	System.out.println("forest's minimum coordinate must be smaller than it's maximum coordinate");
        	return;
        }
        System.out.println("give the state of the forest");
        state = in.next();
        int dummy_id = -1;

        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;
        try {
            if(!state_exists(conn, state)) {
                System.out.println("invalid state");
                return;
            }

            //start transaction
            conn.setAutoCommit(false);

            System.out.println("adding the forest");

            //insert forest into FOREST
            sql = "INSERT INTO FOREST (forest_no,name, area, acid_level, mbr_xmin, mbr_xmax, mbr_ymin, mbr_ymax)"
                    + " VALUES (?,?, ?, ?, ?, ?, ?, ?);";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, dummy_id);
            pst.setString(2, name);
            pst.setDouble(3, area);
            pst.setDouble(4, acid_level);
            pst.setDouble(5, mbr_xmin);
            pst.setDouble(6, mbr_xmax);
            pst.setDouble(7, mbr_ymin);
            pst.setDouble(8, mbr_ymax);

            pst.executeUpdate();

            //get the primary key of the new forest
            sql = "SELECT forest_no FROM FOREST WHERE name = ?";
            pst = conn.prepareStatement(sql);
            pst.setString(1, name);
            rslt = pst.executeQuery();

            //confirm insertion
            int forest_no = -1;
            while(rslt.next()) {
                forest_no = rslt.getInt("forest_no");
                System.out.println("forest was inserted, forest_no = " + forest_no);
            }

            if(forest_no == -1) {
                System.out.println("error, failed to insert forest");
            }

            //insert the state this forest is located in into COVERAGE
            sql = "INSERT INTO COVERAGE (forest_no, state, area)"
                    + " VALUES (?, ?, ?)";
            pst = conn.prepareStatement(sql);
            pst.setInt(1, forest_no);
            pst.setString(2, state);
            pst.setDouble(3, area);

            pst.executeUpdate();

            //commit transaction
            conn.commit();
            conn.setAutoCommit(true);
        }
        catch (SQLException e) {
            //rollback transaction
            conn.rollback();
            conn.setAutoCommit(true);

            //display errors
            System.out.println("Error");
            while (e != null) {
                System.out.println("Message = " + e.getMessage());
                e = e.getNextException();
            }
        }
    }//add_forest

    /*
     * returns true if a given state exists in the database
     */
    private static boolean state_exists(Connection conn, String state) throws SQLException {
        PreparedStatement pst = null;
        ResultSet rslt = null;
        String sql;

        //check that state is in the database
        sql = "SELECT * FROM STATE WHERE abbreviation = ?";
        pst = conn.prepareStatement(sql);
        pst.setString(1, state);
        rslt = pst.executeQuery();

        return rslt.next();
    }//state_exists
}//Menu