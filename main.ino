//Bibliotecas
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <tuple>
#include <iostream>
using namespace std;

//Constantes de tempo de atraso
int delaytime1 = 2000;
int delaytime2 = 20000; //3 dados por minuto
int delaytime3 = 500;
int delaytime4 = 15000; //4 dados por minuto
int period = 5000;
int radius = 147;
const float pi = 3.14159265;

//WiFi
String apiKey = "insira a sua key do ThingSpeak aqui";
const char* ssid = "insira o seu ssid Wi-Fi aqui";
const char* password = "insira a senha do seu roteador aqui";
const char* server = "api.thingspeak.com";
WiFiClient client;

//DHT11 
#define DHTPIN D5
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

//BMP280
#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)
Adafruit_BMP280 bmp;

//Sensor de Efeito Hall
#define Hall sensor D3
unsigned int Sample = 0;
unsigned int counter = 0;
unsigned int RPM = 0;
float speedwind = 0;
float windspeed = 0;
int ar = 0;
int wd = 0;
int wds = 0;

//Inicializadores
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Inicializando a Central Meteorológica!");
  Serial.println();
  delay(10);

  wifi_begin();
  
  dht.begin(); //DHT11

  bmp.begin(0x76); //BMP280
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                    Adafruit_BMP280::SAMPLING_X2,     
                    Adafruit_BMP280::SAMPLING_X16,    
                    Adafruit_BMP280::FILTER_X16,      
                    Adafruit_BMP280::STANDBY_MS_500); 

  pinMode(D3, INPUT); //Sensor de Efeito Hall
  digitalWrite(D3, HIGH);
}

void wifi_begin(){
  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(delaytime3);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
}

tuple<float, float> DHT_read(){
  float dhth = dht.readHumidity();
  float dhtt = dht.readTemperature();
  
  if (isnan(dhth) || isnan(dhtt)) 
  {
    Serial.println("Falha ao interpretar os dados do sensor DHT!");
    return {0, 0};
  }
  
  return {dhth, dhtt};
}

tuple<float, float> BMP_read(){  
  float bmpt = bmp.readTemperature();
  float bmpp = bmp.readPressure()/100;
  float bmpa = bmp.readAltitude(1019.66); //1019.66 é a pressão ao nível do mar na cidade onde o sistema foi desenvolvido
  float bmpaa = bmpa/3.281; //Passando o valor da altitude de pressão de pés 
  para metros
  
  if (isnan(bmpt) || isnan(bmpp) || isnan(bmpa))
  {
    Serial.println("Falha ao interpretar os dados do sensor BMP!");
    return {0, 0, 0};
  }
  
  return {bmpp, bmpaa};
}

float ANEM_read(){
  windvelocity();
  RPMcalc();
  WindSpeed(); //m*s^-1  
  SpeedWind(); //km*h^-1 
  delay(delaytime1);   
  windir = winddir();  //Direção do vento
  
  return windir;
}

void windvelocity(){
	speedwind = 0;
  windspeed = 0;
  
  counter = 0;
  attachInterrupt(0, addcount, RISING);
  unsigned long millis();
  long startTime= millis();
  while(millis() < startTime + period){ 
  }
}

void RPMcalc(){
  RPM = ((counter) * 60)/(period/1000);
}

void WindSpeed(){
  windspeed = ((4 * pi * radius * RPM)/60) / 100; //m*s^-1
}

float SpeedWind(){
  speedwind = (((4 * pi * radius * RPM)/60)/100)*3.6; //km*h^-1
}

void addcount(){
  counter++;
}

tuple<float> winddir(){
  for (int i = 0; i < 20; i++){
    wd = analogRead(0); //Conexão A0 ESP8266-12E
    Serial.println(wd);
    wds=wds+wd;
    delay(50);
  }
  ar = wds / 20;
  float wdir = 0;
  if(ar >= 0 && ar <=64 )  { 
      wdir = 315;
      }
    if (ar >= 65  && ar <= 100) {
      wdir = 270;
      }
    if (ar >= 101  && ar <=200) {
      wdir = 225;
      }
    if (ar >= 201 && ar <= 300) {
      wdir = 180;       
      }
    if (ar >= 301 && ar <= 400){ 
      wdir = 135;                                                                                                                                                                     
      }
    if (ar >= 401 && ar <= 480) {                                                                                                                                                                                                                                       
      wdir = 90;
     }
    if (ar >= 481 && ar <= 580) {
      wdir = 45;
     }
    if (ar >= 581 && ar <= 699 ) {                                  
     wdir= 0;
  }
  ar = 0;
  wd = 0;
  wds = 0;
  
  return wdir;
}

tuple<float> dewpoint(){
  float celsiustemp = dht.readTemperature();
  float humid = dht.readHumidity();
  float dewp = (celsiustemp - ((100-humid)/5));
  return dewp;
}

void share_info(float dhthb, float dhttb, float bmppb, float bmpaab, float SpeedWind, float windirb, float dewpoint){
  float dhth = dhthb;		
  float dhtt = dhttb;
  float bmpp = bmppb; 
  float bmpaa = bmpaab;
  float windir = windirb;

  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr +="&field1=";
    postStr += String(dhtt);
    postStr +="&field2=";
    postStr += String(dhth);
    postStr += "&field3=";
    postStr += String(bmpp);
    postStr += "&field4=";
    postStr += String(bmpaa);
    postStr += "&field5=";
    postStr += String(speedwind);
    postStr += "&field6=";
    postStr += String(wdir);
    postStr += "&field7="
    postStr += String(dewp);
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

    Serial.print("Temperatura do DHT: "); //Impressão dos dados no monitor serial para fins de acompanhamento
    Serial.print(dhtt);
    Serial.print(" ºCelsius, Umidade Relativa do DHT: ");
    Serial.print(dhth);
    Serial.print("Temperatura do BMP: ");
    Serial.print(bmpt);
    Serial.print(" ºCelsius, Pressão Atmosférica do DHT: ");
    Serial.print(bmpp);
    Serial.print(" hPa, Altitude de Pressão do BMP: ");
    Serial.print(bmpaa);
    Serial.print(" metros.");
    Serial.println("Enviando os dados para o servidor do ThingsSpeak!");
  }

  client.stop();
}

//Ações em repetição
void loop() {
  auto [dhth, dhtt] = DHT_read();
  auto [bmpt, bmpp, bmpa, bmpaa] = BMP_read();
  auto [windir] = ANEM_read();
	auto [dewp] = dewpoint();
  share_info(dhth, dhtt, bmpp, bmpaa, windir, dewp);
  
  Serial.println("Atualizando em 20 segundos...");
  delay(delaytime2);
}
