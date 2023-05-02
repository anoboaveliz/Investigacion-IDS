#creación de archivos csv, datos normales y datos anormales de húmedad y temperatura
temperaturaSuelo=[22.0,23.0,23.0,23.0,24.0,25.0,26.0,28.0,30.0,33.0,35.0,37.0,38.0,37.0,36.0,35.0,33.0,31.0,29.0,28.0,26.0,25.0,23.0,22.0]
humedadSuelo=[90.0, 88.0, 82.0, 78.0, 75.0, 73.0, 70.0, 68.0, 66.0, 65.0, 65.0, 60.0, 55.0, 50.0, 45.0, 80.0, 75.0, 70.0, 65.0, 60.0, 55.0, 50.0, 45.0, 90.0]

temperaturaAmbiente=[22.0,23.0,23.0,23.0,24.0,25.0,26.0,28.0,30.0,33.0,35.0,37.0,38.0,37.0,36.0,35.0,33.0,31.0,29.0,28.0,26.0,25.0,23.0,22.0]
humedadAmbiente=[80.0, 85.0, 90.0, 84.0, 80.0, 79.0, 78.0, 76.0, 72.0, 68.0, 65.0, 60.0, 55.0, 50.0, 48.0, 45.0, 48.0, 50.0, 55.0, 60.0, 68.0, 72.0, 75.0, 80.0]
iluminacion=[310.0, 320.0, 340.0, 370.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 900.0, 950.0, 1000.0, 950.0, 900.0, 800.0, 700.0, 600.0, 500.0, 350.0, 330.0, 310.0, 305.0, 300.0]

co2=[1000.0, 980.0, 930.0, 900.0, 800.0, 700.0, 700.0, 700.0, 800.0, 700.0, 650.0, 650.0, 600.0, 550.0, 500.0, 550.0, 600.0, 650.0, 750.0, 800.0, 800.0, 850.0, 900.0, 950.0]
presencia=[50,50,50,50,50,53,65,70,75,80,65,60,55,50,50,50,50,60,70,60,50,50,50,50]

limites=[[0.0,0.0],[0.0,0.0],[0.0,0.0],[0.0,0.0],[0.0,0.0],[0.0,0.0]]

import os
import csv
import glob
import random as r


def datoNormal(dato, hora):
    if (dato=="soil_temp"):
        if hora >= 20 or hora <6:
            multiplicador=0.9
        elif hora > 7 or hora <=19:
            multiplicador=1.1
        rstemp=round(r.gauss(temperaturaSuelo[hora],1.0),6)*multiplicador
        return rstemp

    elif (dato=="soil_humi"):
        rshumi=round(r.gauss(humedadSuelo[hora],1.0),6)*1.1
        return rshumi

    elif (dato=="envi_temp"):
        retemp=round(r.gauss(temperaturaAmbiente[hora],1.0),6)
        return retemp

    elif (dato=="envi_humi"):
        rehumi=round(r.gauss(humedadAmbiente[hora],1.0),6)
        return rehumi

    elif (dato=="illuminan"):
        rillum=round(r.gauss(iluminacion[hora],1.0),6)
        return rillum

    elif (dato=="co2_level"):
        rco2le=round(r.gauss(co2[hora],1.2),6)
        return rco2le

    elif (dato=="presence_"):
        rprese=round(r.uniform(0,presencia[hora]),6)+1
        if rprese>50: 
            presence=1
        else:
            presence=0
        return presence

    else:
        return 0.0

def datoAnormal(dato):
    if (dato=="soil_temp"):
        azar=r.randint(0,1)
        if azar==0:
            bajo=r.uniform(0,1)
            rstemp=limites[0][0]*bajo
        else:
            alto=r.uniform(1,3)
            rstemp=limites[0][1]*alto
        return rstemp

    elif (dato=="soil_humi"):
        azar=r.randint(0,1)
        if azar==0:
            bajo=r.uniform(0,1)
            rshumi=limites[1][0]*bajo
        else:
            alto=r.uniform(1,2)
            rshumi=limites[1][1]*alto
        return rshumi

    elif (dato=="envi_temp"):
        azar=r.randint(0,1)
        if azar==0:
            bajo=r.uniform(0,1)
            retemp=limites[2][0]*bajo
        else:
            alto=r.uniform(1,2)
            retemp=limites[2][1]*alto
        return retemp

    elif (dato=="envi_humi"):
        azar=r.randint(0,1)
        if azar==0:
            bajo=r.uniform(0,1)
            rehumi=limites[3][0]*bajo
        else:
            alto=r.uniform(1,2)
            rehumi=limites[3][1]*alto
        return rehumi

    elif (dato=="illuminan"):
        azar=r.randint(0,1)
        if azar==0:
            bajo=r.uniform(0,1)
            rillum=limites[4][0]*bajo
        else:
            alto=r.uniform(1,2)
            rillum=limites[4][1]*alto
        return rillum

    elif (dato=="co2_level"):
        azar=r.randint(0,1)
        if azar==0:
            bajo=r.uniform(0,1)
            rco2le=limites[5][0]*bajo
        else:
            alto=r.uniform(1,2)
            rco2le=limites[5][1]*alto
        return rco2le

    elif (dato=="presence_"):
        rprese=r.randint(0,1)
        return rprese

    else:
        return 0.0

def datosNormales(numArchivos):
    numArch=0
    nombreArch="normal.datosNormales"
    while (numArch<numArchivos):
        with open (nombreArch+str(numArch)+".csv", 'w', newline='') as csvfile:
            hora=0
            cabecera=["timestamp","soil_temp", "soil_humi", "envi_temp", "envi_humi", "illuminan", "co2_level", "presence_"]
            archivo=csv.DictWriter(csvfile, fieldnames=cabecera)
            archivo.writeheader()
            while hora<24:
                if hora >= 20 or hora <6:
                    multiplicador=0.9
                elif hora > 7 or hora <=19:
                     multiplicador=1.1
                rstemp=round(r.gauss(temperaturaSuelo[hora],1.0),6)*multiplicador
                rshumi=round(r.gauss(humedadSuelo[hora],1.0),6)*1.1
                retemp=round(r.gauss(temperaturaAmbiente[hora],1.0),6)
                rehumi=round(r.gauss(humedadAmbiente[hora],1.0),6)
                rillum=round(r.gauss(iluminacion[hora],1.0),6)
                rco2le=round(r.gauss(co2[hora],1.2),6)
                rprese=round(r.uniform(0,presencia[hora]),6)+1
                if rprese>50: 
                    presence=1
                else:
                    presence=0
                unix=hora*3600000
                archivo.writerow({'timestamp':unix,
                                'soil_temp':rstemp,
                                'soil_humi':rshumi,
                                'envi_temp':retemp,
                                'envi_humi':rehumi,
                                'illuminan':rillum,
                                'co2_level':rco2le,
                                'presence_':presence})
                hora=hora+1
            csvfile.close()
        numArch=numArch+1
    print("Archivos de datos normales creados")

def datosAnormales(numArchivos):
    numArch=0
    nombreArch="anormal.datosAnormales"
    while (numArch<numArchivos):
        with open (nombreArch+str(numArch)+".csv", 'w', newline='') as csvfile:
            hora=0
            cabecera=["timestamp","soil_temp", "soil_humi", "envi_temp", "envi_humi", "illuminan", "co2_level", "presence_"]
            cabeceraCopia=["soil_temp", "soil_humi", "envi_temp", "envi_humi", "illuminan", "co2_level", "presence_"]
            archivo=csv.DictWriter(csvfile, fieldnames=cabecera)
            archivo.writeheader()
            while hora<24:
                resultados=[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
                datosGenerados=0
                datosIncorrectos=r.randint(1,7)
                r.shuffle(cabeceraCopia)
                for i in cabeceraCopia:
                    if datosGenerados<datosIncorrectos:
                        resultados[cabecera.index(i)-1]=datoAnormal(i)
                    else:
                        resultados[cabecera.index(i)-1]=datoNormal(i,hora)
                unix=hora*3600000
                archivo.writerow({'timestamp':unix,
                                'soil_temp':resultados[0],
                                'soil_humi':resultados[1],
                                'envi_temp':resultados[2],
                                'envi_humi':resultados[3],
                                'illuminan':resultados[4],
                                'co2_level':resultados[5],
                                'presence_':resultados[6]})
                hora=hora+1
            csvfile.close()
        numArch=numArch+1
    print("Archivos de datos anormales creados")

def datoxdatoNormal(numArchivos):
    hora=0
    numArch=0
    nombreArch="datosNormales"
    while (numArch<numArchivos):
        with open (nombreArch+str(numArch)+".csv", 'w', newline='') as csvfile:
            cabecera=["soil_temp", "soil_humi", "envi_temp", "envi_humi", "illuminan", "co2_level", "presence_"]
            archivo=csv.DictWriter(csvfile, fieldnames=cabecera)
            archivo.writeheader()
            if hora >= 20 or hora <6:
                multiplicador=0.9
            elif hora > 7 or hora <=19:
                    multiplicador=1.1
            rstemp=round(r.gauss(temperaturaSuelo[hora],1.0),6)*multiplicador
            rshumi=round(r.gauss(humedadSuelo[hora],1.0),6)*1.1
            retemp=round(r.gauss(temperaturaAmbiente[hora],1.0),6)
            rehumi=round(r.gauss(humedadAmbiente[hora],2),6)
            rillum=round(r.gauss(iluminacion[hora],1.0),6)
            rco2le=round(r.gauss(co2[hora],1.2),6)
            rprese=round(r.uniform(0,presencia[hora]),6)+1
            if rprese>50: 
                presence=1
            else:
                presence=0
            archivo.writerow({'soil_temp':rstemp,
                            'soil_humi':rshumi,
                            'envi_temp':retemp,
                            'envi_humi':rehumi,
                            'illuminan':rillum,
                            'co2_level':rco2le,
                            'presence_':presence})
            if numArch==0:
                limites[0][0]=rstemp
                limites[0][1]=rstemp
                limites[1][0]=rshumi
                limites[1][1]=rshumi
                limites[2][0]=retemp
                limites[2][1]=retemp
                limites[3][0]=rehumi
                limites[3][1]=rehumi
                limites[4][0]=rillum
                limites[4][1]=rillum
                limites[5][0]=rco2le
                limites[5][1]=rco2le

            else:
                if rstemp<limites[0][0]:
                    limites[0][0]=rstemp
                elif rstemp>limites[0][1]:
                    limites[0][1]=rstemp
                
                if rshumi<limites[1][0]:
                    limites[1][0]=rshumi
                elif rshumi>limites[1][1]:
                    limites[1][1]=rshumi
                
                if retemp<limites[2][0]:
                    limites[2][0]=retemp
                elif retemp>limites[2][1]:
                    limites[2][1]=retemp
                
                if rehumi<limites[3][0]:
                    limites[3][0]=rehumi
                elif rehumi>limites[3][1]:
                    limites[3][1]=rehumi
                
                if rillum<limites[4][0]:
                    limites[4][0]=rillum
                elif rillum>limites[4][1]:
                    limites[4][1]=rillum
                
                if rco2le<limites[5][0]:
                    limites[5][0]=rco2le
                elif rco2le>limites[5][1]:
                    limites[5][1]=rco2le
                        
            hora=hora+1
            csvfile.close()
            numArch=numArch+1
            if hora>=24:
                hora=0
    print("Archivos de datos normales creados")

def datoxdatoAnormal(numArchivos):
    numArch=0
    nombreArch="datosAnormales"
    while (numArch<numArchivos):
        with open (nombreArch+str(numArch)+".csv", 'w', newline='') as csvfile:
            hora=0
            cabecera=["soil_temp", "soil_humi", "envi_temp", "envi_humi", "illuminan", "co2_level", "presence_"]
            cabeceraCopia=["soil_temp", "soil_humi", "envi_temp", "envi_humi", "illuminan", "co2_level", "presence_"]
            archivo=csv.DictWriter(csvfile, fieldnames=cabecera)
            archivo.writeheader()
            resultados=[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
            datosGenerados=0
            datosIncorrectos=r.randint(1,7)
            r.shuffle(cabeceraCopia)
            for i in cabeceraCopia:
                if datosGenerados<datosIncorrectos:
                    resultados[cabecera.index(i)]=datoAnormal(i)
                else:
                    resultados[cabecera.index(i)]=datoNormal(i,hora)
            archivo.writerow({'soil_temp':resultados[0],
                            'soil_humi':resultados[1],
                            'envi_temp':resultados[2],
                            'envi_humi':resultados[3],
                            'illuminan':resultados[4],
                            'co2_level':resultados[5],
                            'presence_':resultados[6]})
            hora=hora+1
            csvfile.close()
            numArch=numArch+1
    print("Archivos de datos anormales creados")

def borrarArchivos():
    archivos = glob.glob('*.csv')
    if len(archivos)>0:
        for archivo in archivos:
            os.remove(archivo)

def main():
    opcion=0
    while (opcion!='1' and opcion!='2' and opcion!='3'):
        print("Que archivo desea hacer?\n1:Dato por dato\n2:Borrar archivos csv")
        opcion=input('Opcion:')
    if opcion=='1':
        numArchivos=int(input("Cuantos archivos de datos normales desea crear?:"))
        datoxdatoNormal(numArchivos)
        print(limites)
        numArchivos=int(input("Cuantos archivos de datos anormales desea crear?:"))
        datoxdatoAnormal(numArchivos)
    elif opcion=='2':
        borrarArchivos()

if __name__ == "__main__":
    main()