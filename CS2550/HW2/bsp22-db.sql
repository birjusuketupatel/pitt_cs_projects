----CS2550 - PRINCIPLES OF DATABASE SYSTEMS(SPRING 2024)
----DEPT. OF COMPUTER SCIENCE, UNIVERSITY OF PITTSBURGH
----HOMEWORK 2
----BIRJU PATEL, BSP22@PITT.EDU

----QUESTION 1

DROP TYPE IF EXISTS storetype CASCADE;
CREATE TYPE storetype
	AS ENUM ('sitting', 'kiosk', 'drive-through');

DROP TYPE IF EXISTS abbreviations CASCADE;
CREATE TYPE abbreviations
	AS ENUM ('Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 	'Aug', 'Sep', 'Oct', 'Nov', 'Dec');

DROP TABLE IF EXISTS STORE CASCADE;
CREATE TABLE STORE (
	store_id integer,
	name varchar(60),
	store_type storetype,
	gps_lon numeric(7,2),
	gps_lat numeric(7,2),
	CONSTRAINT STORE_PK
		PRIMARY KEY (store_id)
		NOT DEFERRABLE,
	CONSTRAINT STORE_AK
		UNIQUE(name)
		DEFERRABLE INITIALLY DEFERRED
);

DROP TABLE IF EXISTS COFFEE CASCADE;
CREATE TABLE COFFEE (
	coffee_id integer,
	name varchar(60),
	description varchar(280),
	country_of_origin varchar(60),
	intensity integer,
	price numeric(7,2),
	CONSTRAINT COFFEE_PK
		PRIMARY KEY (coffee_id)
		NOT DEFERRABLE,
	CONSTRAINT VALID_INTENSITY
		CHECK (intensity >= 1 AND intensity <= 12),
	CONSTRAINT VALID_PRICE
		CHECK (price > 0)
);

DROP TABLE IF EXISTS CUSTOMER CASCADE;
CREATE TABLE CUSTOMER (
	customer_id integer,
	fname varchar(60),
	lname varchar(60),
	email varchar(60),
	month_of_birth abbreviations,
	day_of_birth varchar(2),
	CONSTRAINT CUSTOMER_PK
		PRIMARY KEY (customer_id)
		NOT DEFERRABLE,
	CONSTRAINT CUSTOMER_AK
		UNIQUE (email)
		DEFERRABLE INITIALLY DEFERRED,
	CONSTRAINT VALID_DAY
		CHECK (
		day_of_birth ~ '(0[0-9])|(1[0-9])|(2[0-9])|(30)|(31)'
		)
);

DROP TABLE IF EXISTS SALE CASCADE;
CREATE TABLE SALE (
	sale_id integer,
	customer_id integer,
	store_id integer,
	coffee_id integer,
	quantity integer,
	time_of_purchase timestamp,
	CONSTRAINT SALE_PK
		PRIMARY KEY (sale_id)
		NOT DEFERRABLE,
	CONSTRAINT CUSTOMER_FK
		FOREIGN KEY (customer_id)
		REFERENCES CUSTOMER (customer_id)
		DEFERRABLE INITIALLY IMMEDIATE,
	CONSTRAINT STORE_FK
		FOREIGN KEY (store_id)
		REFERENCES STORE (store_id)
		DEFERRABLE INITIALLY IMMEDIATE,
	CONSTRAINT COFFEE_FK
		FOREIGN KEY (coffee_id)
		REFERENCES COFFEE (coffee_id)
		DEFERRABLE INITIALLY IMMEDIATE
);

DROP TABLE IF EXISTS PROMOTION CASCADE;
CREATE TABLE PROMOTION (
	promotion_id integer,
	name varchar(60),
	start_date date,
	end_date date,
	CONSTRAINT PROMOTION_PK
		PRIMARY KEY (promotion_id)
		NOT DEFERRABLE,
	CONSTRAINT VALID_RANGE
		CHECK (start_date <= end_date)
);

DROP TABLE IF EXISTS PROMOTES CASCADE;
CREATE TABLE PROMOTES (
	promotion_id integer,
	coffee_id integer,
	CONSTRAINT PROMOTES_PK
		PRIMARY KEY (promotion_id, coffee_id)
		NOT DEFERRABLE,
	CONSTRAINT PROMOTION_FK
		FOREIGN KEY (promotion_id)
		REFERENCES PROMOTION (promotion_id)
		DEFERRABLE INITIALLY IMMEDIATE,
	CONSTRAINT COFFEE_FK
		FOREIGN KEY (coffee_id)
		REFERENCES COFFEE (coffee_id)
		DEFERRABLE INITIALLY IMMEDIATE
);

DROP TABLE IF EXISTS CARRIES CASCADE;
CREATE TABLE CARRIES (
	promotion_id integer,
	store_id integer,
	CONSTRAINT CARRIES_PK
		PRIMARY KEY (promotion_id, store_id)
		NOT DEFERRABLE,
	CONSTRAINT PROMOTION_FK
		FOREIGN KEY (promotion_id)
		REFERENCES PROMOTION (promotion_id)
		DEFERRABLE INITIALLY IMMEDIATE,
	CONSTRAINT STORE_FK
		FOREIGN KEY (store_id)
		REFERENCES STORE (store_id)
);