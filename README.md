# Informe Laboratorio 4: BigBrother FS

Grupo 22
Mario Picasso
Mateo Malpassi
Facundo Coria

## Preguntas:

>1. Cuando se ejecuta el main con la opción -d, ¿qué se está mostrando en la pantalla?

Cuando ejecutamos el main con la opción -d se muestra en pantalla todos los mensajes seteados por DEBUG y los provenientes de Fuse.

>2. ¿Hay alguna manera de saber el nombre del archivo guardado en el cluster 157?

Se puede. Para hacerlo, se debería recorrer desde el directorio root (leyendo todos los archivos) hasta que una dentry marque el cluster 157. Luego, si ese cluster no es EOF (no contiene la totalidad del archivo), deberíamos seguir con la cadena de clusters que tengan los datos faltantes del archivo.

>3. ¿Dónde se guardan las entradas de directorio? ¿Cuántos archivos puede tener adentro un directorio en FAT32

En el sector de datos de FAT32 hay clusters de datos y clusters de entradas de directorio, en estos últimos se guardan las entradas de directorio.

Los clusters de las entradas de directorio tienen 512 bytes, y cada entrada es de 32 bytes, por lo tanto dentro de un directorio en FAT32 puede haber 16 entradas de directorios, lo que serían 16 archivos por directorio.

>4. Cuando se ejecuta el comando como ls -l, el sistema operativo, ¿llama a algún programa de usuario? ¿A alguna llamada al sistema? ¿Cómo se conecta esto con FUSE? ¿Qué funciones de su código se ejecutan finalmente?



>5. ¿Por qué tienen que escribir las entradas de directorio manualmente pero no tienen que guardar la tabla FAT cada vez que la modifican?

Las entradas de directorio están almacenadas en la imagen de la FAT, la cual vive en el disco. Esta imagen se lee una vez y se la mapea a memoria RAM, por lo tanto cuando hacemos alguna modificación (ie cambiamos el nombre) lo que en realidad estamos modificando es la tabla mapeada en memoria. Si queremos guardar en disco los cambios que hicimos tenemos que hacerlo mediante la función 
static void write_dir_entry(fat_file parent, fat_file file).


>6. Para los sistemas de archivos FAT32, la tabla FAT, ¿siempre tiene el mismo tamaño? En caso de que sí, ¿qué tamaño tiene?

La tabla FAT tiene siempre el mismo tamaño (aunque depende en cual unidad de almacenamiento se implemente, ej: en un disco de 1TB será más grande que en un pendrive). En el caso de la FAT con la que trabajamos nosotros, notamos que en su estructura el número de clusters está dado por un u32 (unsigned int de 32 bits), con lo cual tendríamos 2^32 aproximadamente. Luego a este número lo multiplicamos por el tamaño de cada cluster (512 Bytes) y obtenemos el tamaño total de la tabla FAT.
