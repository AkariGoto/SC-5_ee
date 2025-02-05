int phase = 1;
char key = '0';

int phase_state = 0;

int launch_PIN = 33;            //トランジスタのピン番号の宣言
int launch_outputsecond = 5;       //点火時の9V電圧を流す時間，単位はsecond

bool prelaunch = false;
int countdown = 3;
int ignitionstate = false;

int64_t sensorValue_bin[14];
int Datanumber = 0;

//phase3で使用する変数
int type = 1;
int yeah = 1;
int type_state = 1;
int cutparac = 32;          //切り離し用トランジスタのピン番号の宣言
int outputcutsecond = 5;    //切り離し時の9V電圧を流す時間，単位はsecond
float time3_1,St_Time;      //時間に関するもの
float Accel[6];             //計測した値をおいておく関数(加速度)
float Altitude[6];          //(高度)
float Preac,differ1,Acsum,Acave,RealDiffer1;
float Preal,differ2,Alsum,Alave,RealDiffer2;
int i=0;
int j=0;


//for GPS
#include <TinyGPS++.h>
#include <math.h>

TinyGPSPlus gps;

//int i=0;
int n=0;
//int j=0;
// double gpslat[10];
// double sum_lat;
// double gpslng[10];
// double sum_lng;
double GOAL_lat = 35.862857820;
double GOAL_lng = 139.607681275;
double v_initial= 38.0;  //[m/s]
double g        = 9.80665;  //[m/s/s]
double delta_lng,GPS_lat,GPS_lng,distance,angle_radian,angle_degree;

//for MPU9250
#include <MPU9250_asukiaaa.h>
#ifdef _ESP32_HAL_I2C_H_
#define SDA_MPU 21 //I2C通信
#define SCL_MPU 22
#endif
MPU9250_asukiaaa mySensor;

//for servomoter
#include <ESP_servo.h>
#include <stdio.h>
#include <string>

ESP_servo servo1;
ESP_servo servo2;
int pos1 = 0;
int pos2 = 0;
int nowAngle1 = 0;
int nowAngle2 = 0;
int Angle1 = 30;
int Angle2 = 15;
int increment = 1;
unsigned long previousMillis = 0;
unsigned long OnTime = 3000;
unsigned long OffTime = 5000;
int moterstate = LOW;
int interval = 30;
int servophase = 6;
int moter_end = 0;



//for BMP180
#include <Wire.h> //I2C通信
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;
#define SDA_BMP 21
#define SCL_BMP 22

//for SD Card
#include <SPI.h> //SDカードはSPI通信
#include <SD.h>

File CanSatLogData;
File SensorData;

const int sck=13 , miso=15 , mosi=14 , ss=27;

//センサー値の格納
double Temperature, Pressure, accelX, accelY, accelZ, magX, magY, magZ, gyroX, gyroY, gyroZ, accelSqrt, gps_latitude, gps_longitude, altitude;
int gps_time;


// Interrupt timer
volatile int timeCounter1;
hw_timer_t *timer1 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//for phase1,2
int mode_average1 = 0;
int mode_average2 = 0;
int count1 = 0;
int count2 = 0;
double altitude_average = 0;
double altitude_sum = 0;
double altitude_target = 100; //目標地点の高さ
double altitude_max; //目標地点の海抜高さ(BMPで測定)
double TBD_accel = 6.0;
double TBD_altitude = 7; //終端速度3[m\s]*切断にかかる時間2[s]+パラシュートがcansatにかぶらないで分離できる高度1[m]
double TBD_h = altitude_max - altitude_target + TBD_altitude; //ニクロム線に電流を流し始める海抜高度
double alt[5];



//サーボモータの角度変更の計算
//int t_caculator(double distance_, v_initial_){
//    theta = atan(power(initial_,2)/(g*distance_) + sqrt(power()))
//}


// Interrupt timer function
void IRAM_ATTR onTimer1(){
    portENTER_CRITICAL_ISR(&timerMux);
    timeCounter1++;
    portEXIT_CRITICAL_ISR(&timerMux);
}


// 64bit整数データのバイナリー変換関数
void casttobyte64(int64_t data, byte buf[]){
    buf[0] = (data >> 56) & 0x00FF;
    buf[1] = (data >> 48) & 0x00FF;
    buf[2] = (data >> 40) & 0x00FF;
    buf[3] = (data >> 32) & 0x00FF;
    buf[4] = (data >> 24) & 0x00FF;
    buf[5] = (data >> 16) & 0x00FF;
    buf[6] = (data >> 8) & 0x00FF;
    buf[7] = (data) & 0x00FF;
}


// 16bit整数データのバイナリー変換関数
void casttobyte16(int16_t data, byte buf[]){
    buf[0] = (data >> 8) & 0x00FF;
    buf[1] = (data) & 0x00FF;
}







void setup() {
    // SD Card initialization
    SPI.begin(sck,miso,mosi,ss);
    SD.begin(ss,SPI);
    CanSatLogData = SD.open("/CanSatLogData.log", FILE_APPEND);
    SensorData = SD.open("/SensorData.bin",FILE_APPEND);

    //無線通信
    Serial2.begin(115200, SERIAL_8N1, 16, 17); //関数内の引数はデータ通信レート，わからん，RXピン，TXピン
    Serial2.write("TESTING: Serial communication\n");
    Serial2.write("TESTING: Serial communication\n");

    //LED
    pinMode(launch_PIN, OUTPUT);        //点火用トランジスタの出力宣言
    pinMode(cutparac, OUTPUT);      //切り離し用トランジスタの出力宣言
    digitalWrite(launch_PIN, LOW);      //点火用トランジスタの出力オフ
    digitalWrite(cutparac, LOW);    //切り離し用トランジスタの出力オフ

    //for MPU9250
    mySensor.beginAccel();
    mySensor.beginGyro();
    mySensor.beginMag();
    //Offset値を変える必要あるかも
    mySensor.magXOffset = -50;
    mySensor.magYOffset = -55;
    mySensor.magZOffset = -10;


    //for GPS
    Serial1.begin(115200, SERIAL_8N1, 5, 18); //関数内の引数はデータ通信レート,わからん,RXピンTXピン


    //for servomoter
    servo1.init(23,0);
    servo2.init(19,1);
    pinMode(4,OUTPUT);
    digitalWrite(4, moterstate);
    Serial2.begin(115200);

    //for BMP
    Wire.begin(SDA_BMP, SCL_BMP);

    int sensorData_d[10];


}//setup関数閉じ


void loop() {
    //割り込み関数（適切にサンプリングレートを確立するために）
    if (timeCounter1 > 0) {
        portENTER_CRITICAL(&timerMux);
        timeCounter1--;
        portEXIT_CRITICAL(&timerMux);


        //起動時刻の更新
        unsigned long currentMillis = millis();

        //センサー値のアップデート
        mySensor.accelUpdate();
        mySensor.gyroUpdate();
        mySensor.magUpdate();


        //センサー値取得
        Temperature = bmp.readTemperature();
        Pressure = bmp.readPressure();
        accelX = mySensor.accelX();
        accelY = mySensor.accelY();
        accelZ = mySensor.accelZ();
        magX = mySensor.magX();
        magY = mySensor.magY();
        magZ = mySensor.magZ();
        gyroX = mySensor.gyroX();
        gyroY = mySensor.gyroY();
        gyroZ = mySensor.gyroZ();
        altitude = bmp.readAltitude();
        accelSqrt = mySensor.accelSqrt();

        // GPSデータの更新をするかどうか
        if(gps.location.isUpdated()){   //アップデートの実行
            char c = Serial1.read();    //GPSチップからのデータを受信
            gps.encode(c);              //GPSチップからのデータの整形

            gps_latitude = gps.location.lat();
            gps_longitude = gps.location.lng();
            gps_time = gps.time.value();

        }


        //地上局からのフェーズ指示
        if(Serial2.available()){

            char key = Serial2.read();

            switch(key){
                case '1':
                    phase = 1;
                    break;

                case '2':
                    phase = 2;
                    break;

                case '3':
                    phase = 3;
                    break;

                case '4':
                    phase = 4;
                    break;

                case '5':
                    phase = 5;
                    break;
            }
        }//地上局からのフェーズ指示閉じ





        //各フェーズごとの記述
        switch (phase){

            //########## 待機フェーズ ##########
            case 1:

                if(!phase_state == 1){
                    //待機フェーズに入ったとき１回だけ実行したいプログラムを書く
                    Serial2.write("Phase1: transition completed\n");
                    CanSatLogData.print(gps_time);
                    CanSatLogData.print("\tPhase1: transition completed\n");
                    CanSatLogData.flush();
                    phase_state = 1;
                }

                if(accelSqrt >= 0.1 && accelSqrt <= 0.2 ) //最高点に来たら
                {
                    if(mode_average1==0){//5個のデータがたまるまで
                      alt[count1] = altitude;
                      count1++;
                      if(count1==5){
                          for(count2=0;count2<5;count2++){
                            altitude_sum = altitude_sum + alt[count2]; // 受信したデータを足す
                          }
                          altitude_max = altitude_sum/5;
                          mode_average1++;
                          count1=0;
                      }
                    }
                }

                if(accelSqrt >= TBD_accel && accelZ < 0) //落下開始を加速度で判定
                {
                    Serial2.write("FALL STARTED\n");
                    phase = 2;
                }


                /*if(altitude_average <= TBD_h) //パラシュートを分離する予定の高度を下回った場合、分離フェーズ(フェーズ3)に直接移行
                {
                    Serial.write("MPU DOESN'T OPERATE. SKIP PHASE 2.\n");
                    phase = 3;
                }*/


                break;

            //########## 降下フェーズ ##########
            case 2:
                if(!phase_state == 2){
                    //降下フェーズに入ったとき１回だけ実行したいプログラムを書く
                    Serial2.write("Phase2: transition completed\n");
                    CanSatLogData.print(gps_time);
                    CanSatLogData.print("\tPhase2: transition completed\n");
                    CanSatLogData.flush();
                    phase_state = 2;
                }

                if(altitude_average>TBD_h)
                {
                  if(mode_average2==0){//5個のデータがたまるまで
                    alt[count1] = altitude;
                    count1++;
                    if(count1==5){
                        for(count2=0;count2<5;count2++){
                          altitude_sum = altitude_sum + alt[count2]; // いったん受信したデータを足す
                        }
                        altitude_average = altitude_sum/5;
                        mode_average2++;
                        count1=0;
                    }
                  }
                  else{//5個のデータがたまった後
                        altitude_sum = 0;
                        altitude_average = 0;
                        for(count2=0;count2<4;count2++){
                          alt[count2]=alt[count2+1];
                        }
                        alt[4]=altitude;
                        for(count2=0;count2<5;count2++){
                          altitude_sum = altitude_sum + alt[count2];
                        }
                        altitude_average = altitude_sum/5;
                  }
                }else{//ニクロム線に電流を流す高度以下になったら
                  phase = 3;
                }

                break;

            //########## 分離フェーズ ##########
            case 3:
                if(!phase_state == 3){
                    //分離フェーズに入ったとき１回だけ実行したいプログラムを書く
                    Serial2.Write("Phase3: transition completed\n");
                    Serial2.Write("");
                    phase_state = 3;
                    St_Time = time3_1 + outputcutsecond * 1000;        //基準時間
                    time3_1 = currentMillis;                           //phase3　開始時間の保存
                    
                    Serial2.write("WARNING: The cut-para code has been entered.\n");
                    digitalWrite(cutparac, HIGH); //オン
                    Serial2.write("WARNING: 9v voltage is output.\n");
                }
                    
                
                switch(type){
                case 1:  //type = 1
                    if(!type_state == 1){     //電流フェーズに入ったとき１回だけ実行したいプログラムを書く              
                        Serial2.Write("Phase3_type1: transition completed\n");
                        Serial2.Write("");
                        type_state = 2;

                        Serial2.Write("WARNING: The cut-para code has been entered.\n");
                        digitalWrite(cutparac, HIGH); //オン
                        Serial2.Write("WARNING: 9v voltage is output.\n");
                    }
 
                    if(time3_1 > St_Time){     //電流を流した時間が基準時間を超えたら 
                        digitalWrite(cutparac, LOW); //オフ
                        Serial2.Write("WARNING: 9v voltage is stop.\n");
                        type = 2;
                    }


                case 2:  //type = 2
                    if (mySensor.accelUpdate() == 0) {
                        if(!type_state == 2){　　//停止フェーズに入ったとき１回だけ実行したいプログラムを書く
                            Serial2.Write("Phase3_type2: transition completed\n");
                            Serial2.Write("");
                            type_state = 3;
                            i = 0;
                            j = 0;
                            Preac = 0;         //1秒前の加速度を記憶
                            differ1 = 0.1;     //accelsqurt移動平均の差
                            differ2 = 0.5;        //altitude移動平均の差
                            Acave = 0;         //加速度5個の平均値
                            RealDiffer1 = 0;    //1秒前との差を記憶する
                            RelaDiffer2 = 0;
                        }

                        Acsum = 0;         //加速度5個の合計値
                        Alsum = 0;         //高度5個の合計値

                        if(yeah == 1){     //データを初めから五個得るまで
                          Accel[i] = accelSqrt;
                          Altitude[i] = altitude;
                          i = i + 1;
                          if(i == 6){        //5個得たらその時点での平均値を出し，次のフェーズへ
                               yeah = 2;
                               i = 0; //iの値をリセット
                               for(j=1 ; j<6 ; j++){   //j=0の値は非常に誤差が大きいので1から
                                Acsum = Acsum + Accel[j];　  
                                Alsum = Alsum + Altitude[j];
                               }
                               Acave = Acsum/5;
                               Alave = Alave/5;
                          }
                        }else{
                          Preac = Acave;
                          Preal = Alave;  
                          Accel[i] = accelSqrt;
                          Altitude[i] = altitude;
                          for(j=0 ; j<5 ; j++){
                            Acsum = Acsum + Accel[j];
                            Alsum = Alsum + Altitude[j];
                          }
                          Acave = Acsum/5;
                          Alave = Alsum/5;
                          RealDiffer1 = Preac - Acave;
                          RealDiffer2 = Preal - Alave;
                          if(i == 5){
                            i = 0;
                          }else{
                            i = i+1;
                          }
                          
                          if( RealDiffer1 < differ1 ){ //移動平均が基準以内の変化量だった時
                            phase = 4;
                          }else( RealDiffer2 < differ2 ){
                            phase = 4;
                          }
                        }
                    }
                }




            //########## 採取フェーズ ##########
            case 4:
                if(!phase_state == 4){
                    //待機フェーズに入ったとき１回だけ実行したいプログラムを書く
                    Serial2.write("Phase4: transition completed\n");
                    CanSatLogData.print(gps_time);
                    CanSatLogData.print("\tPhase4: transition completed\n");
                    CanSatLogData.flush();
                    phase_state = 4;
                }

                if((moterstate == LOW) && (moter_end == 0) && (currentMillis - previousMillis >= OffTime)){

                    moterstate = HIGH;
                    previousMillis = currentMillis;
                    Serial2.write("Moter start rotating \n");
                    CanSatLogData.print(gps_time);
                    CanSatLogData.print("\tMotor start rotating\n");
                    CanSatLogData.flush();
                    digitalWrite(4,moterstate);

                }else if((moterstate == HIGH) && (moter_end == 0) && (currentMillis - previousMillis >= OnTime)){
                    moterstate = LOW;
                    previousMillis = currentMillis;
                    Serial2.write("Moter finished rotating \n");
                    CanSatLogData.print(gps_time);
                    CanSatLogData.print("\tMotor finished rotating\n");
                    CanSatLogData.flush();
                    digitalWrite(4,moterstate);
                    moter_end = 1;
                }
                else if((moterstate == LOW) && (moter_end == 1) && (currentMillis - previousMillis >= OffTime)){
                    previousMillis = currentMillis;
                    phase = 5;
                }
                break;




            //########## 発射フェーズ ##########
            case 5:
                if(!phase_state == 5){
                    //待機フェーズに入ったとき１回だけ実行したいプログラムを書く
                    Serial2.write("Phase5: transition completed\n");
                    CanSatLogData.print(gps_time);
                    CanSatLogData.print("\tPhase5: transition completed\n");
                    CanSatLogData.flush();
                    phase_state = 5;
                }


                switch(servophase){

                    case 6:
                        if(nowAngle1 != Angle1){
                            if((pos1 < Angle1) && (currentMillis - previousMillis >= interval)) {
                                previousMillis = currentMillis;
                                pos1 += increment;
                                servo1.write(pos1);
                                Serial2.write(pos1);
                            }else if ((pos1 > Angle1) && (currentMillis - previousMillis >= interval)){
                                previousMillis = currentMillis;
                                pos1 -= increment;
                                servo1.write(pos1);
                                Serial2.write(pos1);
                            }
                            nowAngle1 = pos1;
                        }else if(nowAngle1 == Angle1){
                            Serial2.write("******Servo1 finished rotating***** \n");
                            CanSatLogData.print(gps_time);
                            CanSatLogData.print("\tServo1 finished rotating\n");
                            CanSatLogData.flush();
                            servophase = 7;
                            }
                        break;



                    case 7:
                        if(nowAngle2 != Angle2){
                            if((pos2 < Angle2) && (currentMillis - previousMillis >= interval)) {
                                previousMillis = currentMillis;
                                pos2 += increment;
                                servo2.write(pos2);
                                Serial2.write(pos2);
                            }else if ((pos2 > Angle2) && (currentMillis - previousMillis >= interval)){
                                previousMillis = currentMillis;
                                pos2 -= increment;
                                servo2.write(pos2);
                                Serial2.write(pos2);
                            }
                            nowAngle2 = pos2;
                        }else if(nowAngle2 == Angle2){
                            Serial2.write("******Servo2 finished rotating***** \n");
                            CanSatLogData.print(gps_time);
                            CanSatLogData.print("\tServo2 finished rotating\n");
                            CanSatLogData.flush();
                            servophase = 8;
                        }

                        break;

        }//フェーズ関数閉じ
 
        break;


        //無線通信による指示switch関数
        switch(key){
        case 'm':
        Serial2.write("****** Servo Motor1 plung angle determination mode ******\n");
        Serial2.write("Enter Motor Angle: ");
        while(true){
            if (Serial2.available() ){
                String key = Serial2.readStringUntil(';');
                Serial2.write(key.c_str());
                Serial2.write('\n');
                int newAngle1 = atoi(key.c_str());
                if (nowAngle1 != newAngle1){
                    if (newAngle1 <= 180 && newAngle1 >= 0){
                        Serial2.write("WARMING: MORER1 IS ROTATING \n");
                        while (pos1 != newAngle1){
                            currentMillis = millis();
                            if  ((pos1 < newAngle1) && (currentMillis - previousMillis >= interval)){
                                previousMillis = currentMillis;
                                servo1.write(pos1++);
                                Serial2.write(pos1);
                            }
                            else if((pos1 > newAngle1) && (currentMillis - previousMillis >= interval)){
                                previousMillis = currentMillis;
                                servo1.write(pos1--);
                                Serial2.write(pos1);
                            }
                        }
                    }
                    else{
                        Serial2.write("Warming: Out of the range \n");
                    }
                    nowAngle1 = newAngle1;
                }
                break;
            }
        }
        currentMillis = previousMillis;
        key = '0';
        break;

        case 'n':
        Serial2.write("****** Servo Motor2 launch angle determination mode ******\n");
        Serial2.write("Enter Motor Angle: ");
        while(true){
            if (Serial2.available()){
                String key = Serial2.readStringUntil(';');
                Serial2.write(key.c_str());
                Serial2.write('\n');
                int newAngle2 = atoi(key.c_str());
                if (nowAngle2 != newAngle2){
                    if (newAngle2 <= 180 && newAngle2 >= 0){
                        Serial2.write("WARMING: MORER1 IS ROTATING \n");
                            while (pos2 != newAngle2){
                                currentMillis = millis();
                                if ((pos2 < newAngle2) && (currentMillis - previousMillis >= interval)){
                                    previousMillis = currentMillis;
                                    servo2.write(pos2++);
                                    Serial2.write(pos2);
                                }
                                else if ((pos2 > newAngle2) && (currentMillis - previousMillis >= interval)){
                                    previousMillis = currentMillis;
                                    servo2.write(pos2--);
                                    Serial2.write(pos2);
                                }
                            }
                        }
                        else{
                            Serial2.write("Warming: Out of the range\n");
                        }
                        nowAngle2 = newAngle2;
                    }
                    break;
                }
            }
            currentMillis = previousMillis;
            key = '0';
            break;

         case 'l':
              Serial.write("WARNING: The firing code has been entered.\n");
              Serial.write("WARNING: Are you sure you want to fire it?\n");
              Serial.write("WARNING: Press the y key to allow firing.\n");
              prelaunch = true;
              key = '0';
              break;

         case 'y':
              if(prelaunch){
                if(ignitionstate){
                  if(currentMillis - previousMillis >= launch_outputsecond * 1000){
                    Serial.write("LAUCHING: 9V voltage is stop.\n");
                    digitalWrite(launch_PIN, LOW); //オフ
                    ignitionstate = 0;
                    countdown = 3;
                    prelaunch = false;
                    key = '0';
                  }
                }
                else if(currentMillis - previousMillis >= 1000){
                  char c_countdown = '0' + countdown;
                  Serial.write("COUNTDOWN: ");
                  Serial.write(c_countdown);
                  Serial.write("\n");
                  --countdown;
                  if(countdown+1<=0){
                    Serial.write("LAUCHING: 9V voltage is output.\n");
                    digitalWrite(launch_PIN, HIGH); //オン
                    ignitionstate = true;
                  }
                  previousMillis = currentMillis;
                }
              }
              break;

        case 'e':
            prelaunch = false;
            digitalWrite(launch_PIN, LOW);
            ignitionstate = 0;
            countdown = 3;
            Serial2.write("WARNING: The EMERGENCY code has been entered\n");
            key = '0';
            break;

        }//無線通信による指示switch関数閉じ

        Datanumber++;


        //SDカードへデータを保存する
        sensorValue_bin[0] = Temperature * 1000;
        sensorValue_bin[1] = Pressure * 1000;
        sensorValue_bin[2] = accelX * 1000;
        sensorValue_bin[3] = accelY * 1000;
        sensorValue_bin[4] = accelZ * 1000;
        sensorValue_bin[5] = magX * 1000;
        sensorValue_bin[6] = magY * 1000;
        sensorValue_bin[7] = magZ * 1000;
        sensorValue_bin[8] = gyroX * 1000;
        sensorValue_bin[9] = gyroY * 1000;
        sensorValue_bin[10] = gyroZ * 1000;
        sensorValue_bin[11] = gps_latitude * 1000000000;
        sensorValue_bin[12] = gps_longitude * 1000000000;
        sensorValue_bin[13] = gps_time;

        for (int i = 0; i<14; i++) {
                byte buf[8];
                casttobyte64(sensorValue_bin[i],buf);
                SensorData.write(buf,sizeof(buf));
        }



        }
    }
    //割り込み関数の閉じ
}
//loop関数の閉じ
