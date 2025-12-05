#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "file_transfer.h"

file_result *get_file_1_svc(filename_t *name, struct svc_req *rqstp)
{
    static file_result res;
    FILE *f;
    long len;
    char *buf;
    size_t n;
    
    /* Clean up previous call */
    if (res.data.filedata_t_val) {
        free(res.data.filedata_t_val);
        res.data.filedata_t_val = NULL;
        res.data.filedata_t_len = 0;
    }
    
    res.status = 0;
    res.data.filedata_t_val = NULL;
    res.data.filedata_t_len = 0;
    
    printf("Request: %s\n", *name);
    
    /* Open file */
    f = fopen(*name, "rb");
    if (!f) {
        perror("fopen");
        res.status = errno;
        return &res;
    }
    
    /* Get size */
    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        res.status = errno;
        fclose(f);
        return &res;
    }
    
    len = ftell(f);
    if (len < 0 || len > MAXFILESIZE) {
        fprintf(stderr, "Invalid size: %ld\n", len);
        res.status = (len < 0) ? errno : EFBIG;
        fclose(f);
        return &res;
    }
    
    /* Allocate buffer */
    buf = malloc(len);
    if (!buf) {
        perror("malloc");
        res.status = ENOMEM;
        fclose(f);
        return &res;
    }
    
    /* Read file */
    rewind(f);
    n = fread(buf, 1, len, f);
    fclose(f);
    
    if (n != (size_t)len) {
        perror("fread");
        free(buf);
        res.status = EIO;
        return &res;
    }
    
    /* Set result */
    res.status = 0;
    res.data.filedata_t_val = buf;
    res.data.filedata_t_len = (u_int)len;
    
    printf("Sent: %u bytes\n", res.data.filedata_t_len);
    
    return &res;
}
