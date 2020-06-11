void ReadSetup() {
  Serial.println("mounted file system");
  if (SPIFFS.exists("/config.json")) {
    //file exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      Serial.println("opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success()) {
        Serial.println("\nparsed json");

        strcpy(mqtt_server, json["mqtt_server"]);
        strcpy(mqtt_port, json["mqtt_port"]);
        strcpy(username, json["username"]);
        strcpy(password, json["password"]);
        strcpy(ssid, json["ssid"]);
        strcpy(wifipassword, json["wifipassword"]);
      } else {
        Serial.println("failed to load json config");
      }
    }
  }
}

void SaveConfig() {
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["username"] = username;
  json["password"] = password;
  json["ssid"] = ssid;
  json["wifipassword"] = wifipassword;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
  //end save
}

void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(ID);
  sprintf(mqttTopicStatus, "/%08X/Status", ESP.getChipId());
  sprintf(ONOFF_FEED, "/%08X/cmd", ESP.getChipId());

  if (SPIFFS.begin()) ReadSetup();
  
  //if (SPIFFS.begin()) SaveConfig();
  WiFi.mode(WIFI_STA);
  WiFi.begin(&ssid[0], &wifipassword[0]);
  WiFi.hostname(ID);


  //if you get here you have connected to the WiFi
  if (WiFi.waitForConnectResult() == WL_CONNECTED)
    Serial.println("connected...yeey :)");
  else {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Proceeding");
  Serial.printf(" ESP8266 Chip id = -%08X-\n", ESP.getChipId());


  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ID);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR   ) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR  ) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR    ) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  /* Prepare MQTT client */
  Serial.printf("MQTTServer %s port %s\n", mqtt_server, mqtt_port);
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);

}


String ipToString(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

void reconnect() {
  // Loop until we're reconnected
  char str[25];
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID)) {
      Serial.println("connected");
      client.subscribe(ONOFF_FEED);
      ipToString(WiFi.localIP()).toCharArray(IPadr, 25);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void wifiHandle() {
  ArduinoOTA.handle();
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) reconnect(); else client.loop();
  }
}