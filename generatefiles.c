// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <string.h>
// #include <fcntl.h>
// #include <time.h>
// #include <sys/time.h>

// #define MIN_FILE_SIZE  100*1024
// #define MAX_FILE_SIZE  120*1024
// #define MAX_LINES 50

// int main() {

//     srand(time(NULL));
//     int startport = 6000;
//     const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

//     for(int i=0;i<24;i++){
//         printf("Generating file %d\n",i);
//         char filename[100];
//         sprintf(filename,"%d.%s",startport+i,"txt");
//         int fp = open(filename,O_CREAT | O_WRONLY | O_TRUNC, 0666);
//         int file_size = MIN_FILE_SIZE + (rand()%(MAX_FILE_SIZE-MIN_FILE_SIZE))+1;
//         printf("%d\n",file_size);
//         int curr_size=0;
//         int line_count=0;

//         while(curr_size<file_size){
//             char random_char = charset[rand()%strlen(charset)];
//             char* buff = &random_char;
//             write(fp,buff,1);
//             curr_size++;
//             if ((rand() % 97) == 0 && line_count < MAX_LINES) {
//                 write(fp, "\n", 1);
//                 line_count++;
//                 curr_size++;
//             }
//         }
//         fsync(fp);
//         close(fp);

//     printf("Completed generating the random files\n");
//     return 0;
//     }
// }