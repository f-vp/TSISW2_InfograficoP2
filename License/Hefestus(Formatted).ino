#include <Ultrasonic.h>
// Motor A : Left
// Motor B : Right
// Preto no ENA
#define DIST_THRESHOLD 77	// Definir tolerância do sensor ultrassom
#define DEF_THRESHOLD 5		// Definir tolerância para fuga
#define BORDER_THRESHOLD 10	// Definir tolerância para fuga da borda
#define MAX_ATK_TIME 3000	// Definir tempo máximo de ataque
#define ECHO_F 9			// Pino echo do ultrassom frontal
#define TRIG_F 8			// Pino trig do ultrassom frontal
#define ECHO_B 7			// Pino echo do ultrassom traseiro
#define TRIG_B 2			// Pino trig do ultrassom traseiro
#define BORDER_F 12			// Pino do sensor IR frontal
#define BORDER_B 0			// Pino do sensor IR traseiro
#define enableA 11			// Pino de ativar motor esquerdo
#define pinA1 6				// Pino para avançar motor esquerdo
#define pinA2 5				// Pino para retorceder motor esquerdo
#define enableB 10			// Pino de ativar motor direito
#define pinB1 4				// Pino para avançar motor direito
#define pinB2 3				// Pino para retorceder motor direito
#define TIMEOUT 3000		// Define o timeout para 3000 microssegundos (3ms)

// Define as variáveis
unsigned long attackTime, oldTime;
long front_distance, back_distance;
bool borderFrontal, borderBack, attacking, beingAttackedF, beingAttackedB, verified;
Ultrasonic ultrasonic_F(TRIG_F, ECHO_F, TIMEOUT);
Ultrasonic ultrasonic_B(TRIG_B, ECHO_B, TIMEOUT);

void setup()
{
	// Inicializar os pinos como saída
	Serial.begin(4800);
	pinMode(BORDER_F, INPUT);
	pinMode(BORDER_B, INPUT);
	pinMode(enableA, OUTPUT);
	pinMode(pinA1, OUTPUT);
	pinMode(pinA2, OUTPUT);
	pinMode(enableB, OUTPUT);
	pinMode(pinB1, OUTPUT);
	pinMode(pinB2, OUTPUT);
	verified = attacking = beingAttackedF = beingAttackedB = borderFrontal = borderBack = false;	// Inicializa as variáveis como falso
	attackTime = oldTime = 0;
	//delay(5000);
}

void loop()
{
	enableMotors();
	if (!attacking)
		brakeMotor();

	// Bordas
	if (!digitalRead(BORDER_F))												// Verifica se o detector de borda frontal foi ativado
	{
		Serial.print("Borda Frontal ");
		if (!digitalRead(BORDER_B))											// Verifica se o detector de borda traseira foi ativado
		{
			turnRight();													// Vira para direita para tentar fugir, ele está "encostando em duas bordas"
			attacking = false;												// Define o estado como não atacando
			Serial.print("Rodando para a direita (Fugindo da Borda)\n");
		}
		else if (back_distance <= BORDER_THRESHOLD)							// Verifica o sensor ultrassonico traseiro
		{
			turnRight();													// Vira para direita para tentar fugir, está sendo atacado
			attacking = false;												// Define o estado como não atacando
			Serial.print("Rodando para a direita (Fugindo da Borda e Inimigo na frente)\n");
		}
		else if (front_distance <= BORDER_THRESHOLD)						// Verifica o sensor ultrassonico frontal
		{
			forward();														// Ataca, o inimigo ainda não caiu
			attacking = true;												// Define o estado como atacando
			Serial.print("Atacando (Inimigo na borda da frente)\n");
		}
		else
		{
			backward();														// Foge da borda
			attacking = true;												// Define o estado como atacando
			Serial.print("Indo para trás (Fugindo da Borda)\n");
		}
	}

	else if (!digitalRead(BORDER_B))										// Verifica se o detector de borda traseira foi ativado
	{
		Serial.print("Borda Traseira ");
		if (!digitalRead(BORDER_F))											// Verifica se o detector de borda frontal foi ativado
		{
			turnRight();													// Vira para direita para tentar fugir, ele está "encostando em duas bordas"
			attacking = false;												// Define o estado como não atacando
			Serial.print("Rodando para a direita (Fugindo da Borda)\n");
		}
		else if (front_distance <= BORDER_THRESHOLD)						// Verifica o sensor ultrassonico frontal
		{
			turnRight();													// Vira para direita para tentar fugir, está sendo atacado
			attacking = false;												// Define o estado como não atacando
			Serial.print("Rodando para a direita (Fugindo da Borda e Inimigo na traseira)\n");
		}
		else if (back_distance <= BORDER_THRESHOLD)							// Verifica o sensor ultrassonico traseiro
		{
			backward();														// Ataca, o inimigo ainda não caiu
			attacking = true;												// Define o estado como atacando
			Serial.print("Atacando (Inimigo na borda de trás)\n");
		}
		else
		{
			forward();														// Foge da borda
			Serial.print("Indo para frente (Fugindo da Borda)\n");
		}
	}

	// Ultrassonicos
	front_distance = ultrasonic_F.read(CM);									// Lê a distância do sensor frontal
	else if (front_distance <= DIST_THRESHOLD)								// Verifica se a distância é menor ou igual a tolerância
	{
		if (verified)														// Verifica se já foi feita a verificação
		{
			if (beingAttackedF)												// Se estiver sendo atacado
			{
				attacking = false;											// Define o estado como não atacando
				motorBCoast();												// Foge
				motorABackward();
				Serial.print("Fugindo (Frente)\n");
			}
			else															// Se não está sendo atacado
			{
				attackTime += millis() - oldTime;							// Calcula tempo de ataque
				if (attackTime >= MAX_ATK_TIME)  							// Se o tempo de ataque excede 3 segunds
				{
					attacking = false;										// Define o estado como não atacando
					motorBCoast();											// Foge
					motorABackward();
					Serial.print("Mudando de posicão (Tempo limite de ataque)\n");
				}
				else														// Se não exceder
				{
					attacking = true;										// Define o estado como atacando
					forward();												// Ataca
					Serial.print("Indo para frente (Atacando)\n");
				}
			}
		}
		else																// Se não tiver verificado
		{
			brakeMotor();													// Espera para poder ler se está sendo atacado
			delay(150);
			if (front_distance - ultrasonic_F.read(CM) >= DEF_THRESHOLD)		// Se estiver sendo atacado
			{
				beingAttackedF = true;										// Define como "está sendo atacado"
				attacking = false;											// Define o estado como não atacando
				Serial.print("Está sendo atacado (Frente)\n");
			}
			verified = true;												// Define como "verificado"
		}
	}

	back_distance = ultrasonic_B.read(CM); 									// Lê a distância do sensor traseiro
	else if (back_distance <= DIST_THRESHOLD)							// Verifica se a distância é menor ou igual a tolerância
	{
		if (verified)													// Verifica se já foi feita a verificação
		{
			if (beingAttackedB)											// Se estiver sendo atacado
			{
				attacking = false;										// Define o estado como não atacando
				motorACoast();											// Foge
				motorBForward();
				Serial.print("Fugindo (Trás)\n");
			}
			else														// Se não está sendo atacado
			{
				attackTime += millis() - oldTime;						// Calcula tempo de ataque
				if (attackTime >= MAX_ATK_TIME)  						// Se o tempo de ataque excede 3 segunds
				{
					attacking = false;									// Define o estado como não atacando
					motorACoast();										// Foge
					motorBForward();
					Serial.print("Mudando de posicão (Tempo limite de ataque)\n");
				}
				else													// Se não exceder
				{
					attacking = true;									// Define o estado como atacando
					backward();											// Ataca
					Serial.print("Indo para trás (Atacando)\n");
				}
			}
		}
		else															// Se não tiver verificado
		{
			brakeMotor();												// Espera para poder ler se está sendo atacado
			delay(150);
			if (back_distance - ultrasonic_B.read(CM) >= DEF_THRESHOLD)	// Se estiver sendo atacado
			{
				beingAttackedB = true;									// Define como "está sendo atacado"
				attacking = false;										// Define o estado como não atacando
				Serial.print("Está sendo atacado (Trás)\n");
			}
			verified = true;											// Define como "verificado"
		}
	}

	else																// Se nada for detectado
	{
		turnRight();													// Procura
		Serial.print("Rodando para a esquerda (Procurando)\n");
		verified = attacking = beingAttackedF = beingAttackedB = false;	// Reseta as variáveis
		attackTime = 0;
	}
	
	oldTime = millis();													// Define o tempo do ciclo anterior (a cada fim de ciclo)
}

void enableMotors()
{
	motorAOn();
	motorBOn();
}
void disableMotors()
{
	motorAOff();
	motorBOff();
}
void forward()
{
	motorAForward();
	motorBForward();
}
void backward()
{
	motorABackward();
	motorBBackward();
}
void turnLeft()
{
	motorABackward();
	motorBForward();
}
void turnRight()
{
	motorAForward();
	motorBBackward();
}
void coast()
{
	motorACoast();
	motorBCoast();
}
void brakeMotor()
{
	motorABrake();
	motorBBrake();
}
// Define low-level H-bridge commands
// Enable motors
void motorAOn()
{
	digitalWrite(enableA, HIGH);
}
void motorBOn()
{
	digitalWrite(enableB, HIGH);
}
// Disable motors
void motorAOff()
{
	digitalWrite(enableB, LOW);
}
void motorBOff()
{
	digitalWrite(enableA, LOW);
}
// Motor A controls
void motorAForward()
{
	digitalWrite(pinA1, HIGH);
	digitalWrite(pinA2, LOW);
}
void motorABackward()
{
	digitalWrite(pinA1, LOW);
	digitalWrite(pinA2, HIGH);
}
// Motor B controls
void motorBForward()
{
	digitalWrite(pinB1, HIGH);
	digitalWrite(pinB2, LOW);
}
void motorBBackward()
{
	digitalWrite(pinB1, LOW);
	digitalWrite(pinB2, HIGH);
}
// Coasting and braking
void motorACoast()
{
	digitalWrite(pinA1, LOW);
	digitalWrite(pinA2, LOW);
}
void motorABrake()
{
	digitalWrite(pinA1, HIGH);
	digitalWrite(pinA2, HIGH);
}
void motorBCoast()
{
	digitalWrite(pinB1, LOW);
	digitalWrite(pinB2, LOW);
}
void motorBBrake()
{
	digitalWrite(pinB1, HIGH);
	digitalWrite(pinB2, HIGH);
}
