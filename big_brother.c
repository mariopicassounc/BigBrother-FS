#include "big_brother.h"
#include "fat_volume.h"
#include "fat_table.h"
#include "fat_util.h"
#include <stdio.h>
#include <string.h>

int bb_is_log_file_dentry(fat_dir_entry dir_entry) {
    return strncmp(LOG_FILE_BASENAME, (char *)(dir_entry->base_name), 3) == 0 &&
           strncmp(LOG_FILE_EXTENSION, (char *)(dir_entry->extension), 3) == 0;
}

int bb_is_log_filepath(char *filepath) {
    return strncmp(BB_LOG_FILE, filepath, 8) == 0;
}

int bb_is_log_dirpath(char *filepath) {
    return strncmp(BB_DIRNAME, filepath, 15) == 0;
}

// /* Searches for a cluster that could correspond to the bb directory and returns
//  * its index. If the cluster is not found, returns 0.
//  */
// u32 search_bb_orphan_dir_cluster() {
//     errno = 0;
//     fat_volume vol = NULL;
//     u32 bb_dir_start_cluster = 0;

//     vol = get_fat_volume();

//     bb_dir_start_cluster = fat_table_get_next_bad_cluster(vol->table);
    
//     if (bb_dir_start_cluster == FAT_CLUSTER_END_OF_CHAIN_MAX){
//         bb_dir_start_cluster = 0;
//     }

//     return bb_dir_start_cluster;
// }

// /* Creates the /bb directory as an orphan and adds it to the file tree as 
//  * child of root dir.
//  */
// int bb_init_log_dir(u32 start_cluster) {
//     errno = 0;
//     fat_volume vol = NULL;
//     fat_tree_node root_node = NULL;

//     vol = get_fat_volume();

//     // Create a new file from scratch, instead of using a direntry like normally done.
//     fat_file loaded_bb_dir = fat_file_init_orphan_dir(BB_DIRNAME, vol->table, start_cluster);

//     // Add directory to file tree. It's entries will be like any other dir.
//     root_node = fat_tree_node_search(vol->file_tree, "/");
//     vol->file_tree = fat_tree_insert(vol->file_tree, root_node, loaded_bb_dir);

//     return -errno;
// }

// int bb_init_log() {
//     errno = 0;

//     u32 start_cluster = search_bb_orphan_dir_cluster();

//     /*  ACA FALTA 
//         "Su primera entrada de directorio es un archivo con nombre fs.log"
//         de alguna manera hay que obtener dentry y pasarsela a la funcion bb_is_log_file_dentry
        
//         mas o menos seria:
//         1) obtener root del arbol
//         2) obtener el dentry offset con la funcion fat_table_cluster_offset
//         3) de alguna manera obtener la entry (usando el file que se obtendria de root?)
//         4) chequear con bb_is_log_file_dentry, ie agregar esa condicion al if de aca abajo
//     */
//     // If it is currently not created
//     if (start_cluster == 0){
//         start_cluster = fat_table_get_next_free_cluster();
//     }

//     errno = bb_init_log_dir(start_cluster);

//     return -errno;
// }