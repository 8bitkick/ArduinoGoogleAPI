// 
// OAuth 2.0 for Google APIs on Arduino
//
// To set-up app on cloud see prerequisites section:
// https://developers.google.com/identity/protocols/OAuth2ForDevices
//
// NB Limited API scopes
//
// 8bitkick 2018

#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi101.h>
#include <FlashStorage.h>


// WiFi
char ssid[] = SECRET_SSID;   
char pass[] = SECRET_PASS;  
int status = WL_IDLE_STATUS;
WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, "accounts.google.com", 443);

struct HttpResponse {
  int statusCode;
  String response;
};


// Google API - Scope is access to Google Drive files this app creates
// -------------------------------------------------------------------
String scope = "https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fdrive.file";
String client_id = SECRET_CLIENT_ID;
String client_secret = SECRET_CLIENT_SECRET;
String access_token;

// Save refresh token in flash 
// So we can skip user authorization after reset

typedef struct {
  boolean valid;
  // Google OAuth fresh token 
  char refresh_token[70];
} SavedState;

FlashStorage(flash, SavedState);
SavedState mySavedState;


// JSON Arduino  
const size_t bufferSize = JSON_OBJECT_SIZE(5) + 230;
DynamicJsonBuffer jsonBuffer(bufferSize);


// ==========================================================================
// SECTION: Wifi
// ==========================================================================


void setup_wifi(){
  
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    
    // wait 1 seconds for connection:
    delay(3000);
  }
  Serial.println("Connected to wifi");
  
  printWifiStatus();
  
  
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


// ==========================================================================
// SECTION: Google API access
// ==========================================================================

void setup_auth(){
  
  // We're an app requesting access to Google Drive for only the files we create
  // scope = https://www.googleapis.com/auth/drive.file
  
  Serial.println("Google API: Authentication required"); 
  
  String content = "client_id=" + client_id + "&scope=" + scope;
  HttpResponse googRequestCodes = https_post("accounts.google.com","/o/oauth2/device/code", "application/x-www-form-urlencoded", content);
  
  // Parse response
  JsonObject& parsedJSON = jsonBuffer.parseObject(googRequestCodes.response);
  
  String verification_url = parsedJSON["verification_url"]; 
  int expires_in = parsedJSON["expires_in"]; 
  int interval = parsedJSON["interval"]; 
  String device_code = parsedJSON["device_code"]; 
  String user_code = parsedJSON["user_code"]; 
  
  jsonBuffer.clear(); // Allocated memory is freed
  
  // Display user code and request user grants this app permission 
  Serial.println("\n\n*********************");
  Serial.println("To activate go to:\n");
  Serial.println(verification_url);
  Serial.print  ("\ncode: ");
  Serial.println(user_code);
  Serial.println("*********************");
  
  // Poll to see if they have granted permission
  HttpResponse googAccessRequest;
  
  while (googAccessRequest.statusCode != 200){
    
    googAccessRequest = https_post("www.googleapis.com","/oauth2/v4/token", "application/x-www-form-urlencoded", 
    "client_id=" + client_id + 
    "&client_secret=" + client_secret + 
    "&code=" + device_code + 
    "&grant_type=http%3A%2F%2Foauth.net%2Fgrant_type%2Fdevice%2F1.0");
    
    delay(5000);
  }
  
  // Parse response
  JsonObject& parsedJSON2 = jsonBuffer.parseObject(googAccessRequest.response);
  
  access_token = parsedJSON2["access_token"].as<String>(); 
  //int token_expires_in = parsedJSON2["expires_in"]; 
  String refresh_token = parsedJSON2["refresh_token"];     // To be stored in Flash
  
  jsonBuffer.clear(); // Allocated memory is freed
  
  Serial.println("\nAccess granted");
  Serial.println("*********************");
  
  refresh_token.toCharArray(mySavedState.refresh_token, 70);
  mySavedState.valid = true;
  
}


void token_refresh(){ 
  // We're requesting a token refresh
  HttpResponse googTokenRefresh = https_post("www.googleapis.com","/oauth2/v4/token", "application/x-www-form-urlencoded", 
  "client_id=" + client_id + 
  "&client_secret=" + client_secret + 
  "&refresh_token=" + mySavedState.refresh_token + 
  "&grant_type=refresh_token");  
  
  // Parse response
  JsonObject& parsedJSON3 = jsonBuffer.parseObject(googTokenRefresh.response);
  access_token = parsedJSON3["access_token"].as<String>(); 
  // int token_expires_in = parsedJSON3["expires_in"]; TODO
  
  jsonBuffer.clear(); // Allocated memory is freed
  
  Serial.println("Google API: Access token refreshed");
}


// POST
HttpResponse https_post(String host, String path, String contentType, String data) {
  
  #ifdef DEBUG
  Serial.println("POST: "+data);
  #endif
  
  client.stop();
  client = HttpClient(wifi, host, 443);
  
  client.beginRequest();
  client.post(path);
  client.sendHeader("Content-Type", contentType);
  client.sendHeader("Content-Length", data.length());
  client.endRequest();
  client.print(data);
  
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  
  #ifdef DEBUG
  Serial.print("Status code is: ");
  Serial.println(statusCode);
  
  Serial.print("Response is: ");
  Serial.println(response);
  #endif
  
  return HttpResponse {statusCode, response};
}

// GET
HttpResponse https_get(String host, String path) {
  
  client.stop(); 
  client = HttpClient(wifi, host, 443);
  client.beginRequest();
  client.get(path);
  client.sendHeader("Authorization", "Bearer "+ access_token);
  client.endRequest();
  
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  
  #ifdef DEBUG
  Serial.print("Status code is: ");
  Serial.println(statusCode);
  
  Serial.print("Response is: ");
  Serial.println(response);
  #endif
  
  return HttpResponse {statusCode, response};
}


// ==========================================================================
// SECTION: Set up
// ==========================================================================



void setup() {
  
  // Serial set-up
  //--------------------------------------------
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // Restore state from flash
  // ------------------------
  mySavedState = flash.read();
  
  
  // WiFi set-up
  
  setup_wifi();
  
  // Google API access - refresh 
  
  if (mySavedState.valid == true){
    Serial.println("Google API: Refresh token exists in flash"); 
    token_refresh(); // TODO track expiry time
  } else {
    
    
    // Google API access - new
    
    setup_auth();
    
    // Save refresh token to flash
    
    flash.write(mySavedState);
    
  }
  
  // Test out an API call
  
  Serial.println(https_get("www.googleapis.com","/drive/v3/files").response);
  
}




// ==========================================================================
// SECTION: Loop
// ==========================================================================


void loop() {
  
  
}

