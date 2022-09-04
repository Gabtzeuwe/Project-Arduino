#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

// declarare variabile pentru reteaua de wifi
const char* ssid = "Gabriel";
const char* password = "12345678";

#define bleServerName "esp32" // numele placutei (bluetooth)
String newID; // variabila pentru salvarea idului cerut
String baseURL = "http://proiectia.bogdanflorea.ro/api/futurama/";  // api url
bool deviceConnected = false;  // variabila pentru a vedea daca device ul este conectat la bluetooth

  String APPResponse(BLECharacteristic *characteristic) {  // preia cerinta de la aplicatia de pe telefon, deserializeaza intr-un string si trimite mai departe 
  String APPString = characteristic->getValue().c_str(); // preia valoarea cerintei : fetchdata fetchdetails
  
  DynamicJsonDocument Document(256); // document pentru deserializarea cerintei :   fetchdata/details
  deserializeJson(Document, APPString); // deserializarea efectiva

  Serial.println(); // debug pentru a vedea in monitorul serial cerinta :fetchdata/details
  
  String toDo = Document["action"]; // salvarea valorii de la cheia action : fetchdata/details
  if (toDo == "fetchData") {
    // daca action e fetchData adica pagina principala 
    Serial.println(toDo);
    return toDo; // return fetchData pentru a urma cerinta separat intr o alta functie
  }
  else if (toDo == "fetchDetails") {
    String id = Document["id"];
    Serial.print(toDo);
    Serial.print(" : ");
    Serial.println(id);   
      newID = id; // salvam id;
    return toDo;
  }

}
String APIRequest(String adder) {
  
  HTTPClient http;  // declarare protocol http

  String path = baseURL + adder; // se creaza url
  String payload = "";
  Serial.print("Connecting to: ");
  Serial.println(path);
  http.setTimeout(10000); 
  http.begin(path); // conectarea la api
  int httpCode = http.GET(); 
  payload = http.getString(); 
  http.end(); 
  return payload;
}
void fetchDetails(String payload,BLECharacteristic *characteristic) {
    StaticJsonDocument <1024> JSONDocument; // declarea document static pentru deserializarea stringului de forma json primit din api
    deserializeJson(JSONDocument, payload.c_str());

  // salvam datele necesare;
    String responseString; 
    String id = JSONDocument["char_id"].as<String>();
    String name = JSONDocument["Name"].as<String>();
    String image = JSONDocument["PicUrl"].as<String>();
    String Species = JSONDocument["Species"].as<String>();
    String Age = JSONDocument["Age"].as<String>();
    String Planet = JSONDocument["Planet"].as<String>();
    String Profession = JSONDocument["Profession"].as<String>();
    String Status = JSONDocument["Status"].as<String>();
    String VoicedBy = JSONDocument["VoicedBy"].as<String>();
    String description = "Species: " + Species + "\n" + "Age: " + Age + "\n" + "Planet:" + Planet + "\n" + "Profession: "+ Profession + "\n" + "Status: "+ Status + "\n" + "VoicedBy: "+ VoicedBy;
    StaticJsonDocument <512> newJSONDocument;
    JsonObject object = newJSONDocument.to<JsonObject>();
    // declaram un obiect pe care o sa il setam ca valoare in aplicatie ;
    object["id"] = id;
    object["name"] = name;
    object["image"] = image;
    object["description"] = description;
    object["status"]=Status;
    serializeJson(newJSONDocument, responseString);
    characteristic->setValue(responseString.c_str());
    characteristic->notify();  //notify
    Serial.println(responseString); 
}

void fetchData(String payload,BLECharacteristic *characteristic) {
  DynamicJsonDocument JSONDocument(15000);
  //analog difera doar tipul de date din obiect json in array json (parcurgem array ul json avand analogul de mai sus pentru un singur obiect)
  deserializeJson(JSONDocument, payload.c_str());

    JsonArray list = JSONDocument.as<JsonArray>();

    int index = 1;
    for (JsonVariant value : list) {
   
      JsonObject listItem = value.as<JsonObject>();
    
      String id = listItem["char_id"].as<String>();
      String name = listItem["Name"].as<String>();
      String image = listItem["PicUrl"].as<String>();
      String responseString;

      StaticJsonDocument <512> newJSONDocument;
      JsonObject object = newJSONDocument.to<JsonObject>();
      object["id"] = id;
      object["name"] = name;
      object["image"] = image;
      serializeJson(newJSONDocument, responseString);
      characteristic->setValue(responseString.c_str());
      characteristic->notify();
      Serial.print(index);
      Serial.print(" ");
      Serial.println(responseString);
      delay(100);
      index++;
    }
  
}  //Daca requestul este de tipul {"action":"fetchData"} inseamna ca noi trebuie sa afisam sub aceasta forma
//{id: int|string, name: string, image: string (url|base64)}}. Conecteaza la
// api folosind protocolul http si se salveaza datele intr un document json pe
//care il vom deserializa si formata pentru a obtine stringul dorit. 

#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59" // definire adresa uuids

BLECharacteristic indexCharacteristic(
  "ca73b3ba-39f6-4ab3-91ae-186dc9577d99", 
  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
);

BLEDescriptor *indexDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));

BLECharacteristic detailsCharacteristic(
  "183f3323-b11f-4065-ab6e-6a13fd7b104d", 
  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
);
BLEDescriptor *detailsDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2902));


class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true; // true daca s a conectat la bluetooth
    Serial.println("Device connected");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;  // false daca s a conectat la bluetooth
    Serial.println("Device disconnected");
  }
};class CharacteristicsCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
            String responseFromApp = APPResponse(characteristic); // variabila pentru salvarea stringului din app
        if (responseFromApp == "fetchData") {
          String payload = APIRequest("characters");        
          fetchData(payload,characteristic);    
        }
      
        else if (responseFromApp == "fetchDetails") {
         
          String payload = APIRequest("character?id="+newID); // query la api
         
          fetchDetails(payload,characteristic);        
        }
        
      }
};

void setup() { // setam o singura data anumiti parametrii;
  
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); // conectare la wifi
  while (WiFi.status() != WL_CONNECTED) { //verificam statusul
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");

  BLEDevice::init(bleServerName); // initializam bluetooth-ul

  BLEServer *pServer = BLEDevice::createServer();  // cream un server ble
  pServer->setCallbacks(new MyServerCallbacks()); // setam o cale de acces


  BLEService *bmeService = pServer->createService(SERVICE_UUID); // cream un server pentru servicii
    // setam caracteristicile 
  bmeService->addCharacteristic(&indexCharacteristic);  
  indexDescriptor->setValue("Get data list");
  indexCharacteristic.addDescriptor(indexDescriptor);
  indexCharacteristic.setValue("Get data List");
  indexCharacteristic.setCallbacks(new CharacteristicsCallbacks());

  bmeService->addCharacteristic(&detailsCharacteristic);  
  detailsDescriptor->setValue("Get data details");
  detailsCharacteristic.addDescriptor(detailsDescriptor);
  detailsCharacteristic.setValue("Get data details");
  detailsCharacteristic.setCallbacks(new CharacteristicsCallbacks());
  
  bmeService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  
}
