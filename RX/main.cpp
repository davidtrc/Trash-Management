#include <stdio.h>
#include <string.h>
#include <RCSwitch.h>
#include "mbed.h"

#define MEASURE_SIZE 26
//I/O pins
DigitalOut led_tx(LED1);
RCSwitch mySwitch = RCSwitch(D2, D3); //SERIAL2 TX & RX. (cuadrado y rectangular)
//Values to manage the received message and its ACK
char received[MEASURE_SIZE]; //Received chain in binary format
int received_value; //Received chain in int format
int received_length; //Length of the received message
char received_ack[6]; //Received chain of possible ack in binary format
int received_ack_int; //Received chain of possible ack in int format
//Values to decode the received message
char node_id[4];
char priority_message[4];
char sensor_id[4];
char lecture[14];
char* ack_message;

//------------------------------------
// UART configuration
// 9600 bauds, 8-bit data, no parity
//------------------------------------
Serial pc(SERIAL_TX, SERIAL_RX); //PRE-DEFINED PINS, CAN BE USED WITHOUT ANY PROBLEM CAUSE DON'T USE AVAILABLE PCB I/O
 
char* itoa(int value, char* result, int base);
void send_received(void);
void check_received();
void decode_received_message(void);

int main(){
    strcpy(ack_message, "101010");
    pc.printf("Inicializacion completada. Recibiendo...\n");
    while(1){
        while(!mySwitch.available()){
        }    
        if(mySwitch.available()){
            received_value = mySwitch.getReceivedValue();
            received_length = mySwitch.getReceivedBitlength();
            send_received();
            mySwitch.resetAvailable();
        }
        pc.printf("Waiting for ACK\n");
        while(!mySwitch.available()){
            //Añadir timeout para el ACK
        }
        if(mySwitch.available()){
            received_ack_int = mySwitch.getReceivedValue();
            check_received();
            mySwitch.resetAvailable();
        }
        if(received_value == 0){
        
        } else {
            decode_received_message();
        }
        pc.printf("Alcanzado el final del while1");
    }
}

void send_received(){
    itoa(received_value, received,2);
            
    if (received_value==0){
        pc.printf("Unknown format");  
    } else {
        pc.printf("I have received: %s of %d bits \r \n", received, received_length );
        wait(1);
        pc.printf("Sending received chain for ACK \n");
        led_tx = 1;
        mySwitch.send(received);
        led_tx = 0;
    }
}

void check_received(){
    itoa(received_ack_int, received_ack,2);
            
    if (received_ack_int==0){
        pc.printf("Unknown format");  
    } else if ( received_ack_int == 42 ){
        pc.printf("Transmitter confirms that the message is correct. Sending final ACK and proccessing the message\n" );
        wait(1);
        pc.printf("Sending final ACK \n");
        led_tx = 1;
        mySwitch.send(received_ack);
        led_tx = 0;
    } else {
        pc.printf("Transmitter informs that received message is not correct. Discarding and waiting for a new one\n");
        received_value = 0; 
        received_length = 0;
    }   
}

void decode_received_message(){
    node_id[0] = received[0];
    priority_message[0] = received [1];
    node_id[0] = received [2];
    lecture[0] = received [3];
    
    for(int i=0; i<3; i++){
        node_id[i] = received[i];
    }
    for(int j=4; j<8; j++){
        priority_message[j-4] = received[j];
    }
    for(int k=8; k<12; k++){
        sensor_id[k-8] = received[k];
    }
    for(int l=12; l<MEASURE_SIZE; l++){
        lecture[l-12] = received[l];
    }
    pc.printf("El nodo %s informa con prioridad %s que el sensor %s tiene una lectura de %s\n", node_id, priority_message, sensor_id, lecture);
}

char* itoa(int value, char* result, int base){
    // check that the base if valid
    if ( base < 2 || base > 36 ) {
        *result = '\0';
        return result;
    }
    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;
    do {                                                                                                                                                 
        tmp_value = value;                                                                                                                               
        value /= base;                                                                                                                                   
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];                             
    } while ( value );                                                                                                                                   
    // Apply negative sign                                                                                                                               
    if ( tmp_value < 0 )                                                                                                                                 
    *ptr++ = '-';                                                                                                                                    
    *ptr-- = '\0';                                                                                                                                       
    while ( ptr1 < ptr ) {                                                                                                                               
    tmp_char = *ptr;                                                                                                                                 
    *ptr-- = *ptr1;                                                                                                                                  
    *ptr1++ = tmp_char;                                                                                                                              
    }                                                                                                                                                    
    return result;                                                                                                                                       
}