# Informe Laboratorio 4: BigBrother FS

- Grupo 22
- Mario Picasso
- Mateo Malpassi
- Facundo Coria

## Preguntas:

>1. Cuando se ejecuta el main con la opción -d, ¿qué se está mostrando en la pantalla?

Cuando ejecutamos el Main con la opción -d, se muestran en pantalla las llamadas a las fuse_operations que va haciendo cada operación que realizamos en la imagen montada.

>2. ¿Hay alguna manera de saber el nombre del archivo guardado en el cluster 157?

Se puede. Para hacerlo, se debería recorrer desde el directorio root (leyendo todos los archivos) hasta que una dentry marque el cluster 157. Luego, si ese cluster no es EOF (no contiene la totalidad del archivo), deberíamos seguir con la cadena de clusters que tengan los datos faltantes del archivo.

>3. ¿Dónde se guardan las entradas de directorio? ¿Cuántos archivos puede tener adentro un directorio en FAT32

En el sector de datos de FAT32 hay clusters de datos y clusters de entradas de directorio, en estos últimos se guardan las entradas de directorio.

Los clusters de las entradas de directorio tienen 512 bytes, y cada entrada es de 32 bytes, por lo tanto dentro de un directorio en FAT32 puede haber 16 entradas de directorios, lo que serían 16 archivos por directorio.

>4. Cuando se ejecuta el comando como ls -l, el sistema operativo, ¿llama a algún programa de usuario? ¿A alguna llamada al sistema? ¿Cómo se conecta esto con FUSE? ¿Qué funciones de su código se ejecutan finalmente?

"ls -l" no realiza ninguna llamada a programa de usuario, ls mismo es un programa de usuario. Sí utiliza las syscalls: opendir, readdir, getattr y releasedir

Una vez que el sistema está corriendo, cada vez que se invoca una llamada a
sistema que involucra al filesystem, el SO transfiere la llamada a FUSE, que busca en la estructura fuse_operations la función que realiza la tarea de la syscall requerida. Estas funciones están implementadas en `fat_fuse_ops.c`.

En este programa de usuario se ejecutan las funciones fat_fuse_opendir, fat_fuse_getattr, fat_fuse_readdir, fat_fuse_releasedir.

>5. ¿Por qué tienen que escribir las entradas de directorio manualmente pero no tienen que guardar la tabla FAT cada vez que la modifican?

Las entradas de directorio están almacenadas en la imagen de la FAT, la cual vive en el disco. Esta imagen se lee una vez y se la mapea a memoria RAM, por lo tanto cuando hacemos alguna modificación (ie cambiamos el nombre) lo que en realidad estamos modificando es la tabla mapeada en memoria. Si queremos guardar en disco los cambios que hicimos tenemos que hacerlo mediante la función 
static void write_dir_entry(fat_file parent, fat_file file).

>6. Para los sistemas de archivos FAT32, la tabla FAT, ¿siempre tiene el mismo tamaño? En caso de que sí, ¿qué tamaño tiene?

El tamaño de la tabla FAT depende del tamaño de la imágen. Una vez creada la imágen, ese tamaño se mantiene fijo.
