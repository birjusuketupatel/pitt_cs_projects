# # Set base image for container
FROM ubuntu:latest

# Install basic packages
RUN apt update
RUN apt install -y python3 pip
RUN apt-get install -y iproute2

# Set working directory in container
WORKDIR /code

# Copy pip requirements into container
COPY requirements.txt .

# Install dependencies
RUN pip install -r requirements.txt

# Copy contents of src into container
COPY src/ .

# Make chat_server runnable using ./
# RUN chmod +x chat_server.py

# Run bash on container start
# ENTRYPOINT /bin/bash