#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(D5,D4,D3,D2,D1,D0);



const int Sensor_Pin = A0;
unsigned int Sensitivity = 100;   // 185mV/A for 5A, 100 mV/A for 20A and 66mV/A for 30A Module
int theft=0;
float Vpp = 0; // peak-peak voltage
float Vrms = 0; // rms voltage
float Irms = 0; // rms current
float Supply_Voltage = 233.0;           // reading from DMM
float Vcc = 5.0;         // ADC reference voltage // voltage at 5V pin
float power = 0;         // power in watt              
float Wh =0 ;             // Energy in kWh
unsigned long last_time =0;
unsigned long current_time =0;
unsigned long interval = 100;
unsigned int calibration = 100;  // V2 slider calibrates this
unsigned int pF = 85;           // Power Factor default 95
unsigned int bill_amount = 0;   // 30 day cost as present energy usage incl approx PF
unsigned int energyTariff = 8.0; // Energy cost in INR per unit (kWh)
unsigned int A=0;

int connection=D6;
int buzzer=D7;//relay

unsigned long counterChannelNumber = 2022806;                // Channel ID
const char * myCounterReadAPIKey = "BZD5FH1BIEWL3KKP";      // Read API Key
const int FieldNumber1 = 6;                                 // The field you wish to read //TEST WAS DONE ON FIELD 3CHANGE TO FIELD 4
//const int FieldNumber2 = 2;                                 // The field you wish to read
 
//String apiKey = "AAAAAAAAAAAAAA";     //  Enter  API key from ThingSpeak
 
const char *ssid =  "C15";     // replace with your wifi ssid and wpa2 key
const char *pass =  "12345678";
const char* server = "api.thingspeak.com";
 

WiFiClient client;

void getACS712() {  // for AC
  Vpp = getVPP();
  Vrms = (Vpp/2.0) *0.707;
  Vrms = Vrms - (calibration / 10000.0);     // calibtrate to zero with slider
  Irms = (Vrms * 1000)/Sensitivity ;
  if((Irms > -0.015) && (Irms < 0.008)){  // remove low end chatter
    Irms = 0.0;
  }
  power= (Supply_Voltage * Irms) * (pF / 100.0);
  last_time = current_time;
  current_time = millis();    
  Wh = Wh+  power *(( current_time -last_time) /3600000.0) ; // calculating energy in Watt-Hour
  bill_amount = Wh * energyTariff;
  Serial.print("voltage:  ");
  Serial.print(String(Supply_Voltage, 3));
  Serial.println(" V");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(String(Supply_Voltage, 3));
  lcd.println(" V");
 
  Serial.print("Irms:  ");
  Serial.print(String(Irms, 3));
  Serial.println(" A");
  lcd.setCursor(7, 0);
  lcd.print(" I:");
  lcd.print(String(Irms, 3));
  lcd.println(" A");
 
 }
  Serial.print("  Bill Amount: INR");
  Serial.println(String(bill_amount, 2));
  lcd.setCursor(7, 1);
  lcd.print(" Bal:");
  lcd.print(bill_amount);
  lcd.print("     ");
 
}

float getVPP()
{
  float result;
  int readValue;                
  int maxValue = 0;            
  int minValue = 1024;          
  uint32_t start_time = millis();
  while((millis()-start_time) < 950) //read every 0.95 Sec
  {
     readValue = analogRead(Sensor_Pin);    
     if (readValue > maxValue)
     {        
         maxValue = readValue;
     }
     if (readValue < minValue)
     {          
         minValue = readValue;
     }
  }
   result = ((maxValue - minValue) * Vcc) / 1024.0;  
   return result;
 }    

 
 
void setup()
{
     
       Serial.begin(115200);
       lcd.begin(16, 2);
       pinMode(Sensor_Pin,INPUT);
       pinMode(connection,OUTPUT);
       pinMode(buzzer,OUTPUT);
       lcd.setCursor(0, 0);
       lcd.print("prepaid meter");
       lcd.setCursor(0, 1);
       lcd.print("System");
       delay(10);
     
 
       Serial.println("Connecting to ");
       Serial.println(ssid);
 
 
       WiFi.begin(ssid, pass);
 
      while (WiFi.status() != WL_CONNECTED)
     {
            delay(500);
            Serial.print(".");
     }
      Serial.println("");
      Serial.println("WiFi connected");
      ThingSpeak.begin(client);
 
}
 
void loop()
{
 delay(1000);
   A = ThingSpeak.readLongField(counterChannelNumber, FieldNumber1, myCounterReadAPIKey);
 //Serial.println("OK");
 
 Serial.println(A);
 getACS712();
 unsigned int amount=0;
    amount = A - bill_amount;
  lcd.setCursor(7, 1);
  lcd.print(" BAL:");
  lcd.print(amount);
   lcd.print("               ");
     delay(5000);
 
                         if (client.connect(server,80))   //   "184.106.153.149" or api.thingspeak.com
                      {  
                           
                             String postStr = apiKey;
                             postStr +="&field1=";
                             postStr += String(Supply_Voltage);
                             postStr +="&field2=";
                             postStr += String(Irms);
                             postStr +="&field3=";
                             postStr += String(power);
                             postStr +="&field4=";
                             postStr += String(bill_amount);
                             postStr +="&field5=";
                             postStr += String(Wh);
                             postStr +="&field6=";
                             postStr += String(amount);
                             postStr +="&field7=";
                             postStr += String(theft);
                             

                             
                             postStr += "\r\n\r\n";
 
                             client.print("POST /update HTTP/1.1\n");
                             client.print("Host: api.thingspeak.com\n");
                             client.print("Connection: close\n");
                             client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
                             client.print("Content-Type: application/x-www-form-urlencoded\n");
                             client.print("Content-Length: ");
                             client.print(postStr.length());
                             client.print("\n\n");
                             client.print(postStr);
                             Serial.println("%. Send to Thingspeak.");
                        }
          client.stop();
          delay(1000);
           if(amount<=10)
    {
      digitalWrite(connection,LOW);
     digitalWrite(buzzer,HIGH);
      delay(1000);
     digitalWrite(buzzer,LOW);
    }
     else
    {
     digitalWrite(connection,HIGH);  
    }

    while(amount<=10)
     {  

       A = ThingSpeak.readLongField(counterChannelNumber, FieldNumber1, myCounterReadAPIKey);
       if(A>19)
       {
        break;
       }
       
     }
          Serial.println("Waiting...");
 
  // thingspeak needs minimum 1 sec delay between updates
  delay(15000);

    }