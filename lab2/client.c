#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_transfer.h"

#define REQUIRED_ARGS 4

int main(int argc, char *argv[])
{
    CLIENT *cl;
    file_result *result;
    FILE *fp;
    size_t written;
    
    /* Check arguments */
    if (argc != REQUIRED_ARGS) {
        fprintf(stderr, "Usage: %s <host> <remote_file> <local_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    /* Initialize RPC client */
    cl = clnt_create(argv[1], FILE_TRANSFER_PROG, FILE_TRANSFER_VERS, "tcp");
    if (cl == NULL) {
        clnt_pcreateerror(argv[1]);
        return EXIT_FAILURE;
    }
    
    /* Call remote procedure */
    result = get_file_1(&argv[2], cl);
    if (result == NULL) {
        clnt_perror(cl, "call failed");
        clnt_destroy(cl);
        return EXIT_FAILURE;
    }
    
    /* Check result status */
    if (result->status != 0) {
        fprintf(stderr, "Error: status %d\n", result->status);
        clnt_destroy(cl);
        return EXIT_FAILURE;
    }
    
    /* Write file to disk */
    fp = fopen(argv[3], "wb");
    if (fp == NULL) {
        perror("fopen");
        clnt_destroy(cl);
        return EXIT_FAILURE;
    }
    
    written = fwrite(result->data.filedata_t_val, 1, 
                     result->data.filedata_t_len, fp);
    fclose(fp);
    
    if (written != result->data.filedata_t_len) {
        fprintf(stderr, "Write error: %zu/%u bytes\n", 
                written, result->data.filedata_t_len);
        clnt_destroy(cl);
        return EXIT_FAILURE;
    }
    
    printf("File saved: %u bytes -> %s\n", 
           result->data.filedata_t_len, argv[3]);
    
    clnt_destroy(cl);
    return EXIT_SUCCESS;
}
