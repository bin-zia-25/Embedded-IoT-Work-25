# Week 14: MQTT Multiple Publishers and Subscribers

## Project Objective
The goal of this project is to implement a many-to-many communication architecture using the MQTT protocol. The setup involves a central broker facilitating data transfer between several independent publishing and subscribing nodes.

---

## Step 1: Network Configuration
All devices, including the broker host and the hardware nodes, are connected to the same **wireless hotspot**. This ensures that all clients are on the same subnet and can reach the broker via its local IP address.

## Step 2: Initializing the Broker
The **Mosquitto broker** is activated on the host machine. It acts as the central hub of the network, responsible for receiving all incoming messages and routing them to the appropriate subscribers based on their topic filters.

## Step 3: Configuring Publishers
Multiple nodes are programmed via **PlatformIO** to act as publishers. Each node is configured to connect to the broker and transmit data to specific, predefined topics.

## Step 4: Multi-Topic Distribution
The system is scaled by creating a variety of unique topics. This validates the broker's ability to manage multiple data streams simultaneously without cross-interference or data loss.

## Step 5: Establishing Multiple Subscribers
Multiple subscriber nodes are deployed using **PlatformIO** to receive the transmitted data.
* **Unique Client IDs:** Each subscriber is assigned a distinct ID to ensure the broker maintains independent, stable connections for every device.
* **Topic Subscription:** The subscribers are configured to listen for specific topics, allowing them to selectively process data from the publishers.

---

## System Summary
| Component | Function |
| :--- | :--- |
| **Mosquitto Broker** | Manages message distribution and client connections. |
| **Publishers** | Source nodes that send data to specific topics. |
| **Subscribers** | Target nodes that receive data using unique IDs. |
| **Topics** | The logical channels used to separate data streams. |