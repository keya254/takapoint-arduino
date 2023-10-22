#include <Wire.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Arduino_JSON.h>

#define RX_PIN 10  // Define the RX pin for the SIM800L module
#define TX_PIN 11  // Define the TX pin for the SIM800L module

SoftwareSerial sim800l(RX_PIN, TX_PIN);  // Create a SoftwareSerial object for the SIM800L module

const char* serverAddress = "backendtaka.onrender.com";
const char* endpoint = "/api/binCans";

int irPin = 27;
int count = 0;
boolean state = true;

#define I2CADDR_KPD 0x3D
#define I2CADDR_LCD 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

const char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', '+'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[4] = {3, 2, 1, 0};
byte colPins[4] = {7, 6, 5, 4};

Keypad_I2C kpd(makeKeymap(keys), rowPins, colPins, 4, 4, I2CADDR_KPD, PCF8574);
LiquidCrystal_I2C lcd(I2CADDR_LCD, LCD_COLS, LCD_ROWS);

String phoneNumber = "";

void updateEndpoint(String phoneNumber, int count) {
  // Construct the JSON payload
  JSONVar data;
  data["binId"] = "R102";
  data["phoneNumber"] = phoneNumber;
  data["canNo"] = count;
  String requestBody = JSON.stringify(data);

  // Send the HTTP request using the SIM800L module
  sim800l.print("AT+HTTPINIT\r");
  delay(1000);
  sim800l.print("AT+HTTPPARA=\"CID\",1\r");
  delay(1000);
  sim800l.print("AT+HTTPPARA=\"URL\",\"");
  sim800l.print(serverAddress);
  sim800l.print(endpoint);
  sim800l.print("\"\r");
  delay(1000);
  sim800l.print("AT+HTTPPARA=\"CONTENT\",\"application/json\"\r");
  delay(1000);
  sim800l.print("AT+HTTPDATA=");
  sim800l.print(requestBody.length());
  sim800l.print(",10000\r");
  delay(1000);
  sim800l.print(requestBody);
  delay(1000);
  sim800l.print("AT+HTTPACTION=1\r");
  delay(1000);

  // Check the HTTP response
  while (!sim800l.find("+HTTPACTION:1,")) {
    // Waiting for the response
    delay(1000);
  }

  int responseCode = sim800l.parseInt();
  if (responseCode == 200) {
    Serial.println("Data sent successfully (HTTP 200 OK)");
  } else {
    Serial.println("Failed to send data");
  }

  // Terminate the HTTP session
  sim800l.print("AT+HTTPTERM\r");
  delay(1000);
}

void setup() {
  Wire.begin();
  kpd.begin(makeKeymap(keys));
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  pinMode(26, OUTPUT);
  pinMode(irPin, INPUT);
  Serial.println("start");

  // Initialize the SIM800L module
  sim800l.begin(9600);

  // Connect to the cellular network
  sim800l.print("AT+CREG?\r");
  while (!sim800l.find("+CREG: 0,1")) {
    // Waiting for network registration
    delay(1000);
    sim800l.print("AT+CREG?\r");
  }
  Serial.println("Registered on the network");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("#Welcome 2 Takapont");
  lcd.setCursor(0, 1);
  lcd.print("press A: to Deposit");
  lcd.setCursor(0, 2);
  lcd.print("press B: For Balance");
}

void loop() {
  char key = kpd.getKey();

  if (!digitalRead(irPin) && state) {
    digitalWrite(26, HIGH);
    count++;
    state = false;
    Serial.print("Count: ");
    Serial.println(count);
  } else {
    if (digitalRead(irPin)) {
      state = true;
      digitalWrite(26, LOW);
      delay(100);
    }
  }

  if (key) {
    Serial.println(key);
    switch (key) {
      case 'A':
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter phone number:-");
        phoneNumber = "";
        break;

      case '#':
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Drop your bottles");
        lcd.setCursor(0, 1);
        lcd.print(count);
        break;

      case 'D':
        if (phoneNumber.length() > 9) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Congratulations we are now updating....");
          updateEndpoint(phoneNumber, count);
          delay(1000);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("#Welcome 2 Takapont");
          lcd.setCursor(0, 1);
          lcd.print("press A: to Deposit");
          lcd.setCursor(0, 2);
          lcd.print("press B: For Balance");
          phoneNumber = "";
          count = 0;
        } else {
          lcd.clear();
          lcd.print("Please enter number");
          delay(1000);
        }
        break;

      case 'C':
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cancelling...");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("#Welcome 2 Takapont");
        lcd.setCursor(0, 1);
        lcd.print("press A: to Deposit");
        lcd.setCursor(0, 2);
        lcd.print("press B: For Balance");
        phoneNumber = "";
        count = 0;
        break;

      default:
        if (phoneNumber.length() < 13) { // Limit to 13 characters for a phone number
          phoneNumber += key;
          lcd.print(key);
        }
        break;
    }
  }

  // Update count and display on LCD
  if (!state) {
    lcd.setCursor(22, 1);
    lcd.print("    ");
    lcd.setCursor(22, 1);
    lcd.print(count);
    delay(1500); // update ThingSpeak every 30 seconds
  }
}
