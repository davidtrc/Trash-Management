# Trash-Management
System made to manage a public trash container, implemented with sensors, actuators, and communication between nodes (P2P) and with
a master node (central node).

This project is based on a NUCLEO (used model F446RE, but it should work in any model, making some lift changes) for regular nodes
(TX folder) and a Raspberry (RX folder).

The name of the folders is because the main role of each node is transmission or reception, but both can do both.

You can use the libraries that I attach to my project, but it can be obsoletes by the time you read this (wrote 5-12-2016), so I
recommend download them from the mbed Compiler.

The RCSwitch library is made by Chris Dick and you can find it in https://developer.mbed.org/users/TheChrisyd/code/RCSwitch/

The HX711 library is made by Cr-300-Litho and you can find it in https://developer.mbed.org/teams/Cr300-Litho/code/HX711/

The main in TX folder reads the sensors. The trash made has:
    
    · One of pressure: to detect the weight of the residues
    · One ultrasonic: placed in the roof of the trash can, measures the height of the residues
    · One flame sensor: to detect fire.
    · One of gasses: to measure levels of, por example, methane or carbon monoxid
    
After read them, send the data in five minutes intervals to the central node. If detects fire or anormal gasses levels it sends the alarm
inmediately.

The main in RX folder receives the data of the nodes and process them.
