/* RC522 - Arduino
 * 3.3v =>  3.3v
 * RST => 9
 * GND => GND 
 * MISO => 12 
 * MOSI => 11 
 * SCK =>  13
 * SDA => 10
*/

#include <SPI.h>
#include <MFRC522.h>


#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

#define NR_KNOWN_KEYS 8 // Nombre de clef connues
// Liste des clefs connus
byte knownKeys[NR_KNOWN_KEYS][MFRC522::MF_KEY_SIZE] =  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // FF FF FF FF FF FF 
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // A0 A1 A2 A3 A4 A5
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}, // B0 B1 B2 B3 B4 B5
    {0x2c, 0x92, 0x4d, 0x58, 0xed, 0x0c}, // 2c 92 4d 58 ed 0c
    {0x33, 0xda, 0x0a, 0xb5, 0x2a, 0xd6},  // 33 da 0a b5 2a d6
    {0xf1, 0x3d, 0xa3, 0x07, 0x23, 0x6b},  // f1 3d a3 07 23 6b
    {0xe5, 0x16, 0x0a, 0x49, 0x2e, 0xfe},  // e5 16 0a 49 2e fe
    {0xb7, 0xe9, 0x92, 0x41, 0xf3, 0x52},  // b7 e9 92 41 f3 52   
};

#define NR_BLOCK 45
#define NR_BUFF_1 9
#define NR_BUFF_2 10

int montantTape;
boolean start = false;

/*
 * Initialize.
 */
void setup() {
    Serial.begin(9600); // Initialisation de la communication serial 
    SPI.begin(); // Initialisation du bus SPI                // Init SPI bus
    mfrc522.PCD_Init(); // Initialisation du module MFRC522
    Serial.println("Placez votre carte, indiquez le montant voulu en CENTIME puis touche Entrée : ");
    Serial.println("10e = 1000");
    Serial.println("5e = 500");
    Serial.println("2,5e = 250");
    Serial.println("1,25e = 125");
    Serial.println("**************");
    Serial.println("Montant Max = 25e");
    Serial.println("**************");
}

/*
 * Conversion d'octet en hexa
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

int StrToHex(char str[])
{
  return strtol(str, 0, 16);
}

int hexStringToInt(String s){
    char char_array[s.length() + 1];
    s.toCharArray(char_array, s.length() + 1);
    return strtol(char_array, 0, 16);
}

int getSolde(byte *buffer, byte bufferSize){
    Serial.println();
    String soldeHexa = "";
    soldeHexa = String(buffer[NR_BUFF_1], HEX)+String(buffer[NR_BUFF_2], HEX);
    char char_array[soldeHexa.length() + 1];
    soldeHexa.toCharArray(char_array, soldeHexa.length() + 1);
    int soldeDec = strtol(char_array, 0, 16);
    Serial.println((String)"Solde actuel = " + soldeDec + " centimes");
    return soldeDec;
}

boolean try_key(MFRC522::MIFARE_Key *key)
{
    boolean result = false;
    byte buffer[18];
    MFRC522::StatusCode status;
    
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return false;
    if ( ! mfrc522.PICC_ReadCardSerial())
        return false;
        
    Serial.println(F("Authentification avec la clef A..."));
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, NR_BLOCK, key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        return false;
    }

    // Read block
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(NR_BLOCK, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
      Serial.println("Erreur de Status");
    }
    else {
        int newSolde = montantTape;
        result = true;
        Serial.print(F("Clef utilisee:"));
        dump_byte_array((*key).keyByte, MFRC522::MF_KEY_SIZE);
        Serial.println();
        // Dump block data
        Serial.print(F("Block ")); Serial.print(NR_BLOCK); Serial.print(F(":"));
        dump_byte_array(buffer, 16);
        getSolde(buffer, 16);
        Serial.println((String)"Futur solde : " + newSolde + " centimes");
        Serial.println((String)"Futur solde Hex : " + String(newSolde,HEX));
        String newSoldeHex = String(newSolde,HEX);
        newSoldeHex = newSoldeHex.length() == 3 ? String("0")+newSoldeHex : "";
        buffer[NR_BUFF_1] = hexStringToInt((String)newSoldeHex[0]+newSoldeHex[1]);
        buffer[NR_BUFF_2] = hexStringToInt((String)newSoldeHex[2]+newSoldeHex[3]);
        
        Serial.println();

        status = mfrc522.MIFARE_Write(NR_BLOCK, buffer, 16);
        if (status != MFRC522::STATUS_OK) {
            Serial.print(F("MIFARE_Write() failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
                  return;
        }else{
          Serial.println("Ajout d'argent effectue avec succes");
          Serial.println("On dit merci qui ?");
          return true;
        }
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return result;
}

void serialEvent(){
  while (Serial.available()) {
    String input = Serial.readString();
    input.trim();
    if (input) {
      montantTape = input.toInt();
      start = true;
    }else{
      start = false;
    }
  }
}

void loop() {
    if(start){
      if(montantTape<=2500){
     
        // Recherche de carte
        if ( ! mfrc522.PICC_IsNewCardPresent())
            Serial.print("Pas de carte detecte");
    
        // Selection de carte
        if ( ! mfrc522.PICC_ReadCardSerial())
            Serial.print("Erreur lors de la lecture du serial");
    
        Serial.print(F("UID De la carte :"));
        dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
        Serial.println();
        Serial.print(F("type de PICC: "));
        MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
        Serial.println(mfrc522.PICC_GetTypeName(piccType));        
        MFRC522::MIFARE_Key key;
        for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
            for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
                key.keyByte[i] = knownKeys[k][i];
            }
            if (try_key(&key)) {
                break;
            }
        }
      start = false;
      }else{
        Serial.println("Erreur : Le montant doit etre inferieur ou egal 2500 centimes ! \nIndiquez le montant voulu en centime : ");
      }
    }
    start = false;
}
