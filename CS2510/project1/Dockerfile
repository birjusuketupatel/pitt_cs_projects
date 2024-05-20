# # Set base image for container
FROM ubuntu:latest

# Install basic packages
RUN apt update
RUN apt install -y python3 pip

# Set working directory in container
WORKDIR /code

# Copy pip requirements into container
COPY requirements.txt .

# Install dependencies
RUN pip install -r requirements.txt

# Copy contents of src into container
COPY src/ .

# Run bash on container start
ENTRYPOINT /bin/bash