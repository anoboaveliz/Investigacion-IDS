# Investigacion-IDS
Este es un repositorio sobre los códigos creados y utilizados durante mi itinerario de investigación en los PAO I y II del año 2022.

## Códigos nodos
Esta carpeta contiene los códigos de las simulaciones realizadas en la investigación, estas fueron desarrolladas en Arduino IDE para los microcontroladores ESP8266. Tanto el esquema centralizado como el distribuido para funcionar tienen distintos requerimientos que serán explicados a continuación:

### Esquema centralizado
Este esquema solo requiere de 5 ESP8266 para su funcionamiento. Sin embargo, para su correcto funcionamiento se requiere que el nodo al que se le cargue el código con nombre "Broker" sea el primero en ser energizado y que el nodo al que se le cargue el código con nombre "dispositivo 5" sea el último en ser energizado, el orden de los demás nodos es indiferente. Los códigos "dispositivo 2", "dispositivo 3" y "dispositivo 4" son los nodos que simulan los sensores de la red y generan datos aleatorios para la simulación. Finalmente, el Código "dispositivo 5" simula ser un intruso en la red.

### Esquema distribuido
Este esquema requiere de un dispositivo que funcione como punto de acceso para que se conecten los ESP8266, puede ser un router inalámbrico o una raspberry. A diferencia del esquema anterior, no se requiere de ningún orden de energización en específico para el correcto funcionamiento.

## Librería de modelos
Esta carpeta contiene las librerías de cada modelo, el modelo con redes neuronales artificiales y el modelo con el algoritmo K-means. Ambos modelos pueden ser cargados a Arduino IDE para trabajar con las funciones de inferencia. Se puede seguir el siguiente tutorial para la instalación de librerías: https://programarfacil.com/blog/arduino-blog/instalar-una-libreria-de-arduino/

## Creador de archivos CSV
Este es un script escrito en Python para crear los datos que entrenarán y probarán al modelo, tanto datos normales como datos fuera del rango (anormales). Al ejecutar el script aparece un menú que guiará al usuario a la creación de los archivos que sean necesarios. NOTA: EL SCRIPT CREA LOS ARCHIVOS EN EL MISMO DIRECTORIO EN EL QUE SE ENCUENTRA Y TAMBIEN ELIMINA ARCHIVOS CUYO NOMBRE TERMINE EN ".csv".
