#include <stdio.h>

#define LINE_LENGTH 1024 // Adjust this according to the fixed length of your lines

int main() {
    FILE* file = fopen("6000.txt", "r"); // Open the file for reading in binary mode
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char line[LINE_LENGTH + 1]; // Buffer to store each line read from the file, plus space for null terminator
    size_t bytes_read; // Variable to store the number of bytes read

    // Read lines from the file until the end of file is reached
    while ((bytes_read = fread(line, sizeof(char), LINE_LENGTH, file)) > 0) {
        // Null-terminate the line
        line[bytes_read] = '\0';
        
        // Print or process the line here
        printf("%s", line);
    }

    // Close the file after reading
    fclose(file);

    return 0;
}