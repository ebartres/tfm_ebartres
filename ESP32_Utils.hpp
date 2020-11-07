void ConnectWiFi_STA()
{
  Serial.begin(115200);
  
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connectant a la WiFi..");
  }
 
  Serial.print("Connectat a la WiFi ");
  Serial.println(ssid);
}
