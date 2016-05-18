#include "RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h> 
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <ctype.h>

#define MEASURE_SIZE 26

/******      I/O PINS      ********/
//DigitalOut led_tx(LED1);
RCSwitch mySwitch; //SERIAL2 TX & RX. (cuadrado y rectangular)
/****** End of I/O PINS     *******/

//Values to manage the received message and its ACK
char* received; //Received chain in binary format
int received_value; //Received chain in int format
int received_length; //Length of the received message
char received_ack[6]; //Received chain of possible ack in binary format
int received_ack_int; //Received chain of possible ack in int format
//Values to decode the received message
char* node_id;
char* priority_message;
char* sensor_id;
char* lecture;
char* ack_message = (char*) "101010";
char decoded_message[150];

static time_t start, end; //with them, can count time waiting for ACK
static double seconds_elapsed = 0;
FILE *modelo;

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

/**************************************************************************************
*                                                                                     *
*   Function: bin2dec(const char* binary)                                             *
*   Converts a binary chain into the correct decimal value. Returns that decimal 	  *
* 	Obtained from: 																	  *
* 	http://stackoverflow.com/questions/2548282/decimal-to-binary-and-vice-versa		  *
*                                                                                     *
**************************************************************************************/
int bin2dec(const char* binary){
    int len,dec=0,i,exp;

    len = strlen(binary);
    exp = len-1;

    for(i=0;i<len;i++,exp--)
        dec += binary[i]=='1'?pow((double)2,exp):0;
    return dec;
}

/**************************************************************************************
*                                                                                     *
*   Function: send_received()                                                         *
*   Prints the received binary chain and resends it. After, waits for and answer of   *
*   the transmitter, saying if the chain was correctly received.                      *
*                                                                                     *
**************************************************************************************/
void send_received(){
	int PIN2 = 3;
	mySwitch = RCSwitch();
	mySwitch.enableTransmit(PIN2);
    itoa(received_value, received,2);
    
    if (received_value==0){
        printf("No reconocido");  
    } else {
        printf("He recibido: %s de %d bits \n", received, received_length );
        usleep(800000);
        printf("Reenviando la secuencia recibida para comprobar si es correcta\n");
        mySwitch.send(received);
    }
}

/**************************************************************************************
*                                                                                     *
*   Function: check_received()                                                        *
*   Check if the transmitter informs that chain is correct or not.                    *
*                                                                                     *
**************************************************************************************/
void check_received(){
	
	int PIN2 = 3;
	mySwitch = RCSwitch();
	mySwitch.enableTransmit(PIN2);
	
    itoa(received_ack_int, received_ack,2);
            
    if (received_ack_int==0){
        printf("No reconocido"); 
    } else if ( received_ack_int == 42 ){ // 42 == 101010, binary ACK
        printf("El transmisor confirma que el mensaje es correcto. Enviando ACK final y procesando el mensaje\n" );
        usleep(800000);
        mySwitch.send(received_ack);
    } else {
        printf("El transmisor informa de que la secuencia no es correcta. Descartada y esperando a recibir de nuevo...\n");
        received_value = 0; 
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
	
	node_id = (char*)malloc(4*sizeof(char)); //Allocates memory for the variables
	priority_message = (char*)malloc(4*sizeof(char));
	sensor_id = (char*)malloc(4*sizeof(char));
	lecture = (char*)malloc((received_length-12)*sizeof(char));
	//This for's sequences decompose the received chain in node_id, priority_message, sensor_id, lecture
    int i = 0;
    for (i = 0; i<4; i++){
        *node_id = *received;
        received++;
        node_id++;
    }
    for (i = 0; i<4; i++){
        *priority_message = *received;
        priority_message++;
        received++;
        
    }
    for (i = 0; i<4; i++){
        *sensor_id = *received;
        sensor_id++;
        received++;
    }
    for (i = 0; i<(received_length-12); i++){
        *lecture = *received;
        lecture++;
        received++;
        
    }
    //Back the pointers to the original position
    for (i = 0; i<4; i++){
		node_id--;
		priority_message--;
		sensor_id--;
	}
	for (i = 0; i<(received_length-12); i++){
		lecture--;
	}
	for(i=0; i< received_length; i++){
		received--;
	}
	//Prints the received chain in understandable human way and stores it in a file
    printf("El nodo ");
    strcpy(decoded_message, "El nodo ");
    printf("%s", node_id);
    strcat(decoded_message, node_id);
    printf(" informa con prioridad ");
    strcat(decoded_message, " informa con prioridad ");
    if(priority_message[1] == '1'){
        printf("ALTA ");
        strcat(decoded_message, "ALTA ");
    }
    if(priority_message[1] == '0'){
        printf("normal ");
        strcat(decoded_message, "normal ");
    }
    printf("que el sensor ");
    strcat(decoded_message, "que el sensor ");
    if(sensor_id[0] == '1' && sensor_id[1] == '0' && sensor_id[2] == '0' && sensor_id[3] == '1' ){
        printf("de presion ");
        strcat(decoded_message, "de presion ");
    }
    if(sensor_id[0] == '1' && sensor_id[1] == '0' && sensor_id[2] == '1' && sensor_id[3] == '1' ){
        printf("de ultrasonidos ");
        strcat(decoded_message, "de ultrasonidos ");
    }
    if(sensor_id[0] == '1' && sensor_id[1] == '1' && sensor_id[2] == '0' && sensor_id[3] == '1' ){
        printf("de llama ");
        strcat(decoded_message, "de llama ");
    }
    if(sensor_id[0] == '1' && sensor_id[1] == '1' && sensor_id[2] == '1' && sensor_id[3] == '1' ){
        printf("de gases ");
        strcat(decoded_message, "de gases ");
    }
    printf("tiene una lectura de ");
    strcat(decoded_message, "tiene una lectura de ");
    
    int lecture_dec =  (bin2dec(lecture));
    printf("%d (%s)", lecture_dec, lecture);
    printf(" \r\n");
    
    
    char str[10];
    sprintf(str, "%d", lecture_dec);
    strcat(decoded_message, str);
    
    
    const char* ending = "\r\n"; 
    strcat(decoded_message, ending);
    //Opens the file, writes in and close it
    modelo = fopen("log-cubos.txt", "a"); //Es el MODELO. Almacena todos los datos de las aulas.
    fputs(decoded_message, modelo);
    fclose(modelo);
    //Commented cause free the pointers cause a non predictable behavior
    /*
    free(node_id);
    free(sensor_id);
    free(priority_message);
    free(lecture);*/
}

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
	
	int PIN = 2;
	if(wiringPiSetup() == -1) {
       printf("wiringPiSetup failed, exiting...");
       return 0;
     }
    mySwitch = RCSwitch();
	mySwitch.enableReceive(PIN);  // Receiver on interrupt 0 => that is pin #2
    
    printf("Inicializacion completada. Recibiendo...\n");
    while(1){
        while(!mySwitch.available());
        if(mySwitch.available()){
            received_value = mySwitch.getReceivedValue();
            received_length = mySwitch.getReceivedBitlength();
            received = (char*)malloc(received_length*sizeof(char));
            send_received();
            mySwitch.resetAvailable();
        }
        printf("Esperando para el ACK\n");
        time(&start);
        while(!mySwitch.available()){
            time(&end);
            seconds_elapsed = difftime(end, start); //If passed 2 seconds anything has been received, decode the chain
            if(seconds_elapsed >=2){
                printf("No se ha recibido el ACK, se considera la secuencia recibida valida y se pasa a su decodificacion\n");
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
    return(0);
}
