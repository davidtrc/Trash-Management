//////////////////////////////////////////////////
//             TO DO                            //
//                                              //
//  Add encryption                              //
//  Correct the read_pressure function          //
//                                              //
//////////////////////////////////////////////////
//https://developer.mbed.org/users/NickRyder/code/Pulse/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <RCSwitch.h>
#include <HX711.h>
#include <Pulse.h>
#include "mbed.h"

#define RESOLUTION 10000 //10000 = 14 bits. This will multiply the read measure (between 0-1)
#define MEASURE_SIZE 17
#define TIME_SEND_S 30 //Defines time elapsed, in seconds, between data sending

/******      I/O PINS      ********/
DigitalOut led_tx(LED1); //LED in nucleo
RCSwitch mySwitch = RCSwitch(D2, D3); //SERIAL2 TX & RX. Its on the bottom of the Nucleo. Initialices a RCSwitch object, in charge of manage the 434 MHz modems
DigitalIn fire_sensor(A0); //Pin to connect fire sensor
//DigitalInOut ultrasound_sensor1(D4); //Pin to connecto Ultrasound sensor. It's inout cause it sends a signal and waits to receive the echo
PulseInOut ultrasound_sensor = PulseInOut(D4); //Init a PulseInOut object, necessary to read ultrasonic sensor, cause mbed dont have PulseIn, used with this sensor in Arduino
HX711 pressureAmplifier = HX711(D5, D6); //(CLK signal, Data signal). Initialices a HX711 object, a circuit necesarry to amplify the pressure sensor signal
//Rest of sensor would be in the others analog or digital pins
/****** End of I/O PINS     *******/

//Sensors ID's
char pressure_id[] = "1001";  //IDs of the sensors. They are hard code and allow a maximum of 16 sensors
char ultrasound_id[] = "1011";
char fire_id[] = "1101";
char gas_id[] = "1111";
//Sensor message type
int pressure_type = 0; // 0 = Regular message, 1 = High Priority Message (alert)
int ultrasound_type = 0;
int gas_type = 0;
int fire_type = 0;
//Sensors lectures (stores read value between 0 and 1)
float pressure_reading_sensor1 = 0;
float ultrasound_reading_sensor1 = 0;
float fire_reading = 0;
float gas_reading = 0;
//Sensores lectures in binary format (same as above but in binary)
char pressure_bin[MEASURE_SIZE];
char ultrasound1_bin[MEASURE_SIZE];
char fire_bin[MEASURE_SIZE];
char gas_bin[MEASURE_SIZE];
//Finals binary string to send (NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE)
char* pressure_final_chain;
char* ultrasound1_final_chain;
char* fire_final_chain;
char* gas_final_chain;
//Intermediate char used in functions to forme the char* from above. If not declare here, the functions doesn't works fine
char formated_chain1[32];
char formated_chain2[32];
char formated_chain3[32];
char formated_chain4[32];

char received[26]; //Stores in binary string format the message received

static time_t start, end; //with them, can count time passed between data sendings
static double seconds_elapsed = 0; //Stores the time passed between above timers

char* actual_message; //Stores the last message send. Useful for wait_for_ACK function
int times_resend = 0; //Times that resend is called. When 6, the programs desists in receive an answer and go on 
//--------------------------------------//
// UART configuration                   //
// 9600 bauds, 8-bit data, no parity    //
//--------------------------------------//
Serial pc(SERIAL_TX, SERIAL_RX); //PRE-DEFINED PINS, CAN BE USED WITHOUT ANY PROBLEM CAUSE DON'T USE AVAILABLE NUCLEO I/O 
void read_pressure (void);
void read_ultrasound (void);
void read_fire (void);
void read_gas (void);
char* formatting_pressure_message (int type_message, char* sensor_id);
char* formatting_ultrasound_message (int type_message, char* sensor_id);
char* formatting_fire_message (int type_message, char* sensor_id);
char* formatting_gas_message (int type_message, char* sensor_id);
unsigned long encryption(unsigned long message);
void resend(void);
void wait_for_ack(void);
char* itoa(int value, char* result, int base);

/**************************************************************************************
*                                                                                     *
*   Function: main()                                                                  *
*   Executes the main program. Constantly reads the fire and gas sensor. If detects   *
*   something wrong, waits 2 seconds and repeats the measure. If persists, starts     *
*   the process to send the information to the central node.                          *
*   If nothing happends, at five minutes send the information of every sensor.        *
*                                                                                     *
**************************************************************************************/
int main(){ 
    pc.printf("Inicializacion completada. Emitiendo...\n");
    time(&start); //Starts the count of five minutes
    while(1) {
        led_tx = 0;
        read_fire();
        read_gas();
        read_ultrasound();
        if(fire_type == 1){
            wait(2); //Wait for check that the alert is not casual. If the alert persists, notify inmediately the central node
            read_fire();
            if(fire_type == 1){
                led_tx = 1; //Useful to check if Nucleo is sending or idle
                printf("Detectado fuego. Comunicando al nodo central\n");
                mySwitch.send(fire_final_chain);
                led_tx = 0;
                actual_message = fire_final_chain; //Make actual_message equals to the message to send, for resend function
                wait_for_ack();
                printf("Nada mas por enviar \r \n");
            }
        }
        if(gas_type == 1){
            wait(2);
            read_gas();
            if(gas_type == 1){
                led_tx = 1;
                printf("Detectados niveles peligrosos de gases. Comunicando al nodo central\n");
                mySwitch.send(gas_final_chain);
                led_tx = 0;
                actual_message = gas_final_chain;
                wait_for_ack();
                printf("Nada mas por enviar \r \n");
            }
        }
        time(&end); //Get actual time in another timer
        seconds_elapsed = difftime (end,start); //Calculates the difference between timers, equals to seconds elapsed
        if(seconds_elapsed >= TIME_SEND_S){
            read_pressure();
            led_tx = 1;
            printf("Transmitiendo peso\n");
            mySwitch.send(pressure_final_chain);
            led_tx = 0;
            actual_message = pressure_final_chain;
            wait_for_ack();
            wait_ms(800); //Wait to start another data sending to avoid collisions and/or the receiver is not ready
            read_ultrasound();
            led_tx = 1;
            printf("Transmitiendo altura de la basura\n");
            mySwitch.send(ultrasound1_final_chain);
            led_tx = 0;
            actual_message = ultrasound1_final_chain;
            wait_for_ack();
            wait_ms(800); //Wait to start another data sending to avoid collisions and/or the receiver is not ready
            led_tx = 1;
            printf("Transmitiendo sensor de llama\n");
            mySwitch.send(fire_final_chain);
            led_tx = 0;
            actual_message = fire_final_chain;
            wait_for_ack();
            wait_ms(800); //Wait to start another data sending to avoid collisions and/or the receiver is not ready
            led_tx = 1;
            printf("Transmitiendo niveles de gases\n");
            mySwitch.send(gas_final_chain);
            led_tx = 0;
            actual_message = gas_final_chain;
            wait_for_ack();
            wait_ms(800); //Wait to start another data sending to avoid collisions and/or the receiver is not ready
            printf("Nada mas por enviar\r \n");
            time(&start); //reset the timer
        }
    }
}

/*************************************************************************************
*                                                                                    *
*   Function: resend()                                                               *
*   Resends the last send data and waits for the ack. This function is called by     *
*   wait_for_ack if that functions doesnt obtain an answer from central node or its  *
*   wrong                                                                            *
*                                                                                    *
**************************************************************************************/
void resend(void){
    led_tx = 1;
    mySwitch.send(actual_message);
    led_tx = 0;
    wait_for_ack();
}

/*************************************************************************************
*                                                                                    *
*   Function: wait_for_ack()                                                         *
*   Waits during five seconds for ack of the central node when data is sent.         *
*   If nothing is received or central node doesnt send the correc tbinary chain (it  *
*   must return the same that was sent) resend the actual message                    *
*                                                                                    *
**************************************************************************************/
void wait_for_ack(void){
    time_t start_ack, stop_ack; //With them count the time that program are waiting for ACK
    time(&start_ack);
    int dif_time=0; //Time passed between time_t's
    while (dif_time<=5){
        if(mySwitch.available()){
            int value = mySwitch.getReceivedValue();
            itoa(value, received, 2); //Explained below. Makes a binary char* from a int
            if (value==0){ //The received message is empty or RCSwitch gives an error
                pc.printf("No reconocido");
                led_tx = 1;
                mySwitch.send(actual_message);
                led_tx = 0;
                dif_time = 0;
                mySwitch.resetAvailable();
                wait_for_ack();
            } else if (value == 42){ //42 is 101010. Its sended by Receptor when TX send an ACK
                dif_time = 0;
                times_resend = 0;
                mySwitch.resetAvailable();
                pc.printf("ACK recibido \r\n");
                return;
            } else if(*(received) == *(actual_message)){ //If received is equals as sended, send ACK
                pc.printf("El destinatario confirma recepcion sin errores. Enviando ACK y pasando a esperar el ACK de vuelta\n");
                strcpy(actual_message, "101010");
                wait_ms(800);
                led_tx = 1;
                mySwitch.send(actual_message); //101010 == ACK
                led_tx = 0;
                dif_time = 0;
                mySwitch.resetAvailable();
                wait_for_ack();
            } else{
                pc.printf("Recibido: %d de %d bits  \n \r", mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength());
                pc.printf("Algo ha salido mal. Probando a reenviar la secuencia... \n \r");
                dif_time = 0;
                times_resend++;
                mySwitch.resetAvailable();
                resend(); //If received is not equal as sended, resend the message
            }
            mySwitch.resetAvailable();
            dif_time = 0;
            return;
        }
        time(&stop_ack);
        dif_time = difftime(stop_ack, start_ack);
    }
    pc.printf("No se ha obtenido ninguna respuesta por parte del receptor. Emitiendo de nuevo...\n \r");
    times_resend++;
    dif_time = 0;
    if(times_resend >= 7){
        pc.printf("No ha sido posible obtener respuesta del receptor. Se han realizado 6 intentos de envio");
        times_resend = 0;
        return;
    }
    resend();
}

/*************************************************************************************
*                                                                                    *
*   Function: read_pressure()                                                        *
*   Reads the pressure sensor and makes the binary chain to be send. That chain      *
*   includes NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE                        *
*                                                                                    *
**************************************************************************************/
void read_pressure (void){
    int measure = pressureAmplifier.getGram(); //Cause max weight the sensor can measure is 10.000 grams its not necessary multiply by RESOLUTION 
    itoa(measure, pressure_bin, 2);
    if(measure < 6000){
        pressure_type = 0; //That variable (sensor_type) indicates the priority of the message to be formed. 0 is regular priority and 1 is high priority
    }else{
        pressure_type = 1;
    }
    pressure_final_chain = formatting_pressure_message(pressure_type, pressure_id); //pressure_id is the id of the sensor. Unique for each sensor.
}

/*************************************************************************************
*                                                                                    *
*   Function: read_ultrasound()                                                      *
*   Reads the ultrasound sensor and makes the binary chain to be send. That chain    *
*   includes NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE                        *
*                                                                                    *
**************************************************************************************/
void read_ultrasound (void){
    long duration;
    int cm;
    ultrasound_sensor.write(0); //Reset bus
    wait_us(2);
    ultrasound_sensor.write_us(1,5); //(Value, time). The time and the method to measure is the recommended by sensor manufacturer
    duration = ultrasound_sensor.read_high_us(1000000); //int timeout in us. If one second elapsed, and didn't receive a pulse, return -1
    if(duration == -1){
        wait_ms(100);
        read_ultrasound(); //If the measure is wrong, do it again
    }
    cm = duration / 29 / 2;
    if(cm == 0){
        wait_ms(100);
        read_ultrasound();
    }
    int read_ultrasound1 = cm * 10; //Here I dont use RESOLUTION cause max value of cm is 400, and this value * RESOLUTION can cause an overflow
    itoa(read_ultrasound1, ultrasound1_bin,2);
    if((read_ultrasound1 < 20)){ //For the demo, 20 is 2 cm
        ultrasound_type = 1;
    }else{
        ultrasound_type = 0;
    }
    ultrasound1_final_chain = formatting_ultrasound_message(ultrasound_type, ultrasound_id);
}

/*************************************************************************************
*                                                                                    *
*   Function: read_fire()                                                            *
*   Reads the fire sensor and makes the binary chain to be send. That chain          *
*   includes NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE                        *
*                                                                                    *
**************************************************************************************/
void read_fire (void){
    int read_fire;
    fire_reading = fire_sensor.read();
    read_fire = fire_reading * RESOLUTION;
    itoa(read_fire, fire_bin,2);
    if((read_fire < 9000)){ //This sensor is digital, so the measure is only 0 or 1. Multiply by RESOLUTION makes it 0 or 10000. The threshold, then, is indifferent
        fire_type = 0;
    }else{
        fire_type = 1;
    }
    fire_final_chain = formatting_fire_message(fire_type, fire_id);
}

/*************************************************************************************
*                                                                                    *
*   Function: read_gas()                                                             *
*   Reads the gas sensor and makes the binary chain to be send. That chain           *
*   includes NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE                        *
*                                                                                    *
**************************************************************************************/
void read_gas (void){
    int read_gas;
    gas_reading = 0.00776556; //Cause I havent a gas sensor I hard coded one hypotetical value.
    read_gas = gas_reading * RESOLUTION;
    itoa(read_gas, gas_bin,2);
    if(read_gas < 80){
        gas_type = 0;
    }else{
        gas_type= 1;
    }
    gas_final_chain = formatting_gas_message(gas_type, gas_id);
}

/*************************************************************************************
*                                                                                    *
*   Function: formatting_pressure_message(int type_message, char* sensor_id)         *
*   Makes the final chain (which will be send) that includes                         *
*   NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE  and returns it                 *
*                                                                                    *
**************************************************************************************/
char* formatting_pressure_message (int type_message, char* sensor_id){
    //Its always better to start copy strings to a char* with strcpy function an continue with strcat (used below)
    strcpy(formated_chain1, "1010"); //NODE ID
    if(type_message == 0){
       strcat(formated_chain1, "1010"); //Type of message (1010 = Regular; 1111 = Alert)
    }else{
       strcat(formated_chain1, "1111");
    }
    strcat(formated_chain1, sensor_id); //Add to the chain the sensor id
    strcat(formated_chain1, pressure_bin); //Add to the chain the sensor id
    
    return formated_chain1;
} 

/*************************************************************************************
*                                                                                    *
*   Function: formatting_ultrasound_message(int type_message, char* sensor_id)       *
*   Makes the final chain (which will be send) that includes                         *
*   NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE  and returns it                 *
*                                                                                    *
**************************************************************************************/
char* formatting_ultrasound_message (int type_message, char* sensor_id){
    strcpy(formated_chain2, "1010"); //NODE ID
    if(type_message == 0 ){
       strcat(formated_chain2, "1010"); //Type of message (1010 = Regular; 1111 = Alert)
    }else{
       strcat(formated_chain2, "1111");
    }
    strcat(formated_chain2, sensor_id);
    strcat(formated_chain2, ultrasound1_bin); 
    
    return formated_chain2;
}

/*************************************************************************************
*                                                                                    *
*   Function: formatting_fire_message(int type_message, char* sensor_id)             *
*   Makes the final chain (which will be send) that includes                         *
*   NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE  and returns it                 *
*                                                                                    *
**************************************************************************************/
char* formatting_fire_message (int type_message, char* sensor_id){
    strcpy(formated_chain3, "1010"); //NODE ID
    if(type_message == 0 ){
       strcat(formated_chain3, "1010"); //Type of message (1010 = Regular; 1111 = Alert)
    }else{
       strcat(formated_chain3, "1111");
    }
    strcat(formated_chain3, sensor_id);
    strcat(formated_chain3, fire_bin);
    
    return formated_chain3;
}

/*************************************************************************************
*                                                                                    *
*   Function: formatting_gas_message(int type_message, char* sensor_id)              *
*   Makes the final chain (which will be send) that includes                         *
*   NODE ID + PRIORITY_MESSAGE + SENSOR ID + LECTURE  and returns it                 *
*                                                                                    *
**************************************************************************************/
char* formatting_gas_message (int type_message, char* sensor_id){ 
    strcpy(formated_chain4, "1010"); //NODE ID
    if(type_message == 0 ){
       strcat(formated_chain4, "1010"); //Type of message (1010 = Regular; 1111 = Alert)
    }else{
       strcat(formated_chain4, "1111");
    }
    strcat(formated_chain4, sensor_id);
    strcat(formated_chain4, gas_bin);
    
    return formated_chain4;
}

/*************************************************************************************
*                                                                                    *
*   Function: itoa(int value, char* result, int base)                                *
*   Takes a int value and writes in char* result, the transformation of that value   *
*   in the base passed as parameter. I.e, base = 2 is binary. The return of the      *
*   function is the same array passed as parameter, so is not needed to make a       *
*   statement like char* array = itoa(...) if you dont want two equals arrays        *
*   Obtained from:                                                                   *
*   https://developer.mbed.org/teams/auPilot/code/auSpeed/file/d38b3edad9b3/itoa.cpp *
*                                                                                    *
**************************************************************************************/
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