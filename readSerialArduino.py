#!/usr/bin/python

# Importamos la libreira de PySerial
import serial
from datetime import datetime,date,time,timedelta
import sys
import serial
import time
import codecs



num_s=4
sens_adc = [0]*num_s
sens_volt= [0]*num_s
sens_voltK= [0]*num_s
hum_6g= [0]*num_s
hum_T=[0]*num_s
muestras=0
#filtro kalman
b_before,var_estados,var_obser,b_after,Gan,sal,sal_ant=[0.0]*num_s,[0.99]*num_s,[5.0]*num_s,[25.0]*num_s,[0.0]*num_s,[0.0]*num_s,[0.0]*num_s

#b_before=0 var_estados=0.99 var_obser=5 b_after=25 Gan=0 sal=0  sal_ant=0
def trozos(v):
    if(v<=1.34):
        h=9.616*v-6.267
    elif(v<=1.41):
        h=79.12*v-100.8
    elif(v<=1.6036):
        h=11.27*v-5.678
    elif(v<=1.7735):
        h=17.75*v-14.86
    elif(v<=2.0923):
        h=40.19*v-54.05
    elif(v<=2.3201):
        h=6.499*v+16.38
    elif v<=3.0:
        h=4.44*v+20.82
    else:
        h=36.0
    return h
def kalman(dat,b_before,var_estados,var_obser,b_after,Gan,sal,sal_ant,index):
  b_before[index]=b_after[index]+var_estados[index]
  Gan[index]=b_before[index]/(b_before[index]+var_obser[index])
  sal[index]=sal_ant[index]+Gan[index]*(dat-sal_ant[index])
  b_after[index]=(1-Gan[index])*b_before[index]
  sal_ant[index]=sal[index]
  return sal[index] 

def write_intxt(date,adc_dat,volts_dat,voltsK_dat,delta):
 try:
  datos=open("/home/pi/Desktop/SEBASTIAN_TESIS/data_set/hum_sexg.txt","a")
  datos.write(str(date)+" "+str(adc_dat[0])+" "+str(adc_dat[1])+" "
              +str(adc_dat[2])+" "+str(adc_dat[3])+" "+delta+"\n")
  datos.close()
  
  datos=open("/home/pi/Desktop/SEBASTIAN_TESIS/data_set/volts_veg.txt","a")
  datos.write(str(date)+" "+str(volts_dat[0])+" "+str(volts_dat[1])+" "
              +str(volts_dat[2])+" "+str(volts_dat[3])+" "+delta+"\n")
  datos.close()
  datos=open("/home/pi/Desktop/SEBASTIAN_TESIS/data_set/hum_ec_T.txt","a")
  datos.write(str(date)+" "+str(voltsK_dat[0])+" "+str(voltsK_dat[1])+" "
              +str(voltsK_dat[2])+" "+str(voltsK_dat[3])+" "+delta+"\n")
  datos.close()
  return False
 except:
  print("ERROR IN SAVE DATA")
  return True
# Abrimos el puerto del arduino a 9600

# Creamos un buble sin fin
  # leemos hasta que encontarmos el final de linea
  
  
a=0 
init_t= datetime.now() 
#while(a<3):
while(True):    
    try:
      PuertoSerie = serial.Serial('/dev/ttyACM0', 9600,timeout=1.0)
      PuertoSerie.timeout=1;
      try:
          sArduino = PuertoSerie.readline()
          inp=codecs.encode(sArduino, 'hex')
          re_s_m=str(inp)
          ls=re_s_m[2:len(re_s_m)-3]
          vals=ls.split("2c")
          for x in range(0,num_s):
            valx=bytes.fromhex(vals[x]).decode() #convert hex to string
            vali=int(valx, 0)#convert string to int
            sens_adc[x]=vali
            volts=round((vali*4.68)/1023,4)
            sens_volt[x]=volts
            volt_k=kalman(volts,b_before,var_estados,var_obser,b_after,Gan,sal,sal_ant,x)
            sens_voltK[x]=round(volt_k,4)
            #ecuacion de 6 grado para vegetronix
            hum_6g[x]=round(3.3876*volts**6-28.292*volts**5+84.238*volts**4-107.41*volts**3
                          +58.528*volts**2-1.1292*volts-6.5007,2)
            #ecuacion atrozos
            hum_T[x]=round(trozos(volts),2)
            
            
            
                
          now=datetime.now()  
          delta=str(now-init_t)
          write_intxt(str(now),hum_6g,sens_volt,hum_T,delta)
            
              # Mostramos el valor leido y eliminamos el salto de linea del final
          print ("Valor Arduino: " + str(vals))
          print ("sensores humedad 6 grado ecuacion: " + str(hum_6g))
          print ("sensores volt: " + str(sens_volt))
          print ("sensores humedad ec a trozos: " + str(hum_T))
          time.sleep(100)
          
          PuertoSerie.close()
      except:
        print("no data")
    except serial.SerialException:
   #-- Error al abrir el puerto serie
      sys.stderr.write("Error al abrir puerto (%s)\n" % str(Puerto))
      sys.exit(1)
    