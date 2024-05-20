----CS2550 - PRINCIPLES OF DATABASE SYSTEMS(SPRING 2024)
----DEPT. OF COMPUTER SCIENCE, UNIVERSITY OF PITTSBURGH
----HOMEWORK 2
----BIRJU PATEL, BSP22@PITT.EDU

----PRINTS ALL TABLES
SELECT * FROM STORE;
SELECT * FROM COFFEE;
SELECT * FROM CUSTOMER;
SELECT * FROM SALE;
SELECT * FROM PROMOTION;
SELECT * FROM PROMOTES;
SELECT * FROM CARRIES;

----QUESTION 3A	
SELECT CUSTOMER.fname,
	CUSTOMER.lname,
	CUSTOMER.email,
	COUNT(DISTINCT SALE.sale_id) AS sale_count
FROM SALE
JOIN CUSTOMER
	ON SALE.customer_id = CUSTOMER.customer_id
GROUP BY CUSTOMER.fname, CUSTOMER.lname, CUSTOMER.email
HAVING COUNT(DISTINCT SALE.sale_id) > 1
ORDER BY COUNT(DISTINCT SALE.sale_id) DESC;

----QUESTION 3B
SELECT CARRIES.store_id,
	STORE.name,
	STORE.store_type,
	COUNT(CARRIES.promotion_id) AS promotion_count
FROM CARRIES
JOIN STORE
	ON CARRIES.store_id = STORE.store_id
GROUP BY CARRIES.store_id, STORE.name, STORE.store_type
ORDER BY COUNT(CARRIES.promotion_id) DESC
FETCH FIRST 1 ROW WITH TIES;

----QUESTION 3C
SELECT DISTINCT ON (SALE.store_id)
	SALE.store_id,
	STORE.name,
	STORE.store_type,
	EXTRACT(YEAR FROM SALE.time_of_purchase) as year
FROM SALE
JOIN STORE
	ON SALE.store_id = STORE.store_id
GROUP BY SALE.store_id,
	STORE.name,
	STORE.store_type,
	EXTRACT(YEAR FROM SALE.time_of_purchase)
ORDER BY SALE.store_id, SUM(SALE.quantity) DESC;

----QUESTION 3D
SELECT SALE.coffee_id,
	COFFEE.name,
	COFFEE.intensity,
	RANK () OVER (
		ORDER BY SUM(SALE.quantity) DESC
	)
FROM SALE
JOIN COFFEE
	ON SALE.coffee_id = COFFEE.coffee_id
GROUP BY SALE.coffee_id, COFFEE.name, COFFEE.intensity;

----QUESTION 3E
SELECT SALE.customer_id,
	CUSTOMER.fname,
	CUSTOMER.lname,
	CUSTOMER.email,
	SUM(SALE.quantity) AS total_quantity
FROM SALE
JOIN CUSTOMER
	ON SALE.customer_id = CUSTOMER.customer_id
GROUP BY SALE.customer_id,
	CUSTOMER.fname,
	CUSTOMER.lname,
	CUSTOMER.email
ORDER BY SUM(SALE.quantity) DESC
LIMIT 3;

----QUESTION 4
DROP VIEW IF EXISTS LAST_QUARTER_PERFORMANCE;
CREATE OR REPLACE VIEW LAST_QUARTER_PERFORMANCE AS
	SELECT SALE.store_id,
		STORE.name,
		STORE.store_type,
		COUNT(DISTINCT SALE.customer_id) AS num_customers
	FROM SALE
	JOIN STORE
		ON SALE.store_id = STORE.store_id
	WHERE
		'2023-09-01' <= DATE(time_of_purchase)
		AND
		DATE(time_of_purchase) <= '2023-12-31'
	GROUP BY SALE.store_id, STORE.name, STORE.store_type;

----QUESTION 5
DROP FUNCTION IF EXISTS avg_customers_per_store;
CREATE FUNCTION avg_customers_per_store()
RETURNS float
LANGUAGE plpgsql
AS
$$
DECLARE
	avg_unique_customers float;
BEGIN
	SELECT AVG(num_customers)
	INTO avg_unique_customers
	FROM LAST_QUARTER_PERFORMANCE;
	
	RETURN avg_unique_customers;
END;
$$;

----QUESTION 6
DROP FUNCTION IF EXISTS avg_customers_per_kiosk_store;
CREATE FUNCTION avg_customers_per_kiosk_store()
RETURNS float
LANGUAGE plpgsql
AS
$$
DECLARE
	avg_unique_customers float;
BEGIN
	SELECT AVG(num_customers)
	INTO avg_unique_customers
	FROM LAST_QUARTER_PERFORMANCE
	WHERE store_type = 'kiosk';
	
	RETURN avg_unique_customers;
END;
$$;

----QUESTION 7
DROP FUNCTION IF EXISTS customer_coffee_intensity;
CREATE FUNCTION customer_coffee_intensity(cid int)
RETURNS boolean
LANGUAGE plpgsql
AS
$$
DECLARE
	most_recent_intensity int;
BEGIN
	SELECT COFFEE.intensity
	INTO most_recent_intensity
	FROM SALE
	JOIN COFFEE
		ON SALE.coffee_id = COFFEE.coffee_id
	WHERE customer_id = cid
	ORDER BY time_of_purchase DESC
	LIMIT 1;
	
	RETURN (most_recent_intensity > 6);
END;
$$;

----QUESTION 8
DROP PROCEDURE IF EXISTS monthly_coffee_promotion_proc;
CREATE PROCEDURE monthly_coffee_promotion_proc(
	beverage_id int,
	ad_id int,
	ad_name varchar(60)
)
LANGUAGE plpgsql
AS
$$
DECLARE
	start_date date;
	end_date date;
BEGIN
	SELECT CURRENT_DATE
	INTO start_date;

	SELECT CURRENT_DATE + 30
	INTO end_date;
	
	INSERT INTO PROMOTION VALUES (
		ad_id,
		ad_name,
		start_date,
		end_date
	);

	INSERT INTO PROMOTES VALUES (ad_id, beverage_id);

	COMMIT;
END;
$$;

----QUESTION 9
DROP FUNCTION IF EXISTS check_inventory_constraint CASCADE;
CREATE FUNCTION check_inventory_constraint()
RETURNS TRIGGER
LANGUAGE plpgsql
AS
$$
BEGIN
	IF (
		SELECT SUM(quantity)
		FROM SALE
		WHERE 
			time_of_purchase::date =
			NEW.time_of_purchase::date
			AND
			store_id = NEW.store_id
	) > 200
	THEN
		RAISE EXCEPTION
		'only 200 cups of coffee may be sold per day';
	END IF;

	RETURN NEW;	
END;
$$;

DROP TRIGGER IF EXISTS check_daily_inventory_trig ON SALE;
CREATE TRIGGER check_daily_inventory_trig
	AFTER INSERT
	ON SALE
	FOR EACH ROW EXECUTE PROCEDURE check_inventory_constraint();

----QUESTION 10
DROP FUNCTION IF EXISTS birthday_discount CASCADE;
CREATE FUNCTION birthday_discount()
RETURNS TRIGGER
LANGUAGE plpgsql
AS
$$
DECLARE
	bmonth varchar(3);
	bday varchar(2);
BEGIN
	SELECT month_of_birth, day_of_birth
	INTO bmonth, bday
	FROM CUSTOMER
	WHERE customer_id = NEW.customer_id;

	IF (
		bday = to_char(NEW.time_of_purchase, 'DD') AND
		bmonth = to_char(NEW.time_of_purchase, 'Mon')
	)
	THEN
		IF (NEW.quantity >= 2)
		THEN
			NEW.quantity := NEW.quantity - 1;
		ELSE
			RAISE NOTICE
			'Two drinks must be purchased as part of this sale in order to apply your birthday offer.';
		END IF;
	END IF;
	
	RETURN NEW;
END;
$$;

DROP TRIGGER IF EXISTS coffee_birthday_sale_trig ON SALE;
CREATE TRIGGER coffee_birthday_sale_trig
	BEFORE INSERT
	ON SALE
	FOR EACH ROW EXECUTE PROCEDURE birthday_discount();
