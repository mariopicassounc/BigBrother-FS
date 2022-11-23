## Informe 
- Modo debug muestra los logs de fuse
- No se puede encontrar el nombre de archivo al que pertenece un cluster arbitrario salvo recorriendo todo el árbol de archivos e importantemente, para cada archivo recorrer cada cadena de clusters hasta llegar al que corresponde. Ustedes asumen que el cluster está en una dentry, es decir que es un `start_cluster`
- Los directorios son archivos y como tales tienen clusters de datos. Los datos de esos clusters son las dentrys. En un solo cluster de 512 bytes entran 16 dentrys pero un directorio puede tener más clusters asignados (en FAT32 porque en fat-fuse efectivamente los directorios tienen un máximo de 16 dentrys)
- `munmap` hace persistente el mapeo a memoria al desmontar el volumen
- En general el tamaño de la tabla fat es el necesario para indexar todos los clusters. O sea que tiene 4 bytes por cada cluster de datos


## Repo 
- OK

## Código
- OK

## Funcionalidad
- rmdir falla en verificar que el directorio esté vacío. 
- El directorio bb se puede inicializar tanto si ya existía como si no. El archivo fs.log solo se inicializa si no existía antes. Siempre que montan llaman a `bb_init_log_file` y eso hace que se reinicie y el log no persista. La guarda no sirve porque hace un search en el árbol cuando aún no está cargado fs.log en el árbol así que nunca lo encuentra. Una vez creado el logfile lo deberían meter en el árbol.