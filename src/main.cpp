#include <Arduino.h>

#define CUSTOM_SETTINGS
#define INCLUDE_SENSOR_MODULE
#define INCLUDE_NOTIFICATION_MODULE
#define INCLUDE_TERMINAL_MODULE
#include <DabbleESP32.h>

#define LED_BUILTIN 2
#define SENSOR  27 //pino medidor de fluxo


const int RelePin = 16; //pino relé

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int volume=0;
unsigned long volume_total =0;
bool cond_valvula = false;
bool cond_iniciar = true;
char estado = 'n';
unsigned long volume_valvula = 1000; // escolher o volume para abrir a valvula em litros
char menu = 's';
bool msg_conf1 = true;
String texto;

void medir_Fluxo()
{
  if (currentMillis - previousMillis > interval) {
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    volume = (flowRate / 60) * 1000;
    volume_total += volume;
  
    if (estado == 'e')
    {
      Serial.print("[*] Enchendo");
      Serial.print(" Flow rate: ");
      Serial.print(int(flowRate));
      Serial.print("L/min");
      Serial.print("\t");   // Print com espaço do tab
      Serial.print("Output Liquid Quantity: ");
      Serial.print(volume_total);
      Serial.print("mL / ");
      Serial.print(volume_total / 1000);
      Serial.print("L ");
      float porcentagem_valvula = (float(volume_total/1000)/float(volume_valvula))*100;
      Serial.print(porcentagem_valvula);
      Serial.println(" %");
      unsigned long tempo_msg = currentMillis - previousMillis;
      if (tempo_msg <= 200)
      {
        String texto = "[E] Enchendo: "+ String(float(volume_total/1000))+"L - " + String(float(volume_valvula))+"L / " + String(float(float((volume_total/1000))/float(volume_valvula))*100)+"% " + "(parar/pausar)";
        Terminal.print(texto);
        
        if (static_cast<int>(porcentagem_valvula)%25 ==0 || porcentagem_valvula ==0 || (porcentagem_valvula >=90 && porcentagem_valvula <91))
        {
          Notification.notifyPhone("Enchimento está em: "+String(float(float((volume_total/1000))/float(volume_valvula))*100)+"%");
        }
      }
    }
    else
    {
      Serial.print("[*] Flow rate: ");
      Serial.print(int(flowRate));  
      Serial.print("L/min");
      Serial.print("\t");       // Print com espaço do tab
      Serial.print("Output Liquid Quantity: ");
      Serial.print(volume_total);
      Serial.print("mL / ");
      Serial.print(volume_total / 1000);
      Serial.println("L");
    }
  }
}

void ligar_valvula()
{
	digitalWrite(RelePin, HIGH);
  Serial.println("[*] Ligando Valvula");
}

void desligar_valvula()
{
	digitalWrite(RelePin, LOW);
  Serial.println("[*] Desligando Valvula");
}

void teclado()
{
	if (Serial.available())
	{
		char leitura = Serial.read(); 
		if (leitura == '1'){cond_valvula = true;}
    else if (leitura == '2'){volume_total -= 1000;}
    else if (leitura == '3'){volume_total +=1000 ;}
    else if (leitura == '4'){ligar_valvula();}
    else if (leitura == '5'){desligar_valvula();}
	}
  switch (menu)
  {
    case 's':
    if (Terminal.compareString("iniciar"))
    {
      Terminal.println("[*] Iniciando");
      Terminal.println("[*] Comando 'parar' realiza o cancelamento de qualquer operacao");
      Terminal.println("[*] Digite o volume a ser enchido: (0 - 999)");
      menu = 'l';
    }
    break;
    case 'c':
    Terminal.println("[C] Enchimento Concluido");
    Notification.setTitle("Dosadora - Enchimento Concluido");
    Notification.notifyPhone("Enchimento Concluido");
    menu = 's';
    break;
    case 'l':
    if (Terminal.available())
    {
      
      int msg = Terminal.readNumber();
      if (Terminal.compareString("parar"))
      {
        Terminal.println("[P] Parando o processo de enchimento");
        menu = 's';
        cond_valvula = false;
      }
      else if(msg <= 999 && msg >=0)
      {
        volume_valvula = float(msg);

        menu ='q';
      }
      else if (msg <0)
      {
        Terminal.println("[*] Por favor Digite um Numero");
        
      }
      else if (msg >999)
      {
        Terminal.println("[*] Volume Invalido");
      } 
    }
    break;
    case 'q':
      if (msg_conf1){Terminal.println("[Q] Tem Certeza do Volume? (s/n)"); msg_conf1 = false;}
      
      if (Terminal.compareString("s"))
      {
        msg_conf1 = true;
        cond_valvula = true; 
        Notification.setTitle("Dosadora - Enchimento em Andamento");
        menu ='n';
      }
      else if (Terminal.compareString("n"))
      {
        Terminal.println("[*] Digite o volume a ser enchido: (0 - 999)");
        msg_conf1 = true;
        menu = 'l';
      }
        
    break;
    case 'n':
    if (Terminal.compareString("parar"))
    {
      menu = 'w';
    }
    if (Terminal.compareString("pausar"))
    {
      Terminal.println("[*] Pausando o enchimento (continuar/parar)");
      estado = 'n';
    }
    if (Terminal.compareString("continuar"))
    {
      Terminal.println("[*] Continuando o enchimento (parar/pausar)");
      estado = 'e';
    }

    break;

    case 'w':
    if (msg_conf1){Terminal.println("[Q] Tem Certeza que deseja parar? (s/n)"); msg_conf1 = false;}
      
      if (Terminal.compareString("s"))
      {
        Terminal.println("[P] Parando o enchimento");
        msg_conf1 = true;
        cond_valvula = false;
        menu ='s';
      }
      else if (Terminal.compareString("n"))
      {
        if (estado == 'e')
        {
          Terminal.println("[*] Continuando o enchimento (parar/pausar)");
        }
        else if (estado == 'n')
        {
          Terminal.println("[*] Enchimento esta pausado (continuar/parar)");
        }
        msg_conf1 = true;
        menu = 'n';
      }
  }
}

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

void setup()
{
  Serial.begin(115200);
  Dabble.begin("Dosadora"); 
  Notification.clear(); 
  Notification.setTitle("Dosadora");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(RelePin, OUTPUT);
  pulseCount = 0;
  flowRate = 0.0;
  volume = 0;
  volume_total = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void loop()
{
  currentMillis = millis();
  Dabble.processInput(); 
  medir_Fluxo();
  teclado();
	
	if (cond_valvula && cond_iniciar)
	{
		volume_total = 0;
		volume = 0;
		estado = 'l';
    cond_iniciar = false;
	}
  else if (!cond_valvula && cond_iniciar)
  {
    estado = 'n';
  }

	switch(estado) //estados da valvula
	{
		case 'l': //ligada
		{
			ligar_valvula();
			estado = 'e';
      break;
		}
		case 'd': //desligada
		{
			desligar_valvula();
			cond_valvula = false;
      cond_iniciar = true;
			estado = 'n';
      break;

		}
		case 'n': //neutro
		{
      //Serial.println("[*] Estado Neutro");
      break;
		}
		case 'e': //enchendo
		{
			if ((volume_total/1000) >= volume_valvula)
			{
				estado = 'd';
        menu = 'c';
        
			}
      else if (!cond_valvula) {estado='d';}
      break;
    }
  }
}
