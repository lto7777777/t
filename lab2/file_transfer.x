/* file_transfer.x : RPC interface specification for remote file retrieval */

const MAXFILESIZE = 1048576;   /* Maximum file size: 1MB */

typedef string filename_t<>;     /* Remote file path requested by client */
typedef opaque filedata_t<>;     /* File content as binary data array */

struct file_result {
    int status;       /* 0 = success, non-zero = error number */
    filedata_t data;  /* File data when status == 0 */
};

program FILE_TRANSFER_PROG {
    version FILE_TRANSFER_VERS {
        file_result GET_FILE(filename_t) = 1;
    } = 1;
} = 0x31234567;
