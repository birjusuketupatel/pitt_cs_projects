run:
	docker build -t p1_image .
	docker run -t -d --name p1_container p1_image
	docker exec -it p1_container bash

clean:
	docker stop p1_container
	docker rm p1_container

bash:
	docker exec -it p1_container bash