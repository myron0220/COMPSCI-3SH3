/*
 * MacId: wangm235, Name: Mingzhe Wang
 * MacId: li64, Name: Xing Li
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> /*For mmap() function*/
#include <string.h> /*For memcpy function*/
#include <fcntl.h> /*For file descriptors*/

#define INT_SIZE 4 // Size of integer in bytes
#define INT_COUNT 20 
#define MEMORY_SIZE INT_COUNT * INT_SIZE
#define CYL_LO 0
#define CYL_HI 299

int movements; // for counting total movements
int head_pos; // for the current head position
int start; // for the initial head position
int direction; // for scan direction, 0 for left and 1 for right

/* insertion sort from https://www.geeksforgeeks.org/insertion-sort/ */
void insertionSort(int arr[]) {
    int i, key, j;
    for (i = 1; i < INT_COUNT; i++) {
        key = arr[i];
        j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

/* get the largest index whose value <= the key in the sorted array
 * arr records the sorted requests
 */
int indexOf(int arr[], int key) {
    for (int i = 0; i < INT_COUNT; i++) {
        if (key < arr[i]) return i - 1;
    }
    return INT_COUNT - 1;
}

/* print requests in the served order and count the movements
 * arr records the requests in the served order
 * tag = 1 for SCAN, 2 for C-SCAN, 0 for other algorithms
 */
void print_result(int arr[], int direction, int tag) {
    movements = 0;
    head_pos = start;
    for (int i = 0; i < INT_COUNT; i++) {
        movements += abs(head_pos - arr[i]);
        if (tag != 0 && direction == 0 && arr[i] - head_pos > 0 && i < INT_COUNT - 1) {
            movements += 2 * (head_pos - CYL_LO); // scan to one side
            if (tag == 1) direction = 1; // change direction to right
            else movements += 2 * (CYL_HI - arr[i]); // scan to another side and keep direction left
        } // LEFT SCAN
        if (tag != 0 && direction == 1 && arr[i] - head_pos < 0 && i < INT_COUNT - 1) {
            movements += 2 * (CYL_HI - head_pos); // scan to one side
            if (tag == 1) direction = 0; // change direction to left
            else movements += 2 * (arr[i] - CYL_LO); // scan to another side and keep direction left
        } // RIGHT SCAN
        head_pos = arr[i]; 
        printf("%d", head_pos);
        if (i == INT_COUNT - 1)
            printf(" \n\n");
        else
            printf(", ");
    }
}

int main(int argc, char *argv[]) {
    int requests[INT_COUNT];        // original requests read from file
    int sortedrequests[INT_COUNT];  // requests sorted in increasing order
    int servedrequests[INT_COUNT];  // requests in the served order for SSTF SCAN LOOK
    int servedrequests2[INT_COUNT]; // requests in the served order for C-SCAN C-LOOK
    
    // validate arguments
    if (argc != 3) {
        printf("Arguments number error (2 required)\n");
        return -1;
    }
    
    start = atoi(argv[1]); 
    if (start < CYL_LO || start > CYL_HI) {
        printf("Init Pos out of range(%d-%d)\n", CYL_LO, CYL_HI);
        return -1;
    }
    if (strcmp(argv[2], "LEFT") == 0) direction = 0;
    else if (strcmp(argv[2], "RIGHT") == 0) direction = 1;
    else {
        printf("Unknown direction (LEFT or RIGHT expected\n");
        return -1;
    }

    // read the file and save to requests
    signed char *mmapfptr;
    int mmapfile_fd = open("request.bin", O_RDONLY);
    mmapfptr = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, mmapfile_fd, 0);
    for (int i = 0; i < INT_COUNT; i++) 
        memcpy(requests + i, mmapfptr + 4*i, INT_SIZE);
    munmap(mmapfptr, MEMORY_SIZE);

    // sort the requests in sortedrequests
    for (int i = 0; i < INT_COUNT; i++) sortedrequests[i] = requests[i];     
    insertionSort(sortedrequests);

    printf("Total requests = %d \n", INT_COUNT);
    printf("Initial Head Position: %d \n", start);
    printf("Direction of Head: %s \n\n", argv[2]);

    /*---------- FCFS ----------*/
    printf("FCFS DISK SCHEDULING ALGORITHM:\n\n");
    print_result(requests, direction, 0);
    printf("FCFS - Total head movements = %d \n\n", movements);


    /*---------- SSTF ----------*/
    int indexl = indexOf(sortedrequests, start); // index for the left closest
    int indexr = indexl + 1; // index for the right closest
    head_pos = start;
    for (int i = 0; i < INT_COUNT; i++) {
        if (indexl < 0) servedrequests[i] = sortedrequests[indexr++];
        else if (indexr == INT_COUNT) servedrequests[i] = sortedrequests[indexl--];
        else {            
            if (abs(sortedrequests[indexl] - head_pos) <= abs(sortedrequests[indexr] - head_pos))
              head_pos = sortedrequests[indexl--];
            else head_pos = sortedrequests[indexr++];
            servedrequests[i] = head_pos;
        }
    }
    printf("SSTF DISK SCHEDULING ALGORITHM \n\n");
    print_result(servedrequests, direction, 0);
    printf("SSTF - Total head movements = %d \n\n", movements);

    
    /*---------- SCAN LOOK ----------*/
    int index = indexOf(sortedrequests, start);
    int count = 0;
    if (direction == 0) { // LEFT
        for (int i = index; i >= 0; i--) servedrequests[count++] = sortedrequests[i];  
        for (int i = index + 1; i < INT_COUNT; i++) servedrequests[count++] = sortedrequests[i];
    } else { // RIGHT
        if (sortedrequests[index] == start) index--; // boundary case
        for (int i = index + 1; i < INT_COUNT; i++) servedrequests[count++] = sortedrequests[i];
        for (int i = index; i >= 0; i--) servedrequests[count++] = sortedrequests[i];
    }
    
    /*---------- C-SCAN C-LOOK ----------*/
    index = indexOf(sortedrequests, start);
    if (direction == 0) { // LEFT
        index += INT_COUNT; // always modulo a positive number
        for (int i = 0; i < INT_COUNT; i++) servedrequests2[i] = sortedrequests[(index--)%INT_COUNT];  
    } else { // RIGHT
        if (sortedrequests[index] != start) index++; // boundary case
        for (int i = 0; i < INT_COUNT; i++) servedrequests2[i] = sortedrequests[(index++)%INT_COUNT]; 
    }

    // print result for SCAN C-SCAN LOOK C-LOOK
    printf("SCAN DISK SCHEDULING ALGORITHM \n\n");
    print_result(servedrequests, direction, 1);
    printf("SCAN - Total head movements = %d \n\n", movements);
    
    printf("C-SCAN DISK SCHEDULING ALGORITHM \n\n");
    print_result(servedrequests2, direction, 2);
    printf("C-SCAN - Total head movements = %d \n\n", movements);

    printf("LOOK DISK SCHEDULING ALGORITHM \n\n");
    print_result(servedrequests, direction, 0);
    printf("LOOK - Total head movements = %d \n\n", movements);
    
    printf("C-LOOK DISK SCHEDULING ALGORITHM \n\n");
    print_result(servedrequests2, direction, 0);
    printf("C-LOOK - Total head movements = %d \n\n", movements);

    return 0;
}