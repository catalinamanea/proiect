#include <LiquidCrystal_I2C.h>
 
LiquidCrystal_I2C lcd(0x27,16,2); 

#include <Wire.h>


int x, y, z;
long ax, ay, az, acc;
long timer;
long x_calibrat, y_calibrat, z_calibrat;
int temperature;
int counter;
float unghi_incl, unghi_rotire;
int inclinare1, rotire1;
boolean unghiuri;
float rotireacc, inclinareacc;
float out, rotire;




void setup() {
  Wire.begin(); //comunicare I2C pentru MPU

  lcd.backlight();
                                                
  Wire.beginTransmission(0x68);          //adresa default pentru MPU                              
  Wire.write(0x6B);                      //registrul PWR_MGMT_1 permite configurarea MPU                    
  Wire.write(0x00);                                                    
  Wire.endTransmission();                                              

  Wire.beginTransmission(0x68);                                        
  Wire.write(0x1C);                          //registrul pentru configurarea accelerometrului                         
  Wire.write(0x10);                                                    
  Wire.endTransmission();                                              
 
  Wire.beginTransmission(0x68);                                       
  Wire.write(0x1B);                       //registrul pentru configurarea giroscopului                           
  Wire.write(0x08);                                                    
  Wire.endTransmission();                                                    

  lcd.begin(16, 2);  
                                                   
  lcd.clear();                                                         
 
  lcd.setCursor(0,0);                                                  
  lcd.print("Calibrare");                                       
  lcd.setCursor(0,1);    
  //se efactueaza calibrarea senzorului mpu6050 pentru a afisa o valoare cat mai concreta                                              
  for (int i = 0; i < 2000 ;i++){                  
    if(i % 125 == 0)
      lcd.print(".");   //estetic                           
    read_mpu();                                              
    x_calibrat += x;                                              
    y_calibrat += y;                                              
    z_calibrat += z;                                              
    delay(2);                                                          
  }
  x_calibrat /= 2000;           //se calculeza valoare de calibrare facand media arimetica a valorilor pentru fiecare axa in parte                                       
  y_calibrat /= 2000;                                                  
  z_calibrat /= 2000;                                                  

  lcd.clear();                                                        
 
  lcd.setCursor(0,0);                                                  
  lcd.print("Unghi: ");                                                 
  lcd.setCursor(0,1);                                                  
  lcd.print("Rotire:");                                                 
 
 
  timer = micros();                                               
}

void loop(){

  read_mpu();          //se citeste o noua valoare pentru fiecare dintre axe si se scade valoarea de la calibrare                                      

  x -= x_calibrat;                                                
  y -= y_calibrat;                                                
  z -= z_calibrat;                                               
 
  
 
  unghi_incl += x * 0.0000611;           //valoarea cu care se inmulteste este obtinuta pri formula 1/(250Hz / 65.5) unde 250Hz (4000us), iar 65.5 este LSB/g de la 500 dps                          
  unghi_rotire += y * 0.0000611;                                    
 
  
  unghi_incl += unghi_rotire * sin(z * 0.000001066);        //in cazul in care s-a intamplat o deviere atunci se transefra unghiurile intre ele           
  unghi_rotire -= unghi_incl * sin(z * 0.000001066);         // valoarea reprezinta  0.0000611 * (3.142(pi) / 180 grade)          
 
  
  acc = sqrt((ax*ax)+(ay*ay)+(az*az)); 
  
  inclinareacc = asin((float)ay/acc)* 57.296;       //1/(pi/180) rad=>grade  
  rotireacc = asin((float)ax/acc)* -57.296;       
 
  
  inclinareacc -= 0.0;                 //calibrare                                
  rotireacc -= 0.0;                                              

  if(unghiuri){                                                 
    unghi_incl = unghi_incl * 0.9996 + inclinareacc * 0.0004;     //daca are loc o rotire atunci se recalzuleaza valorile 
    unghi_rotire = unghi_rotire * 0.9996 + rotireacc * 0.0004;    //coreactand unghiurile de la giroscop cu cele de la accelerometru      
  }
  else{                                                               
    unghi_incl = inclinareacc;                                    
    unghi_rotire = rotireacc;                                       
    unghiuri = true;                                            
  }
 
  
  out = out * 0.9 + unghi_incl * 0.1;     // se aplica unn asa-zis filtru prin care pentru inclinare pastram 90% din valoarea initiala si 10% din ce de inclinare
  rotire = rotire * 0.9 + unghi_rotire * 0.1;      //invers pt rotatie
 
  write_mpu();                                                         

  while(micros() - timer < 4000);                                 //dupa 4000us se reseteaza loop ul
  timer = micros();                                              
}


void read_mpu(){                                             
  Wire.beginTransmission(0x68);                                        
  Wire.write(0x3B);                               // se citesc 14 adrese incepand de la 3B adica de la ACCEL_XOUT_H                     
  Wire.endTransmission();                                              
  Wire.requestFrom(0x68,14);                                           
  while(Wire.available() < 14);                                        
  ax = Wire.read()<<8|Wire.read();             //formam valorile intregi pentru fiecare componenta
  ay = Wire.read()<<8|Wire.read();             //pentru valorile calculate de accelerometru pe x,y,z                    
  az = Wire.read()<<8|Wire.read();                                  
  temperature = Wire.read()<<8|Wire.read();       //acesta citeste si temperatura, nu ne ajuta la nimic, dar trebuie sa scapam de valorile citite                     
  x = Wire.read()<<8|Wire.read();               //valorile de la giroscop pe x,y si z                  
  y = Wire.read()<<8|Wire.read();              //fiecare Wire.read() da o secventa de 8 biti, doar ca valoarea intreaga este formata din 15                   
  z = Wire.read()<<8|Wire.read();              //adica din lipirea valorilor de high(_H) si low(_L). asa ca se shifteaza cu 8 pozitii prima citire 
                                               // si se aplica OR pe rezultat cu cealalta valoare citita                 

}

void write_mpu(){                                                      
  
  if(counter == 14)   //pentru a afisa corect valorile pe ecran
    counter = 0;                      
  counter ++;                                                 
  if(counter == 1){   //de la 1 la 7, inclusiv, avem gradul de inclinare
    inclinare1 = out * 10;                      
    lcd.setCursor(8,0);                                                
  }
  if(counter == 2){
    if(inclinare1 < 0)  // se verifica valoarea daca e negativa sau pozitiva
      lcd.print("-");                         
    else 
      lcd.print("+");                                               
  }
  if(counter == 3)
    lcd.print(abs(inclinare1)/1000);    
  if(counter == 4)
     lcd.print((abs(inclinare1)/100)%10);
  if(counter == 5)
     lcd.print((abs(inclinare1)/10)%10); 
  if(counter == 6)
    lcd.print(".");                             
  if(counter == 7)
    lcd.print(abs(inclinare1)%10);      

  if(counter == 8){   //de la 8 la 14 avem gradul de rotire
    rotire1 = rotire * 10;
    lcd.setCursor(8,1);
  }
  if(counter == 9){
    if(rotire1 < 0)  //pozitiv sau negativ in functie de bit
      lcd.print("-");                           
    else 
      lcd.print("+");                                               
  }
  if(counter == 10)
    lcd.print(abs(rotire1)/1000);    
  if(counter == 11)
    lcd.print((abs(rotire1)/100)%10);
  if(counter == 12)
    lcd.print((abs(rotire1)/10)%10); 
  if(counter == 13)
    lcd.print(".");                            
  if(counter == 14)
    lcd.print(abs(rotire1)%10);      
}
