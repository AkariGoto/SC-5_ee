#include <Wire.h>
#include <Adafruit_BMP085.h>


Adafruit_BMP085 bmp;
  
void setup() {
  Serial.begin(9600);
  if (!bmp.begin()) {
  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  while (1) {}
  }
}
  
void loop() {

    
    double Alt[5];
    double ALT;
    double TBD;//決めた高度

    
    //高度について、5個のデータの移動平均を出す。
     
    //高度のデータを配列に入れる。
    for(int i=0;;i++){ 
    Alt[i] = bmp.readAltitude();
    Altsum = 0;//五個のデータの合計値


    //先に作った配列の中身の和を出して、移動平均を出す。
    for(int k=0 ; k<5 ; k++){
      Altsum = Altsum + Alt[k];
      ALT = Altsum/5
    }

    if(i>5 && ALT<TBD) break;　//高度の移動平均が決定地よりも低かったらループを抜け出す
    //抜け出し方は要検討
    

    if( i==4) i=0;
    //iが4に到達したら、i=0を代入し繰り返す。
    }
    
    Serial.println();
    delay(75);
}
