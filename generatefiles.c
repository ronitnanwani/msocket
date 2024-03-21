#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAXLENGTH 10        //max length of each string
#define MIN_STRINGS 4500    //minimum number of strings in the file
#define MAX_STRINGS 9000    //maximum number of strings in the file


// Function to generate a random string
void generateRandomString(char *str, int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`~!@#$^&*()_-+=|/.,><";
    for (int i = 0; i < length; ++i) {
        int index = rand() % (strlen(charset));
        str[i] = charset[index];
    }
    str[length] = '\0';
}

int main() {
    srand(time(NULL));

    int startport = 6000;
    for(int i=0;i<24;i++){
        char filename[100];
        sprintf(filename,"%d.%s",startport+i,"txt");
        FILE* fp = fopen(filename,"w");

        int num_strings = MIN_STRINGS + (rand()%(MAX_STRINGS-MIN_STRINGS));

        for(int j=0;j<num_strings;j++){
                int string_len = 1+rand()%MAXLENGTH;
                char random_string[string_len+1];
                generateRandomString(random_string, string_len);
                fprintf(fp, "%s ", random_string);
                if(((j+1)%50) == 0){
                    fprintf(fp,"%s","\n");      //print newline after every 50 strings
                }
        }
        fclose(fp);
    }
    return 0;
}