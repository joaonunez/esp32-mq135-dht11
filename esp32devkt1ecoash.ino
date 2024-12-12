#include <WiFi.h>
#include <WebServer.h> // Biblioteca para el servidor web
#include <Preferences.h>
#include <FirebaseESP32.h> // Biblioteca para Firebase
#include <DHT.h> // Biblioteca para el sensor DHT11

// Pin Definitions
#define MQ135_PIN_AOUT 32 // Pin analógico para el sensor MQ-135 (D32)
#define DHT_PIN 33 // Pin digital para el sensor DHT11 (D33)
#define DHT_TYPE DHT11 // Definiendo el tipo de sensor DHT

// Firebase configuration
FirebaseData firebaseData; // Objeto para interactuar con Firebase
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// Credenciales Wi-Fi
const char* ssid = "arduino"; // Nombre de la red Wi-Fi
const char* password = "tendobanshou"; // Contraseña del Wi-Fi

// ID del dispositivo
String dispositivoID = "-ODW3V8uzFjP7SzuMqqE"; // ID del dispositivo en la base de datos

// Crear objeto DHT
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando...");

  // Conectar a Wi-Fi
  conectarWiFi();

  // Configurar Firebase
  Serial.println("Configurando Firebase...");
  firebaseConfig.api_key = "AIzaSyDtQJo1wtbbGDfN8Ki0jivaldKmmR3gFPA"; // Clave de API de tu proyecto
  firebaseConfig.database_url = "https://ecoash-96aed-default-rtdb.firebaseio.com/"; // URL de la base de datos
  firebaseAuth.user.email = "joaovaldiglesias@gmail.com"; // Usuario autenticado
  firebaseAuth.user.password = "123456"; // Contraseña del usuario

  Serial.println("API Key: " + String(firebaseConfig.api_key.c_str()));
  Serial.println("Correo: " + String(firebaseAuth.user.email.c_str()));
  Serial.println("Contraseña: " + String(firebaseAuth.user.password.c_str()));

  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase configurado.");

  // Iniciar DHT
  dht.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado. Actualizando datos...");
    actualizarDatosFirebase();
    delay(10000); // Intervalo de actualización (10 segundos)
  }
}

void conectarWiFi() {
  Serial.println("\nConectando a Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado a Wi-Fi");
  Serial.print("📶 Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void actualizarDatosFirebase() {
  // Leer el valor crudo del sensor MQ-135
  int valorCrudo = analogRead(MQ135_PIN_AOUT);
  
  // Convertir el valor crudo a ppm (conversión) según el modelo
  float ppm = convertirAppm(valorCrudo);

  // Calcular el CO en ppm (esto es una aproximación)
  float co = calcularCO(ppm);

  // Leer la temperatura y la humedad del sensor DHT11
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("❌ Error al leer del sensor DHT11");
    return;
  }

  Serial.print("Valor crudo leído del sensor: ");
  Serial.println(valorCrudo);
  Serial.print("Concentración de CO2 en ppm: ");
  Serial.println(ppm);
  Serial.print("Concentración de CO en ppm: ");
  Serial.println(co);
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");

  String path = "/dispositivos/" + dispositivoID + "/";
  
  // Subir valor crudo a Firebase
  Serial.println("Enviando calidad de aire cruda a Firebase...");
  if (Firebase.setInt(firebaseData, path + "lectura_cruda", valorCrudo)) {
    Serial.println("✅ Valor crudo actualizado en Firebase");
  } else {
    Serial.print("❌ Error al actualizar valor crudo: ");
    Serial.println(firebaseData.errorReason());
  }

  // Subir ppm de CO2 a Firebase
  Serial.println("Enviando concentración de CO2 (ppm) a Firebase...");
  if (Firebase.setFloat(firebaseData, path + "CO2", ppm)) {
    Serial.println("✅ PPM de CO2 actualizado en Firebase");
  } else {
    Serial.print("❌ Error al actualizar PPM de CO2: ");
    Serial.println(firebaseData.errorReason());
  }

  // Subir ppm de CO a Firebase
  Serial.println("Enviando concentración de CO (ppm) a Firebase...");
  if (Firebase.setFloat(firebaseData, path + "CO", co)) {
    Serial.println("✅ PPM de CO actualizado en Firebase");
  } else {
    Serial.print("❌ Error al actualizar PPM de CO: ");
    Serial.println(firebaseData.errorReason());
  }

  // Subir temperatura a Firebase
  Serial.println("Enviando temperatura a Firebase...");
  if (Firebase.setFloat(firebaseData, path + "temperatura", temperatura)) {
    Serial.println("✅ Temperatura actualizada en Firebase");
  } else {
    Serial.print("❌ Error al actualizar temperatura: ");
    Serial.println(firebaseData.errorReason());
  }

  // Subir humedad a Firebase
  Serial.println("Enviando humedad a Firebase...");
  if (Firebase.setFloat(firebaseData, path + "humedad", humedad)) {
    Serial.println("✅ Humedad actualizada en Firebase");
  } else {
    Serial.print("❌ Error al actualizar humedad: ");
    Serial.println(firebaseData.errorReason());
  }
}

float convertirAppm(int valorCrudo) {
  float Vout = (float)valorCrudo / 4095.0;
  const float A = 116.6020682;
  const float B = -2.769034857;
  return A * pow(Vout, B);
}

float calcularCO(float ppmCO2) {
  return ppmCO2 * 0.5; // Relación de conversión de ejemplo, ajustar según sea necesario
}
