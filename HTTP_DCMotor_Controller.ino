#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>
#include <Servo.h>

const int motor_enable_pin = 5; // PIN ATTIVAZIONE/DISATTIVAZIONE MOTORE DC
const int motor_direction_pin1 = 6; // PIN DIREZIONE MOTORE DC - A
const int motor_direction_pin2 = 2; // PIN DIREZIONE MOTORE DC - B
const int servo_pin = 3; // PIN COMUNICAZIONE MOTORE SERVO

byte mac_address[] = {0x90, 0xA2, 0xDA, 0x0F, 0x9D, 0x66}; // INDIRIZZO MAC ETHERNET SHIELD ARDUINO (posto sotto la scheda)
// INDIRIZZO IP ETHERNET SHIELD ARDUINO (statico)
//byte ip_address[] = {192, 168, 0, 177}; // TERAMO
byte ip_address[] = {192, 168, 1, 177}; // L'AQUILA
// INDIRIZZO GATEWAY
// byte gateway[] = {192, 168, 0, 1}; // TERAMO
byte gateway[] = {192, 168, 1, 254}; // L'AQUILA
// INDIRIZZO SUBNET MASK
byte subnet_mask[] = {255, 255, 255, 0}; 

EthernetServer server(8081); // ISTANZA SERVER IN ASCOLTO SU PORTA 8081

int motor_rotation_speed; // VELOCITA' MOTORE DC
int motor_rotation_speed_index; // ?

Servo servo; // ISTANZA MOTORE SERVO
int servo_rotation_angle; // ANGOLO DI ROTAZIONE MOTORE SERVO
int servo_rotation_angle_index;

void setup(){
  Serial.begin(9600); // ATTIVAZIONE COMUNICAZIONE SERIALE (DEBUG ONLY)
  pinMode(motor_direction_pin1, OUTPUT); // ATTIVAZIONE OUTPUT PIN MOTORE DC 
  pinMode(motor_direction_pin2, OUTPUT); // ATTIVAZIONE OUTPUT PIN MOTORE DC
  pinMode(motor_enable_pin, OUTPUT); // ATTIVAZIONE OUTPUT PIN MOTORE DC
  digitalWrite(motor_enable_pin, LOW); // MOTORE DC - INIZIALMENTE DISATTIVO
  if(!SD.begin(4)){ 
    return;
  }
  servo.attach(servo_pin); // ATTIVAZIONE SERVO 
  /* Per utilizzare richiesta DHCP --> Ethernet.begin(mac_address); */
  Ethernet.begin(mac_address, ip_address, gateway, subnet_mask); // ATTIVAZIONE COMUNICAZIONE ETHERNET - NO DHCP 
  server.begin(); // ATTIVAZIONE SERVER 
}

void loop(){
  EthernetClient client = server.available(); // ISTANZA DI CLIENT
  if(client){ // CLIENT PRESENTE 
    String http_request; // RICHIESTA HTTP
    while(client.connected()){ // FINTANTO CHE CLIENT CONNESSO
      if(client.available()){ // SE DISPONIBILE
        char c = client.read(); // LETTURA RICHIESTA HTTP - UN CARATTERE PER ITERAZIONE
        Serial.print(c);
        http_request += c; // CONCATENO CARATTERE LETTO A QUANTO LETTO IN PRECEDENZA
        if(c == '\n'){ // SE SONO GIUNTO AL TERMINE DELLA RIGA (accetto solo GET)
          if(!http_request.startsWith("GET")){ // SE RICHIESTA NON DI TIPO GET - 501 NOT IMPLEMENTED 
            client.println("HTTP/1.1 501 Not Implemented");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<html><head><title>Error 501 Not Implemented</title></head><body><h1>Error 501 Not Implemented</h1></body></html>");
            break;
          } else { 
            motor_rotation_speed_index = http_request.indexOf("motor_rotation_speed"); 
            servo_rotation_angle_index = http_request.indexOf("servo_rotation_angle");
            if(http_request.indexOf(" /?") > 0){ // PARAMETRI 
              if(motor_rotation_speed_index > 0 && servo_rotation_angle_index > 0 && servo_rotation_angle_index > motor_rotation_speed_index){ // CONTROLLO VALIDITA POSIZIONE/PRESENZA PARAMETRI 
                if(http_request.indexOf("&", servo_rotation_angle_index) == -1){ // HO I SOLI DUE PARAMETRI RICHIESTI
                  motor_rotation_speed = http_request.substring(motor_rotation_speed_index + 21, http_request.indexOf(servo_rotation_angle_index - 1, motor_rotation_speed_index)).toInt();  
                  servo_rotation_angle = http_request.substring(servo_rotation_angle_index + 21, http_request.indexOf(" ", servo_rotation_angle_index + 1)).toInt();
                } else { // HO ALTRI PARAMETRI NON ATTESI - 400 BAD REQUEST
                  client.println("HTTP/1.1 400 Bad Request");
                  client.println("Content-Type: text/html");
                  client.println();
                  client.println("<html><head><title>Error 400 Bad Request</title></head><body><h1>Error 400 Bad Request</h1></body></html>");
                  break;
                }
              } else { // PARAMETRI CON DIFFERENTE POSIZIONE/PRESENZA - 400 BAD REQUEST
                client.println("HTTP/1.1 400 Bad Request");
                client.println("Content-Type: text/html");
                client.println();
                client.println("<html><head><title>Error 400 Bad Request</title></head><body><h1>Error 400 Bad Request</h1></body></html>");
                break;
              }
            } else if(http_request.indexOf(" / ") < 0){ // RISORSA RICHIESTA NON PREVISTA - 404 NOT FOUND
              client.println("HTTP/1.1 404 Not Found");
              client.println("Content-Type: text/html");
              client.println();
              client.println("<html><head><title>Error 404 Not Found</title></head><body><h1>Error 404 Not Found</h1></body></html>");
              break;
            }
            /* SE SONO GIUNTO FIN QUI - 200 OK */
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
            client.print(F("<html><head><title>Arduino Control Over HTTP</title></head><body><fieldset><h3>Arduino Remote Control - Lorenzo Addazi</h3></legend><br><form method=\"GET\"><label>Motor Rotation Speed</label><span> </span><input type=\"number\" min=\"-255\" max=\"255\" value=\""));
            client.print(motor_rotation_speed);
            client.println("\" name=\"motor_rotation_speed\" required><br><br>");
            client.print(F("<label>Servo Rotation Angle</label><span> </span><input type=\"number\" min=\"0\" max=\"180\" value=\""));
            client.print(servo_rotation_angle);
            client.println(F("\" name=\"servo_rotation_angle\" required><br><br><input type=\"submit\"></form></fieldset></body></html>"));
            break;
          }
        } 
      }
    }
    http_request = ""; // RIPRISTINO CONTENUTO STRINGA RICHIESTA HTTP 
    delay(1); // ATTENDO CHE LA TRASMISSIONE SI CONCLUDA (temporizzazione necessaria affinchè la trasmissione vada a buon fine)
    client.stop(); // CHIUDO LA COMUNICAZIONE CON CLIENT 
    Serial.println(motor_rotation_speed);
    Serial.println(servo_rotation_angle);
    if(servo_rotation_angle >= 170){ // VALORE ANGOLAZIONE ILLEGALE - MASSIMO CONSENTITO 
      servo.write(170); 
    } else if(servo_rotation_angle <= 0){ // VALORE ANGOLAZIONE ILLEGALE - MINIMO CONSENTITO
      servo.write(0);
    } else { // APPLICO VALORE RICEVUTO
      servo.write(servo_rotation_angle);
    }
    if(motor_rotation_speed < 0){ // DIREZIONE SINISTRA - VALORE NEGATIVO RICEVUTO 
      digitalWrite(motor_direction_pin1, HIGH); // DIREZIONE A - HIGH VOLTAGE
      digitalWrite(motor_direction_pin2, LOW); // DIREZIONE B - LOW VOLTAGE
      if(motor_rotation_speed < -255){ // VALORE ILLEGALE - SOSTITUISCO CON MASSIMO AMMESSO 
        analogWrite(motor_enable_pin, 255); 
      } else { // VALORE LEGALE - APPLICO A MOTORE
        analogWrite(motor_enable_pin, motor_rotation_speed * -1); 
      }
    } else if(motor_rotation_speed > 0){ // DIREZIONE DESTRA - VALORE POSITIVO RICEVUTO 
      digitalWrite(motor_direction_pin1, LOW); // DIREZIONE A - LOW VOLTAGE
      digitalWrite(motor_direction_pin2, HIGH); // DIREZIONE B - HIGH VOLTAGE
      if(motor_rotation_speed > 255){ // VALORE ILLEGALE - SOSTITUISCO CON MASSIMO AMMESSO
        analogWrite(motor_enable_pin, 255);
      } else { // APPLICO VALORE VELOCITA'
        analogWrite(motor_enable_pin, motor_rotation_speed);
      }
    } else { // MOTORE DC DISATTIVATO
      digitalWrite(motor_direction_pin1, LOW);
      digitalWrite(motor_direction_pin2, LOW);
      analogWrite(motor_enable_pin, 0);
    }
  }
}


