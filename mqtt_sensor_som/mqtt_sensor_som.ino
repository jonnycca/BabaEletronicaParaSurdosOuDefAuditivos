/*   
 *   ** SENSOR DE SOM ENVIANDO MENSAGEM VIA WIFI PARA BROKER MQTT
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

#include <ESP8266WiFi.h>        //biblioteca necessária para a conexão com a rede wifi
#include <PubSubClient.h>       //biblioteca necessária para utilizar protocolo MQTT


#define servidor_mqtt             "m11.cloudmqtt.com"  //URL do servidor MQTT
#define servidor_mqtt_porta       16633  //Porta do servidor
#define servidor_mqtt_usuario     "bljascsn"  //Usuário
#define servidor_mqtt_senha       "3h4SwSgfbvyh"  //Senha
#define mqtt_topico_pub           "esp8266/pincmd"    //Tópico para publicar o comando

//Declaração do pino que será utilizado
#define sensorSom                      5                  //Pino que será lido para verificar som do ambiente
int valorSom = 0;                                    //Variável que receberá o valor do som do ambiente
long tempoEnvioUltimaMsg = 0;                       //variavel que vai pegar o tempo da ultima msg enviada
long tempoEsperaEntreMsg = 120000;                  //2 minutos de espera entre as msgs


WiFiClient wifiClient;                               //Instância do WiFiClient
PubSubClient MQTT(wifiClient);                       //Passando a instância do WiFiClient para a instância do PubSubClient

const char* ssid = "********";                  //nome da rede wifi
const char* password = "*********";                //senha da rede wifi


//função para conectar a rede wifi
void conectarWiFi()
{
  if(WiFi.status() == WL_CONNECTED)                  //verifica se o status é conectado, se for, retorna a conexão
  {
    return;
  }

  Serial.print("Conectando a rede:");                //mostrando a rede que está tentando conectar
  Serial.print(ssid);
  Serial.println("Aguarde!");

  WiFi.begin(ssid, password);                        //função que conecta na rede wifi passando os dados como parametro
  while(WiFi.status() != WL_CONNECTED)               //enquanto o status de conexão for diferente de conectado, continuará tentando
  {
    delay(1000);                                     //espera 1 segundo
    Serial.print("...");
  }

  Serial.print("Conectado com sucesso na rede:");    //mostrando a rede que foi conectado
  Serial. println(ssid);
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());                    //mostrando o ip obtido
}


//Função que conectar ao servidor MQTT
void conectarMQTT() 
{

  while (!MQTT.connected())                         //Enquanto não conectado, continuará tentando
  {
    Serial.println("Tentando conectar ao servidor MQTT...");
    
    //Tentativa de conectar. Se o MQTT precisa de autenticação, será chamada a função com autenticação, caso contrário, chama a sem autenticação. 
    bool conectado = strlen(servidor_mqtt_usuario) > 0 ?
                     MQTT.connect("ESP8266publicador", servidor_mqtt_usuario, servidor_mqtt_senha) :
                     MQTT.connect("ESP8266publicador");

    if(conectado)                                   //conectou, retorna conectado
    {
      Serial.println("Conectado!");
    } else {                                        //Se não conectou, retorna mensagem de erro
      Serial.println("Falhou ao tentar conectar. Codigo: ");
      Serial.println(String(MQTT.state()).c_str());
      Serial.println(" tentando novamente em 5 segundos");
      //Aguarda 5 segundos para tentar novamente
      delay(5000);
    }
  }
}

//função para desconectar do servidor MQTT
void desconectar()
{
  Serial.println("Fechando conexao com servidor MQTT");
  MQTT.disconnect();                                 //Função para desconectar do mqtt
}

//função para manter conexão com MQTT e com WIFI
void mantemConexoes()
{
  if(!MQTT.connected())                              //Se esta desconectado do MQTT, tenta conectar 
  {
    conectarMQTT();                                  //Função para se conectar ao mqtt
  }
  conectarWiFi();                                    //refaz a conexão com wifi
}

//Função que publica mensagem
void publicaMensagem() 
{
  if (!MQTT.connected()) {                           //verifica se está conectado com mqtt
    Serial.println("MQTT desconectado! Tentando reconectar para fazer publicação...");
    conectarMQTT();                                  //tenta conectar ao mqtt se não estiver conectado
  }
  
  //funcao que trata de organizar as mensagens em fila 
  MQTT.loop();

  //verificando estado do sensor, não precisa se a verificação for feita na função loop
 // if(valorSom == HIGH)//verifica se detectou som
 // {
     
    //Publicando no MQTT
    Serial.println("Fazendo a publicacao...");
   MQTT.publish(mqtt_topico_pub, "vibrar");//publica vibrar
   Serial.println("publicado!");
  /* delay(15000);//teste de publicação depois de 15 seg
   Serial.println("Passou 15 sec!");
   MQTT.publish(mqtt_topico_pub, "15sec");
   Serial.println("Publicado 15 sec");
*/   valorSom = LOW;
 // }
}

//Função que publica mensagem de teste
void publicaMensagemTeste() 
{
  if (!MQTT.connected()) {                           //verifica se está conectado com mqtt
    Serial.println("MQTT desconectado! Tentando reconectar para fazer publicação...");
    conectarMQTT();                                  //tenta conectar ao mqtt se não estiver conectado
  }
  
  //funcao que trata de organizar as mensagens em fila 
  MQTT.loop();
   
    //Publicando no MQTT
    Serial.println("Fazendo a publicacao de teste...");
   MQTT.publish(mqtt_topico_pub, "test");//publica vibrar
   Serial.println("publicado teste!");
}


//Função inicial (será executado SOMENTE quando ligar o ESP)
void setup() 
{
  Serial.begin(115200);

  //Fazendo o pino ser de entrada.
  pinMode(sensorSom, INPUT);

  conectarWiFi();                                    //conectar a rede wifi
  MQTT.setServer(servidor_mqtt, servidor_mqtt_porta);//passando informações sobre o broker mqtt
}

void loop() 
{
  valorSom = digitalRead(sensorSom);
  if(valorSom == HIGH)
  {
    Serial.println("detectou som");
    publicaMensagem();
    tempoEnvioUltimaMsg = millis();
    desconectar();
    MQTT.loop(); 
    delay(20000);
  }
  // se o tempo atual menos o tempo da ultima mensagem for maior que 2 minutos, faz uma publicação de text
  if((millis() - tempoEnvioUltimaMsg) > tempoEsperaEntreMsg)
  {
    publicaMensagemTeste();
    desconectar();
    MQTT.loop(); 
    tempoEnvioUltimaMsg = millis();
  }
}
