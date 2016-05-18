#include <stdio.h>
#include <string.h>
#include <RCSwitch.h>
#include <math.h> 
#include "mbed.h"

#define MEASURE_SIZE 26

/******      I/O PINS      ********/
DigitalOut led_tx(LED1);
RCSwitch mySwitch = RCSwitch(D2, D3); //SERIAL2 TX & RX. (cuadrado y rectangular)
/****** End of I/O PINS     *******/

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
char decoded_message[150];

//------------------------------------
// UART configuration
// 9600 bauds, 8-bit data, no parity
//------------------------------------
Serial pc(SERIAL_TX, SERIAL_RX); //PRE-DEFINED PINS, CAN BE USED WITHOUT ANY PROBLEM CAUSE DON'T USE AVAILABLE PCB I/O
void send_received(void);
void check_received();
void decode_received_message(void);
int bin2dec(const char* binary);
char* itoa(int value, char* result, int base);

static time_t start, end; //with them, can count time waiting for ACK
static int seconds_elapsed = 0;

/**************************************************************************************
*                                                                                     *
*   Function: main()                                                                  *
*   Executes the main program. Waits to receive a packet, and when comes, resend it   *
*   to check if it is correct. If is, send ACK and decode the message, split it in    *
*   node ID, priority of the message, sensor ID, and lecture                          *
*   If nothing happends, at five minutes send the information of every sensor.        *
*                                                                                     *
**************************************************************************************/
int main(){
    strcpy(ack_message, "101010"); //Hard code the ack string message
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
        pc.printf("Esperando para el ACK\n");
        time(&start);
        while(!mySwitch.available()){
            time(&end);
            seconds_elapsed = difftime(start, end);
            if(seconds_elapsed >=2){
                pc.printf("No se ha recibido el ACK, se considera la secuencia recibida valida y se pasa a su decodificacion\n");
                break;
            }
        }
        if(mySwitch.available()){
            received_ack_int = mySwitch.getReceivedValue();
            check_received();
            mySwitch.resetAvailable();
        }
        if((received_value != 0) && (received_length >=7)){
            decode_received_message();
        }
    }
}

/**************************************************************************************
*                                                                                     *
*   Function: send_received()                                                         *
*   Prints the received binary chain and resends it. After, waits for and answer of   *
*   the transmitter, saying if the chain was correctly received.                      *
*                                                                                     *
**************************************************************************************/
void send_received(){
    itoa(received_value, received,2);
            
    if (received_value==0){
        pc.printf("No reconocido");  
    } else {
        pc.printf("He recibido: %s de %d bits \n", received, received_length );
        wait_ms(800);
        pc.printf("Reenviando la secuencia recibida para comprobar si es correcta\n");
        led_tx = 1;
        mySwitch.send(received);
        led_tx = 0;
    }
}

/**************************************************************************************
*                                                                                     *
*   Function: check_received()                                                        *
*   Check if the transmitter informs that chain is correct or not.                    *
*                                                                                     *
**************************************************************************************/
void check_received(){
    itoa(received_ack_int, received_ack,2);
            
    if (received_ack_int==0){
        pc.printf("No reconocido"); 
    } else if ( received_ack_int == 42 ){ // 42 == 101010, binary ACK
        pc.printf("El transmisor confirma que el mensaje es correcto. Enviando ACK final y procesando el mensaje\n" );
        wait_ms(800);
        led_tx = 1;
        mySwitch.send(received_ack);
        led_tx = 0;
    } else {
        pc.printf("El transmisor informa de que la secuencia no es correcta. Descartada y esperando a recibir de nuevo...\n");
        received_value = 0; 
        //received_length = 0;
    }   
}

/**************************************************************************************
*                                                                                     *
*   Function: decode_received_message()                                               *
*   Takes the received binary chain and decode it, splitting in node ID, priority of  *
*   the message, sensor ID, and lecture in human comprehensible way                   *
*                                                                                     *
**************************************************************************************/
void decode_received_message(){
    int lecture_dec;
    int i = 0;
    for (i = 0; i<4; i++){
        node_id[i] = received[i];
    }
    for (i = 4; i<8; i++){
        priority_message[i-4] = received[i];
    }
    for (i = 8; i<12; i++){
        sensor_id[i-8] = received[i];
    }
    for (i = 12; i<received_length; i++){
        lecture[i-12] = received[i];
    }
    strcpy(decoded_message, "El nodo ");
    pc.printf("El nodo ");
    for(int i=0; i<sizeof(node_id); i++){
        pc.printf("%c", node_id[i]);
    }
    strcat(decoded_message, node_id);
    strcat(decoded_message, " informa con prioridad ");
    pc.printf(" informa con prioridad ");
    if(priority_message[0] == '1' && priority_message[1] == '1' && priority_message[2] == '1' && priority_message[3] == '1' ){
        pc.printf("ALTA ");
        strcat(decoded_message, "ALTA ");
    }
    if(priority_message[0] == '1' && priority_message[1] == '0' && priority_message[2] == '1' && priority_message[3] == '0' ){
        pc.printf("normal ");
        strcat(decoded_message, "normal ");
    }
    strcat(decoded_message, " que el sensor ");
    pc.printf(" que el sensor ");
    if(sensor_id[0] == '1' && sensor_id[1] == '0' && sensor_id[2] == '0' && sensor_id[3] == '1' ){
        pc.printf("de presion ");
        strcat(decoded_message, "de presion ");
    }
    if(sensor_id[0] == '1' && sensor_id[1] == '0' && sensor_id[2] == '1' && sensor_id[3] == '1' ){
        pc.printf("de ultrasonidos ");
        strcat(decoded_message, "de ultrasonidos ");
    }
    if(sensor_id[0] == '1' && sensor_id[1] == '1' && sensor_id[2] == '0' && sensor_id[3] == '1' ){
        pc.printf("de llama ");
        strcat(decoded_message, "de llama ");
    }
    if(sensor_id[0] == '1' && sensor_id[1] == '1' && sensor_id[2] == '1' && sensor_id[3] == '1' ){
        pc.printf("de gases ");
        strcat(decoded_message, "de gases ");
    }
    pc.printf(" tiene una lectura de ");
    strcat(decoded_message, "tiene una lectura de ");
    for(int i=0; i<(received_length-12); i++){
        pc.printf("%c", lecture[i]);
        
    }
    pc.printf("\r \n");
    
    lecture_dec =  bin2dec(lecture);
    char lecture_char = lecture_dec;
    //strcat(decoded_message, lecture_char);
    //pc.printf("%s", decoded_message);
    //pc.printf("lecturaenint: %d", lecture_dec); 
}

int bin2dec(const char* binary){
    int len,dec=0,i,exp;

    len = strlen(binary);
    exp = len-1;

    for(i=0;i<len;i++,exp--)
        dec += binary[i]=='1'?pow((double)2,exp):0;
    return dec;
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