----CS1555/2055 - DATABASE MANAGEMENT SYSTEMS (FALL 2021)
----DEPT. OF COMPUTER SCIENCE, UNIVERSITY OF PITTSBURGH
----ASSIGNMENT #5
----BIRJU PATEL, AATESH ARYA, DB CREATION

----DEFINE DB SCHEMA

DROP TABLE IF EXISTS FOREST CASCADE;
DROP TABLE IF EXISTS STATE CASCADE;
DROP TABLE IF EXISTS COVERAGE CASCADE;
DROP TABLE IF EXISTS ROAD CASCADE;
DROP TABLE IF EXISTS INTERSECTION CASCADE;
DROP TABLE IF EXISTS WORKER CASCADE;
DROP TABLE IF EXISTS SENSOR CASCADE;
DROP TABLE IF EXISTS REPORT CASCADE;
DROP TABLE IF EXISTS EMERGENCY CASCADE;
DROP DOMAIN IF EXISTS energy_dom;

CREATE DOMAIN energy_dom AS integer CHECK (value >= 0 AND value <= 100);

CREATE TABLE FOREST (
    forest_no       SERIAL,
    name            varchar(30) NOT NULL,
    area            real NOT NULL,
    acid_level      real NOT NULL,
    mbr_xmin        real NOT NULL,
    mbr_xmax        real NOT NULL,
    mbr_ymin        real NOT NULL,
    mbr_ymax        real NOT NULL,
    sensor_count    integer NOT NULL DEFAULT 0,
    CONSTRAINT FOREST_PK PRIMARY KEY (forest_no),
    CONSTRAINT FOREST_UN1 UNIQUE (name),
    CONSTRAINT FOREST_UN2 UNIQUE (mbr_xmin, mbr_xmax, mbr_ymin, mbr_ymax),
    CONSTRAINT FOREST_CH CHECK (acid_level >= 0 AND acid_level <= 1)
);


CREATE TABLE STATE (
    name            varchar(30) NOT NULL,
    abbreviation    varchar(2),
    area            real NOT NULL,
    population      integer NOT NULL,
    CONSTRAINT STATE_PK PRIMARY KEY (abbreviation),
    CONSTRAINT STATE_UN UNIQUE (name)
);


CREATE TABLE COVERAGE (
    forest_no       integer,
    state           varchar(2),
    percentage      real,
    area            real NOT NULL,
    CONSTRAINT COVERAGE_PK PRIMARY KEY (forest_no, state),
    CONSTRAINT COVERAGE_FK1 FOREIGN KEY (forest_no) REFERENCES FOREST(forest_no),
    CONSTRAINT COVERAGE_FK2 FOREIGN KEY (state) REFERENCES STATE(abbreviation)
);


CREATE TABLE ROAD (
    road_no varchar(10),
    name    varchar(30) NOT NULL,
    length  real NOT NULL,
    CONSTRAINT ROAD_PK PRIMARY KEY (road_no)
);


CREATE TABLE INTERSECTION (
    forest_no integer,
    road_no   varchar(10),
    CONSTRAINT INTERSECTION_PK PRIMARY KEY (forest_no, road_no),
    CONSTRAINT INTERSECTION_FK1 FOREIGN KEY (forest_no) REFERENCES FOREST(forest_no),
    CONSTRAINT INTERSECTION_FK2 FOREIGN KEY (road_no) REFERENCES ROAD(road_no)
  );


CREATE TABLE WORKER (
    ssn  varchar(9) ,
    name varchar(30) NOT NULL,
    rank integer NOT NULL,
    employing_state varchar(2) NOT NULL,
    CONSTRAINT WORKER_PK PRIMARY KEY (ssn),
    CONSTRAINT WORKER_UN UNIQUE (name),
    CONSTRAINT WORKER_FK FOREIGN KEY (employing_state) REFERENCES STATE(abbreviation)
);


CREATE TABLE SENSOR
  (
    sensor_id SERIAL,
    x real NOT NULL,
    y real NOT NULL,
    last_charged timestamp NOT NULL,
    maintainer   varchar(9) DEFAULT NULL,
    last_read    timestamp NOT NULL,
    energy energy_dom NOT NULL,
    CONSTRAINT SENSOR_PK PRIMARY KEY (sensor_id),
    CONSTRAINT SENSOR_FK FOREIGN KEY (maintainer) REFERENCES WORKER(ssn),
    CONSTRAINT SENSOR_UN UNIQUE (x, y)
);


CREATE TABLE REPORT (
    sensor_id integer,
    report_time timestamp NOT NULL,
    temperature real NOT NULL,
    CONSTRAINT REPORT_PK PRIMARY KEY (sensor_id, report_time),
    CONSTRAINT REPORT_FK FOREIGN KEY (sensor_id) REFERENCES SENSOR(sensor_id)
);


CREATE TABLE EMERGENCY (
	sensor_id integer,
	report_time timestamp NOT NULL,
	CONSTRAINT EMERGENCY_PK
		PRIMARY KEY (sensor_id, report_time),
	CONSTRAINT EMERGENCY_FK
		FOREIGN KEY (sensor_id, report_time)
		REFERENCES REPORT(sensor_id, report_time)
		DEFERRABLE INITIALLY IMMEDIATE
);

BEGIN;
ALTER TABLE FOREST
    DROP CONSTRAINT FOREST_UN1,
    DROP CONSTRAINT FOREST_UN2,
    ADD CONSTRAINT FOREST_UN1 UNIQUE (name) DEFERRABLE INITIALLY DEFERRED,
    ADD CONSTRAINT FOREST_UN2 UNIQUE (mbr_xmin, mbr_xmax, mbr_ymin, mbr_ymax) DEFERRABLE INITIALLY DEFERRED;
COMMIT;

BEGIN;
ALTER TABLE STATE
    DROP CONSTRAINT STATE_UN,
    ADD CONSTRAINT STATE_UN UNIQUE (name) DEFERRABLE INITIALLY DEFERRED;
COMMIT;

BEGIN;
ALTER TABLE COVERAGE
    DROP CONSTRAINT COVERAGE_FK1,
    DROP CONSTRAINT COVERAGE_FK2,
    ADD CONSTRAINT COVERAGE_FK1 FOREIGN KEY (forest_no) REFERENCES FOREST(forest_no) DEFERRABLE INITIALLY IMMEDIATE,
    ADD CONSTRAINT COVERAGE_FK2 FOREIGN KEY (state) REFERENCES STATE(abbreviation) DEFERRABLE INITIALLY IMMEDIATE;
COMMIT;

BEGIN;
ALTER TABLE INTERSECTION
    DROP CONSTRAINT INTERSECTION_FK1,
    DROP CONSTRAINT INTERSECTION_FK2,
    ADD CONSTRAINT INTERSECTION_FK1 FOREIGN KEY (forest_no) REFERENCES FOREST(forest_no) DEFERRABLE INITIALLY IMMEDIATE,
    ADD CONSTRAINT INTERSECTION_FK2 FOREIGN KEY (road_no) REFERENCES ROAD(road_no) DEFERRABLE INITIALLY IMMEDIATE;
COMMIT;

BEGIN;
ALTER TABLE WORKER
    DROP CONSTRAINT WORKER_UN,
    DROP CONSTRAINT WORKER_FK,
    ADD CONSTRAINT WORKER_UN UNIQUE (name) DEFERRABLE INITIALLY DEFERRED,
    ADD CONSTRAINT WORKER_FK FOREIGN KEY (employing_state) REFERENCES STATE(abbreviation) DEFERRABLE INITIALLY IMMEDIATE;
COMMIT;

BEGIN;
ALTER TABLE SENSOR
    DROP CONSTRAINT SENSOR_FK,
    DROP CONSTRAINT SENSOR_UN,
    ADD CONSTRAINT SENSOR_FK FOREIGN KEY (maintainer) REFERENCES WORKER(ssn) DEFERRABLE INITIALLY IMMEDIATE,
    ADD CONSTRAINT SENSOR_UN UNIQUE (x, y) DEFERRABLE INITIALLY DEFERRED;
COMMIT;

BEGIN;
ALTER TABLE REPORT
    DROP CONSTRAINT REPORT_FK,
    ADD CONSTRAINT REPORT_FK FOREIGN KEY (sensor_id) REFERENCES SENSOR(sensor_id) DEFERRABLE INITIALLY IMMEDIATE;
COMMIT;

----IMPLEMENT PROCEDURES AND FUNCTIONS

----GIVEN THE X AND Y COORIDINATES OF A SENSOR,
----FINDS ALL FORESTS IN WHICH THAT SENSOR IS CONTAINES
----AND INCREMENTS THE SENSOR COUNT OF THAT FOREST

CREATE OR REPLACE PROCEDURE incrementSensorCount_proc(
	sensor_x real,
	sensor_y real
)

LANGUAGE plpgsql
AS $$
BEGIN
	UPDATE FOREST
	SET sensor_count = sensor_count + 1
	WHERE
			sensor_x >= FOREST.mbr_xmin AND 
			sensor_x <= FOREST.mbr_xmax AND
			sensor_y >= FOREST.mbr_ymin AND
			sensor_y <= FOREST.mbr_ymax;
END
$$;

----GIVEN THE AREA OF A FOREST IN A PARTICULAR STATE AND THE ID
----OF THAT FOREST, RETURNS THE PERCENT OF THAT FOREST'S AREA
----IN THAT STATE
CREATE OR REPLACE FUNCTION computePercentage(
	forest_id integer,
	area_covered real
)

RETURNS real AS $$
DECLARE
	result real;
	forest_area real;
BEGIN
	SELECT area
	INTO forest_area
	FROM FOREST
	WHERE forest_no = forest_id;

	IF forest_area IS NULL THEN
		RAISE EXCEPTION 'forest_no % not found', forest_id;
	END IF;

	IF forest_area = 0 THEN
		RAISE EXCEPTION 'cannot divide by zero';
	END IF;

	IF area_covered > forest_area THEN
		RAISE EXCEPTION 'state coverage is greater than total forest area';
	END IF;

	result := area_covered / forest_area;

	RETURN result;
END;
$$
	LANGUAGE plpgsql;

----DEFINE TRIGGERS
DROP TRIGGER IF EXISTS sensorCount_tri ON SENSOR;
DROP TRIGGER IF EXISTS percentage_tri ON COVERAGE;
DROP TRIGGER IF EXISTS emergency_tri ON REPORT;
DROP TRIGGER IF EXISTS enforceMaintainer_tri ON SENSOR;

----ON INSERTION OF A SENSOR, UPDATE THE SENSOR COUNTS OF ALL
----FORESTS THIS SENSOR LIES INSIDE

CREATE OR REPLACE FUNCTION increment_sensor_count()
RETURNS TRIGGER AS $$
BEGIN
	CALL incrementSensorCount_proc(NEW.x, NEW.y);
	RETURN NEW;
END;
$$
	LANGUAGE plpgsql;

CREATE TRIGGER sensorCount_tri
	AFTER INSERT
	ON SENSOR
	FOR EACH ROW
	EXECUTE PROCEDURE increment_sensor_count();

----ON UPDATE OF COVERAGE, RECOMPUTE COVERAGE PERCENTAGE
----AND TOTAL AREA OF THE FOREST

CREATE OR REPLACE FUNCTION update_percentage()
RETURNS TRIGGER AS $$
DECLARE
	new_area real;
	cover RECORD;
BEGIN
	----PREVENT RECURSIVE TRIGGERING
	IF pg_trigger_depth() > 1 THEN
		RETURN NEW;
	END IF;

	----CALCULATE AND SET NEW AREA OF FOREST AFTER THE UPDATE
	SELECT SUM(area)
	INTO new_area
	FROM COVERAGE
	WHERE forest_no = NEW.forest_no;

	UPDATE FOREST
	SET area = new_area
	WHERE forest_no = NEW.forest_no;
	
	----UPDATE PERCENTAGES FOR EACH COVERAGE OF THIS FOREST
	FOR cover IN 
		SELECT *
		FROM COVERAGE
		WHERE forest_no = NEW.forest_no
	LOOP
		UPDATE COVERAGE
		SET percentage = computePercentage(New.forest_no, 						 			  cover.area)
		WHERE forest_no = cover.forest_no AND
			 state = cover.state;
	END LOOP;

	RETURN NEW;
END;
$$
	LANGUAGE plpgsql;

CREATE TRIGGER percentage_tri
	AFTER UPDATE OR INSERT
	ON COVERAGE
	FOR EACH ROW
	EXECUTE PROCEDURE update_percentage();

----IF SENSOR REPORTS A TEMPERATURE HIGHER THAN 100,
----CREATE AN EMERGENCY
CREATE OR REPLACE FUNCTION check_emergency()
RETURNS TRIGGER AS $$
BEGIN
	IF NEW.temperature > 100 THEN
		INSERT INTO EMERGENCY VALUES
		(NEW.sensor_id, NEW.report_time);
	END IF;
	RETURN NEW;
END;
$$
	LANGUAGE plpgsql;

CREATE TRIGGER emergency_tri
	AFTER INSERT
	ON REPORT
	FOR EACH ROW
	EXECUTE PROCEDURE check_emergency();

----CHECKS THAT THE MAINTAINER OF A GIVEN SENSOR
----IS EMPLOYED BY SAME STATE THE SENSOR IS IN
----IF NOT, RAISES EXCEPTION
CREATE OR REPLACE FUNCTION check_employing_state()
RETURNS TRIGGER AS $$
DECLARE
	maintainer RECORD;
BEGIN
	IF NEW.maintainer = '-1' THEN
		RETURN NEW;
	END IF;

	----SPECIAL CASE, SENSOR HAS NO MAINTAINER
	IF NEW.maintainer IS NULL THEN
		RETURN NEW;
	END IF;

	----FINDS THE MAINTAINER OF THIS SENSOR
	SELECT *
	INTO maintainer
	FROM WORKER
	WHERE ssn = NEW.maintainer;
	
	IF maintainer IS NULL THEN
		RAISE EXCEPTION 'no such worker exists';
	END IF;	
	
	----FINDS ALL STATES THIS SENSOR COULD BE IN
	----IF MAINTAINER NOT EMPLOYED BY ANY OF THESE STATES,
	----RAISES EXCEPTION
	IF maintainer.employing_state NOT IN 
		(SELECT DISTINCT COVERAGE.state
		FROM FOREST
		JOIN COVERAGE
			ON COVERAGE.forest_no = FOREST.forest_no
		WHERE
			NEW.x >= FOREST.mbr_xmin AND 
			NEW.x <= FOREST.mbr_xmax AND
			NEW.y >= FOREST.mbr_ymin AND
			NEW.y <= FOREST.mbr_ymax)
	THEN
		RAISE EXCEPTION 'maintainer of sensor must be employed by the same state the sensor is in';
	END IF;
	
	RETURN NEW;
END;
$$
	LANGUAGE plpgsql;

CREATE TRIGGER enforceMaintainer_tri
	AFTER UPDATE OR INSERT
	ON SENSOR
	FOR EACH ROW
	EXECUTE PROCEDURE check_employing_state();

----INITIALISE forest_no 
create or replace function func_x()
returns trigger as $$
    declare id int;
    begin
    select max(forest_no)  into id from forest;
    if id IS NULL then
        new.forest_no := 1;
        end if;
    if new.forest_no = -1 then
    new.forest_no := id+1;
    end if;
return new;
end;
$$ language plpgsql;

drop trigger if exists trig_x on forest;
create trigger trig_x
    before insert on forest
    for each row
execute procedure func_x();

----INITIALISE sensor_id
create or replace function func_y()
returns trigger as $$
    declare id int;
    begin
    select max(sensor_id) into id from sensor;
    if id IS NULL then
    	new.sensor_id := 1;
    end if;
    if new.sensor_id = -1 then
    	new.sensor_id := id + 1;
    end if;
return new;
end;
$$ language plpgsql;

drop trigger if exists trig_y on sensor;
create trigger trig_y
    before insert on sensor
    for each row
execute procedure func_y();
