#!/bin/bash
# Run ShipNetSim Server with RabbitMQ

# Start RabbitMQ first
docker-compose up -d rabbitmq

# Wait for RabbitMQ to be ready
echo "Waiting for RabbitMQ to start..."
sleep 10

# Run the server
docker-compose up shipnetsim-server
