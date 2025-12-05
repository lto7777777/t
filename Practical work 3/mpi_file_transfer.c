#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_FILE_SIZE 1048576
#define MSG_TAG_SIZE  100
#define MSG_TAG_FILE  200

/* Function to read file into buffer */
static int read_file_to_buffer(const char *filename, unsigned char **data, int *size)
{
    FILE *file_handle;
    long file_length;
    size_t bytes_read;
    
    file_handle = fopen(filename, "rb");
    if (file_handle == NULL) {
        perror("Failed to open input file");
        return -1;
    }
    
    if (fseek(file_handle, 0, SEEK_END) != 0) {
        perror("Failed to seek to end of file");
        fclose(file_handle);
        return -1;
    }
    
    file_length = ftell(file_handle);
    if (file_length < 0 || file_length > MAX_FILE_SIZE) {
        fprintf(stderr, "File size %ld is invalid (max: %d)\n", file_length, MAX_FILE_SIZE);
        fclose(file_handle);
        return -1;
    }
    
    rewind(file_handle);
    *data = (unsigned char *)malloc(file_length);
    if (*data == NULL) {
        perror("Memory allocation failed");
        fclose(file_handle);
        return -1;
    }
    
    bytes_read = fread(*data, 1, file_length, file_handle);
    fclose(file_handle);
    
    if (bytes_read != (size_t)file_length) {
        perror("Failed to read entire file");
        free(*data);
        *data = NULL;
        return -1;
    }
    
    *size = (int)file_length;
    return 0;
}

/* Function to write buffer to file */
static int write_buffer_to_file(const char *filename, const unsigned char *data, int size)
{
    FILE *file_handle;
    size_t bytes_written;
    
    file_handle = fopen(filename, "wb");
    if (file_handle == NULL) {
        perror("Failed to create output file");
        return -1;
    }
    
    bytes_written = fwrite(data, 1, size, file_handle);
    fclose(file_handle);
    
    if (bytes_written != (size_t)size) {
        fprintf(stderr, "Only wrote %zu of %d bytes\n", bytes_written, size);
        return -1;
    }
    
    return 0;
}

/* Sender process (rank 0) */
static void sender_process(const char *input_filename)
{
    unsigned char *file_data;
    int file_size;
    
    if (read_file_to_buffer(input_filename, &file_data, &file_size) != 0) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return;
    }
    
    printf("[Sender] Read %d bytes from '%s', transmitting to receiver...\n",
           file_size, input_filename);
    
    MPI_Send(&file_size, 1, MPI_INT, 1, MSG_TAG_SIZE, MPI_COMM_WORLD);
    MPI_Send(file_data, file_size, MPI_BYTE, 1, MSG_TAG_FILE, MPI_COMM_WORLD);
    
    printf("[Sender] Transmission completed successfully.\n");
    free(file_data);
}

/* Receiver process (rank 1) */
static void receiver_process(const char *output_filename)
{
    int file_size;
    unsigned char *file_data;
    MPI_Status status;
    
    MPI_Recv(&file_size, 1, MPI_INT, 0, MSG_TAG_SIZE, MPI_COMM_WORLD, &status);
    
    if (file_size <= 0 || file_size > MAX_FILE_SIZE) {
        fprintf(stderr, "[Receiver] Invalid file size received: %d\n", file_size);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return;
    }
    
    file_data = (unsigned char *)malloc(file_size);
    if (file_data == NULL) {
        perror("[Receiver] Memory allocation failed");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return;
    }
    
    MPI_Recv(file_data, file_size, MPI_BYTE, 0, MSG_TAG_FILE, MPI_COMM_WORLD, &status);
    
    if (write_buffer_to_file(output_filename, file_data, file_size) != 0) {
        free(file_data);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return;
    }
    
    printf("[Receiver] Successfully received %d bytes and saved to '%s'\n",
           file_size, output_filename);
    free(file_data);
}

int main(int argc, char *argv[])
{
    int process_rank, total_processes;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &total_processes);
    
    if (total_processes != 2) {
        if (process_rank == 0) {
            fprintf(stderr, "Error: This program requires exactly 2 MPI processes.\n");
            fprintf(stderr, "Usage: mpirun -np 2 %s <input_file> <output_file>\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
    if (argc != 3) {
        if (process_rank == 0) {
            fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
    if (process_rank == 0) {
        sender_process(argv[1]);
    } else {
        receiver_process(argv[2]);
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}
