
/*   
 *   ** RECEBENDO MENSAGEM DO BROKER MQTT
 *   POR: JONNY CHRISTIAN
 *   EM 28/05/2018
 *    
 */

/*
Equivalencia das saidas Digitais entre NodeMCU e ESP8266 (na IDE do Arduino)
NodeMCU - ESP8266
D0 = 16;
D1 = 5;
D2 = 4;
D3 = 0;
D4 = 2;
D5 = 14;
D6 = 12;
D7 = 13;
D8 = 15;
D9 = 3;
D10 = 1;
*/

#include <ESP8266WiFi.h>        //https://github.com/esp8266/Arduino
#include <PubSubClient.h>

//Coloque os valores padrões aqui, porém na interface eles poderão ser substituídos.
#define servidor_mqtt             "m11.cloudmqtt.com"  //URL do servidor MQTT
#define servidor_mqtt_porta       16633  //Porta do servidor (a mesma deve ser informada na variável abaixo)
#define servidor_mqtt_usuario     "bljascsn"  //Usuário
#define servidor_mqtt_senha       "3h4SwSgfbvyh"  //Senha
#define mqtt_topico_pub           "esp8266/pincmd"    //Tópico para publicar o comando de inverter o pino do outro ESP8266

//Declaração do pino que será utilizado
#define pinoVibrador                         4                  //Pino que será lido e caso seja alterado seu status, deverá ser enviado um comando para o outro ESP.
#define pinoBotao                            5
long tempoEsperaEntrePressao = 0; //variavel para auxiliar no tempo de espera entre o botao ser pressionado
long tempoUltimaMsg = 0;
long tempoEsperaEntreMsg = 180000; //3 minutos de espera entre msgs
int pressao = 0; //variavel que vai contar quantas vezes o botao foi pressionado

WiFiClient wifiClient;                                 //Instância do WiFiClient
PubSubClient MQTT(wifiClient);                       //Passando a instância do WiFiClient para a instância do PubSubClient

const char* ssid = "*********"; //nome da rede wifi
const char* password = "********"; //senha da rede wifi


void conectarWiFi()
{
  if(WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  Serial.print("Conectando a rede:");
  Serial.println(ssid);
  Serial.println("Aguarde!");

  WiFi.begin(ssid, password); //função que conecta na rede wifi passando os dados como parametro
  while(WiFi.status() != WL_CONNECTED)
  {
    vibrarEmPausas();
    Serial.print("...");
  }

  Serial.print("Conectado com sucesso na rede:");
  Serial. println(ssid);
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());
}


//Função que reconecta ao servidor MQTT
void conectarMQTT() {
  //Repete até conectar
  while (!MQTT.connected()) {
    Serial.println("Tentando conectar ao servidor MQTT...");
    
    //Tentativa de conectar. Se o MQTT precisa de autenticação, será chamada a função com autenticação, caso contrário, chama a sem autenticação. 
    bool conectado = strlen(servidor_mqtt_usuario) > 0 ?
                     MQTT.connect("ESP8266receptor", servidor_mqtt_usuario, servidor_mqtt_senha) :
                     MQTT.connect("ESP8266receptor");

    if(conectado) {
      Serial.println("Conectado!");
      MQTT.subscribe(mqtt_topico_pub, 1);                   //subscrible para monitorar os comandos recebidos
    } else {
      Serial.println("Falhou ao tentar conectar. Codigo: ");
      Serial.println(String(MQTT.state()).c_str());
      Serial.println(" tentando novamente em 5 segundos");
      //Aguarda 5 segundos para tentar novamente
      vibrarEmPausas();
    }
  }
}

void mantemConexoes()
{
  if(!MQTT.connected())
  {
    conectarMQTT();    
  }
  conectarWiFi(); //refaz a conexão com wifi
}

void recebeMensagem(char* topic, byte* mensagem, unsigned int tamanho)
{

   String msg;

    //obtem a string do playload recebido
    for(int i = 0; i<tamanho; i++)
    {
     char c = (char)mensagem[i];
     msg += c;      
    }
    if (msg == "vibrar")
    {
     digitalWrite(pinoVibrador, HIGH);
     Serial.println("Recebido mensagem vibrar");
     tempoUltimaMsg = millis(); //atualiza o tempo da ultima msg recebida 
    }
    if(msg == "test")
    {
      tempoUltimaMsg = millis(); //atualiza o tempo da ultima msg recebida 
    }
}

void vibrarEmPausas()
{
      digitalWrite(pinoVibrador, HIGH);
      delay(2500);
      digitalWrite(pinoVibrador, LOW);
      delay(2500);
}

//funcao que vai ajudar a decidir quando vibrar
void executaAcao()
{//só vai parar de vibrar quando botao for pressionado duas vezes seguidas
  switch(pressao)
  {
    case 2:
      digitalWrite(pinoVibrador, LOW);
    break;
  }
}

//Função inicial (será executado SOMENTE quando ligar o ESP)
void setup() 
{
  Serial.begin(115200);

  //Fazendo o pino ser de entrada.
  pinMode(pinoVibrador, OUTPUT);
  pinMode(pinoBotao, INPUT);

  conectarWiFi();
  MQTT.setServer(servidor_mqtt, servidor_mqtt_porta);
  MQTT.setCallback(recebeMensagem);
}

void loop() 
{
  mantemConexoes();
  MQTT.loop();

  //se o vibrador estiver vibrando
   if((digitalRead(pinoVibrador)) == HIGH)
   {
    //verificar que usuario pressionou botao
      if(digitalRead(pinoBotao))
      {
        tempoEsperaEntrePressao = millis();
        delay(400);
        pressao++;    
        Serial.println(pressao);  
      }
      if(pressao !=0 && ((millis() - tempoEsperaEntrePressao) > 1000))
      {
        executaAcao(); //funcao que decide o que fazer quando o botao é pressionado
        pressao = 0;
      }
  }

  if((millis() - tempoUltimaMsg) > tempoEsperaEntreMsg)
  {
    vibrarEmPausas();
  }
}
