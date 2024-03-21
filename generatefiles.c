#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MIN_STRINGS 50    //minimum number of strings in the file
#define MAX_STRINGS 90    //maximum number of strings in the file


// Function to generate a random string
void generateRandomString(char *str, int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
    for (int i = 0; i < length-1; ++i) {
        int index = rand() % (strlen(charset));
        str[i] = charset[index];
    }
    str[length-1]='\n';
}

int main() {
    srand(time(NULL)); // Seed the random number generator

    int startport = 6000;
    for(int i=0;i<24;i++){
        char filename[100];
        sprintf(filename,"%d.%s",startport+i,"txt");
        FILE* fp = fopen(filename,"w");

        int num_strings = MIN_STRINGS + (rand()%(MAX_STRINGS-MIN_STRINGS));

        for(int j=0;j<num_strings;j++){
                int string_len = 1024;
                char random_string[string_len];
                generateRandomString(random_string, string_len);
                fwrite(random_string,1,1024,fp);
        }
        fclose(fp);
    }
    return 0;
}