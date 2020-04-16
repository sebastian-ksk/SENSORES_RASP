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
muestras=0
#filtro kalman
b_before,var_estados,var_obser,b_after,Gan,sal,sal_ant=[0.0]*num_s,[0.99]*num_s,[5.0]*num_s,[25.0]*num_s,[0.0]*num_s,[0.0]*num_s,[0.0]*num_s

#b_before=0 var_estados=0.99 var_obser=5 b_after=25 Gan=0 sal=0  sal_ant=0



def kalman(dat,b_before,var_estados,var_obser,b_after,Gan,sal,sal_ant,index):
  b_before[index]=b_after[index]+var_estados[index]
  Gan[index]=b_before[index]/(b_before[index]+var_obser[index])
  sal[index]=sal_ant[index]+Gan[index]*(dat-sal_ant[index])
  b_after[index]=(1-Gan[index])*b_before[index]
  sal_ant[index]=sal[index]
  return sal[index] 

def write_intxt(date,adc_dat,volts_dat,voltsK_dat,delta):
 try:
  datos=open("/home/pi/Desktop/SEBASTIAN_TESIS/data_set/adc_veg.txt","a")
  datos.write(str(date)+" "+str(adc_dat[0])+" "+str(adc_dat[1])+" "
              +str(adc_dat[2])+" "+str(adc_dat[3])+" "+delta+"\n")
  datos.close()
  
  datos=open("/home/pi/Desktop/SEBASTIAN_TESIS/data_set/volts_veg.txt","a")
  datos.write(str(date)+" "+str(volts_dat[0])+" "+str(volts_dat[1])+" "
              +str(volts_dat[2])+" "+str(volts_dat[3])+" "+delta+"\n")
  datos.close()
  datos=open("/home/pi/Desktop/SEBASTIAN_TESIS/data_set/voltsk_veg.txt","a")
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
                
                
          now=datetime.now()  
          delta=str(now-init_t)
          write_intxt(str(now),sens_adc,sens_volt,sens_voltK,delta)
            
              # Mostramos el valor leido y eliminamos el salto de linea del final
          print ("Valor Arduino: " + str(vals))
          print ("sensores: " + str(sens_adc))
          print ("sensores volt: " + str(sens_volt))
          print ("sensores volt: " + str(sens_voltK))
          time.sleep(100)
          
          PuertoSerie.close()
      except:
        print("no data")
    except serial.SerialException:
   #-- Error al abrir el puerto serie
      sys.stderr.write("Error al abrir puerto (%s)\n" % str(Puerto))
      sys.exit(1)
    