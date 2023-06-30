#include "LoRaWan_APP.h"
#include "DHT.h"
#include "Adafruit_TCS34725.h"
#include<Wire.h>//Biblioteca para comunicação I2C

const int MPU_addr=0x68; //Endereço do sensor Accel
int16_t AcX,AcY,AcZ; //Variaveis para pegar os valores medidos
uint16_t r, g, b, c, lux;
const uint8_t pinoLDR = 12;
const uint8_t pinoDHT = 23;
int valorLDR = 0;
float temperatura = 0;
uint8_t umidade = 0;
uint8_t temp_x100_msb, temp_x100_lsb, umid_x100_msb, umid_x100_lsb,  lux_msb, lux_lsb, AcX_msb, AcX_lsb, AcY_msb, AcY_lsb, AcZ_msb, AcZ_lsb;
int ChannelLux = 0, ChannelUmid = 1, ChannelTemp = 2, ChannelAc = 3;
int TypeLux = 101, TypeUmid = 104, TypeTemp = 103, TypeAc = 113;
const int pinoLuz = 25;

DHT dht(pinoDHT, DHT11);

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);


/* OTAA para*/
uint8_t devEui[] = { 0x9f, 0xfc, 0x77, 0x6f, 0x2d, 0x54, 0xe3, 0x61 };
uint8_t appEui[] = { 0x26, 0x78, 0xf2, 0x1f, 0x93, 0x92, 0x0c, 0x59, 0x53, 0x89, 0x97, 0xd3, 0x7c, 0x0b, 0xfd, 0x4f };
uint8_t appKey[] = { 0xec, 0x81, 0xf4, 0x32, 0x0f, 0xb3, 0xdd, 0x35, 0xa1, 0x32, 0x25, 0x0e, 0x10, 0x53, 0x3f, 0xc9 };

/* ABP para*/
uint8_t nwkSKey[] = { 0xdd, 0x3e, 0xd7, 0xe8, 0x58, 0x97, 0xe3, 0x91, 0x7a, 0xf0, 0xd8, 0xc7, 0xef, 0x52, 0xfc, 0x6f };
uint8_t appSKey[] = { 0x26, 0x78, 0xf2, 0x1f, 0x93, 0x92, 0x0c, 0x59, 0x53, 0x89, 0x97, 0xd3, 0x7c, 0x0b, 0xfd, 0x4f };
uint32_t devAddr = (uint32_t) 0x0193a956;


/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };  // Sub banda 1

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 300;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 4;

//Variáveis que indicam o núcleo
static uint8_t taskCoreZero = 0;
static uint8_t taskCoreOne = 1;

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port) {
    /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
    *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
    *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
    *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
    *for example, if use REGION_CN470, 
    *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
    */
    appDataSize = 19;
    appData[0] = (uint8_t) ChannelTemp;
    appData[1] = (uint8_t) TypeTemp;
    appData[2] = temp_x100_msb;
    appData[3] = temp_x100_lsb;
    appData[4] = (uint8_t) ChannelUmid;
    appData[5] = (uint8_t) TypeUmid;
    appData[6] = umidade * 2;
    appData[7] = (uint8_t) ChannelLux;
    appData[8] = (uint8_t) TypeLux;
    appData[9] = lux_msb;
    appData[10] = lux_lsb;
    appData[11] = (uint8_t) ChannelAc;
    appData[12] = (uint8_t) TypeAc;
    appData[13] = AcX_msb;
    appData[14] = AcX_lsb;
    appData[15] = AcY_msb;
    appData[16] = AcY_lsb;
    appData[17] = AcZ_msb;
    appData[18] = AcZ_lsb;
}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ

void setup() {
    Serial.begin(115200);

    Wire.begin(); //Inicia a comunicação I2C
    Wire.beginTransmission(MPU_addr); //Começa a transmissao de dados para o sensor
    Wire.write(0x6B); // registrador PWR_MGMT_1
    Wire.write(0); // Manda 0 e "acorda" o MPU 6050
    Wire.endTransmission(true);

    Mcu.begin();
    dht.begin();
    //Inicializamos nosso sensor DHT11
    tcs.begin();
    //Definimos o modo de saída
    pinMode(pinoLuz, OUTPUT);
    deviceState = DEVICE_STATE_INIT;

     xTaskCreatePinnedToCore(
                coreTaskZero, /* Função que implementa a tarefa do bluetooth */
                "TaskBle0", /* Nome da tarefa */
                10000, /* Numero de palavras a serem alocados para uso com a pilha da tarefa */
                NULL, /* parâmetro de entrada para a tarefa (pode ser NULL) */
                1, /* prioridade da tarefa (0 a N) */
                NULL, /* task handle referencia para a tareffa (pode ser NULL) */
                taskCoreZero); /* Núcleo que executará a tarefa */
   delay(500);

   xTaskCreatePinnedToCore(
                coreTaskFour, /* Função que implementa a tarefa do bluetooth */
                "TaskGPS", /* Nome da tarefa */
                10000, /* Numero de palavras a serem alocados para uso com a pilha da tarefa */
                NULL, /* parâmetro de entrada para a tarefa (pode ser NULL) */
                1, /* prioridade da tarefa (0 a N) */
                NULL, /* task handle referencia para a tareffa (pode ser NULL) */
                taskCoreOne); /* Núcleo que executará a tarefa */
   delay(500);
}

void coreTaskZero( void* pvParameters){

  String taskMessage = "Tarefa 0 executando no nucleo: ";
  taskMessage = taskMessage + xPortGetCoreID();
  Serial.println(taskMessage);

  Serial.begin(115200);

    Wire.begin(); //Inicia a comunicação I2C
    Wire.beginTransmission(MPU_addr); //Começa a transmissao de dados para o sensor
    Wire.write(0x6B); // registrador PWR_MGMT_1
    Wire.write(0); // Manda 0 e "acorda" o MPU 6050
    Wire.endTransmission(true);

    Mcu.begin();
    dht.begin();
    //Inicializamos nosso sensor DHT11
    tcs.begin();
    //Definimos o modo de saída
    deviceState = DEVICE_STATE_INIT;

    switch (deviceState) {
        case DEVICE_STATE_INIT:
        {
            #if (LORAWAN_DEVEUI_AUTO)
                LoRaWAN.generateDeveuiByChipID();
            #endif
            LoRaWAN.init(loraWanClass, loraWanRegion);
            break;
        }
        case DEVICE_STATE_JOIN:
        {
            LoRaWAN.join();
            break;
        }
        case DEVICE_STATE_SEND:
        {

            temperatura = dht.readTemperature();
            umidade = dht.readHumidity();
            uint16_t temp_x100 = temperatura * 10;
            temp_x100_msb = (uint8_t) ((temp_x100 & 0xFF00) >> 8);
            temp_x100_lsb = (uint8_t) ((temp_x100 & 0x00FF));
            uint8_t umid_x100 = umidade;

            tcs.getRawData(&r, &g, &b, &c);//Pega os valores "crus" do sensor referentes ao Vermelho(r), Verde(g), Azul(b) e da Claridade(c).
            lux = tcs.calculateLux(r, g, b);
            lux_msb = (uint8_t) ((lux & 0xFF00) >> 8);
            lux_lsb = (uint8_t) ((lux & 0x00FF));

            Wire.beginTransmission(MPU_addr); //Começa a transmissao de dados para o sensor
            Wire.write(0x3B); // registrador dos dados medidos (ACCEL_XOUT_H)
            Wire.endTransmission(false);
            Wire.requestFrom(MPU_addr,14,true); // faz um "pedido" para ler 14 registradores, que serão os registrados com os dados medidos
            AcX=Wire.read()<<8|Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
            AcY=Wire.read()<<8|Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
            AcZ=Wire.read()<<8|Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)          
            
            //Ajuste de variáveis de Acx para envio LoRa (uint8_t)
            AcX_msb = (uint8_t) ((AcX & 0xFF00) >> 8);
            AcX_lsb = (uint8_t) ((AcX & 0x00FF));

            //Ajuste de variáveis de Acy para envio LoRa (uint8_t)
            AcY_msb = (uint8_t) ((AcY & 0xFF00) >> 8);
            AcY_lsb = (uint8_t) ((AcY & 0x00FF));

            //Ajuste de variáveis de Acz para envio LoRa (uint8_t)
            AcZ_msb = (uint8_t) ((AcZ & 0xFF00) >> 8);
            AcZ_lsb = (uint8_t) ((AcZ & 0x00FF));
            
            prepareTxFrame(appPort);
            LoRaWAN.send();
            deviceState = DEVICE_STATE_CYCLE;
            break;
        }
        case DEVICE_STATE_CYCLE:
        {
            // Schedule next packet transmission
            txDutyCycleTime = appTxDutyCycle; /* + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND); */
            LoRaWAN.cycle(txDutyCycleTime);
            deviceState = DEVICE_STATE_SLEEP;
            break;
        }
        case DEVICE_STATE_SLEEP:
        {
            tcs.getRawData(&r, &g, &b, &c);//Pega os valores "crus" do sensor referentes ao Vermelho(r), Verde(g), Azul(b) e da Claridade(c).
            lux = tcs.calculateLux(r, g, b);
            if(lux < 20) digitalWrite(pinoLuz, HIGH);
            else digitalWrite(pinoLuz, LOW);
            LoRaWAN.sleep(loraWanClass);
            break;
        }
        default:
        {
            deviceState = DEVICE_STATE_INIT;
            break;
        }
      }
      vTaskDelay(500);
      
}

void coreTaskFour( void* pvParameters){

  String taskMessage = "Tarefa 0 executando no nucleo: ";
  taskMessage = taskMessage + xPortGetCoreID();
  Serial.println(taskMessage);

  Serial.begin(115200);

    Wire.begin(); //Inicia a comunicação I2C
    Wire.beginTransmission(MPU_addr); //Começa a transmissao de dados para o sensor
    Wire.write(0x6B); // registrador PWR_MGMT_1
    Wire.write(0); // Manda 0 e "acorda" o MPU 6050
    Wire.endTransmission(true);

    Mcu.begin();
    dht.begin();
    //Inicializamos nosso sensor DHT11
    tcs.begin();
    //Definimos o modo de saída

  tcs.getRawData(&r, &g, &b, &c);//Pega os valores "crus" do sensor referentes ao Vermelho(r), Verde(g), Azul(b) e da Claridade(c).
  lux = tcs.calculateLux(r, g, b);
  if(lux < 20) digitalWrite(pinoLuz, HIGH);
  else digitalWrite(pinoLuz, LOW);
  vTaskDelay(500);
}

void loop() {
}
