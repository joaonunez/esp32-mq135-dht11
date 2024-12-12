#include <WiFi.h>
#include <WebServer.h> // Biblioteca para el servidor web
#include <Preferences.h>
#include <FirebaseESP32.h> // Biblioteca para Firebase
#include <DHT.h> // Biblioteca para el sensor DHT11

// Pin Definitions
#define MQ135_PIN_AOUT 32 // Pin analógico para el sensor MQ-135 (D32)
#define DHT_PIN 33 // Pin digital para el sensor DHT11 (D33)
#define DHT_TYPE DHT11 // Definiendo el tipo de sensor DHT
#define PM10_PIN 34 // Pin analógico para el potenciómetro B20K (D34)

// Firebase configuration
FirebaseData firebaseData; // Objeto para interactuar con Firebase
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// Credenciales Wi-Fi
const char* ssid = "arduino"; // Nombre de la red Wi-Fi
const char* password = "tendobanshou"; // Contraseña del Wi-Fi

// ID del dispositivo
String dispositivoID = "-ODvFtcuXM5ZCqk3C9_0"; // ID del dispositivo en la base de datos

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

  // Leer el valor crudo del potenciómetro B20K
  int pm10Crudo = analogRead(PM10_PIN);
  // Convertir el valor crudo a PM10 (asumimos que el rango de 0-4095 se convierte directamente a 0-1000 µg/m3)
  float pm10 = map(pm10Crudo, 0, 4095, 0, 1000);

  // Calcular PM2.5 a partir de PM10 usando la relación aproximada de 0.65
  float pm2_5 = pm10 * 0.65;

  Serial.print("Valor crudo PM10: ");
  Serial.println(pm10Crudo);
  Serial.print("PM10 (µg/m3): ");
  Serial.println(pm10);
  Serial.print("PM2.5 (µg/m3): ");
  Serial.println(pm2_5);

  String path = "/dispositivos/" + dispositivoID + "/";
  
  // Subir PM10 a Firebase
  subirValorFirebase(path, "PM10", pm10);
  
  // Subir PM2.5 a Firebase
  subirValorFirebase(path, "PM2_5", pm2_5);

  // Subir otros valores como CO, CO2, temperatura y humedad
  subirValorFirebase(path, "lectura_cruda", valorCrudo);
  subirValorFirebase(path, "CO2", ppm);
  subirValorFirebase(path, "CO", co);
  subirValorFirebase(path, "temperatura", temperatura);
  subirValorFirebase(path, "humedad", humedad);
}

void subirValorFirebase(String path, String nombre, float valor) {
  Serial.println("Enviando " + nombre + " a Firebase...");
  if (Firebase.setFloat(firebaseData, path + nombre, valor)) {
    Serial.println("✅ " + nombre + " actualizado en Firebase");
  } else {
    Serial.print("❌ Error al actualizar " + nombre + ": ");
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
