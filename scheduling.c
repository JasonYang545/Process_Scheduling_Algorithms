/*
AUTHOR: JASON YANG
CS202 - OPERATING SYSTEMS LAB 2 (SCHEDULING)
PROFESSOR ALLAN GOTTLIEB
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include<sys/wait.h>
#include <stddef.h>

#define MAX_LEN 50
#define MAX_LINES  100
typedef enum { false, true } bool;

// ////////////////////// QUEUE //////////////////////////////////////////////////////
typedef struct node {
    int val;
    struct node *next;
} node_t;

// function to add node to end of list
void enqueue(node_t **head, int val) {
    node_t *new_node = malloc(sizeof(node_t));
    node_t *last = *head;
    new_node->val  = val;
    new_node->next = NULL;
    if (*head == NULL){
       *head = new_node;
       return;
    }
    while (last->next != NULL) {
        last = last->next;
    }
    last->next = new_node;
    return;
}

// function to delete head node and return value stored
int dequeue(node_t **head) {
    struct node *toDelete;
    int retval=-1;
    if(*head == NULL) {
        return 100000;
    }
    toDelete = *head;
    retval=toDelete->val;
    free(toDelete);
    *head = (*head)->next;
    return(retval);
}

// function to delete a specific node value
void dequeueSpecific(node_t **head, int key){
    struct node *temp = *head, *prev;
    if (temp != NULL && temp->val == key) {
        *head = temp->next;
        free(temp);
        return;
    }
    while (temp != NULL && temp->val != key) {
        prev = temp;
        temp = temp->next;
    }
      if (temp == NULL) return;
    prev->next = temp->next;
    free(temp);  // Free memory
}

// function to print contents/nodes of list
void print_list(node_t *head) {
    node_t *current = head;
    while (current != NULL) {
        printf("%d", current->val);
        current = current->next;
    }
}

// search if any nodes of list contains desired data
int search(node_t **head, int key){
    struct node *current = *head;
    while (current!=NULL){
        if (current->val==key){
            return 1;
        }
        current=current->next;
    }
    return 0;
}

// delete node of a given data value
void deleteKey(node_t **head_ref, int key){
    struct node *temp = *head_ref, *prev;
    while (temp != NULL && temp->val == key) {
        *head_ref = temp->next;
        free(temp);
        temp = *head_ref;
    }
    while (temp != NULL) {
        while (temp != NULL && temp->val != key) {
            prev = temp;
            temp = temp->next;
        }
        if (temp == NULL){
            return;}

        prev->next = temp->next;
        free(temp); // Free memory

        temp = prev->next;
    }
}
///////////////////////// QUEUE //////////////////////////////////////////////////////

///////////////////////// STRING SLICE FUNCTION //////////////////////////////////////
// slices string with given input bounds
void slice(const char * str, char * buffer, size_t start, size_t end) {
    size_t j = 0;
    for ( size_t i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}
///////////////////////// STRING SLICE FUNCTION //////////////////////////////////////



///////////////////////////////// MAIN/DRIVER ////////////////////////////////////////
int main(int argc, char **argv) {
    // check if file given as command
    if (argc<=1) {
        printf("No file input");
        exit(0);
    }
    // open file from input
    FILE* file;
    file = fopen(argv[1], "r");
    // store the desired scheduling mode
    char* mode;
    mode=argv[2];

    // init output file
    FILE *output;

    // construct name of output file using input file name -> slice and concatenate
    const size_t len = strlen(argv[1]);
    char name[len+1];
    slice(argv[1],name,0,len-5);
    strcat(name,"-");
    strcat(name,mode);
    strcat(name,".txt");

    // init output file with constructed name -> check if error
    output = fopen(name, "w");
    if (output == NULL) {
        fprintf(stderr, "Can't open output file!\n");
        exit(1);
    }

    // get number of processes from first line
    int numProcess;
    fscanf(file,"%d", &numProcess);

    // init arrays used to store data of each array -> the index of each array corresponds to the corresponding process
    // i.e. cpu[3] stores the remaining cpu time for process 3
    char buffer[1000];
    // stores process IDs
    int id[MAX_LINES];
    // stores cpu times of all processes
    int cpu[MAX_LINES];
    // stores IO times of all processes
    int IO[MAX_LINES];
    // stores arrival times of all processes
    int arr[MAX_LINES];
    // stores the IO start times of all processes
    int IO_start[MAX_LINES];
    // stores status of all processes
    int done[MAX_LINES];
    // stores CPU burst cycles of all processes
    int elapsed[MAX_LINES];
    // stores start cycle of all processes
    int cycle_started[MAX_LINES];
    // stores cycle finished of all processes
    int cycle_finished[MAX_LINES];
    // used explicitly for round robin -> counts number of CPU bursts
    int burst_count[MAX_LINES];
    size_t lines = 0;

    // scrape data from file and store into appropriate arrays
    while (fgets(buffer, sizeof buffer, file) != NULL) {
        if (sscanf(buffer, "%d%d%d%d",
                   &id[lines], &cpu[lines], &IO[lines], &arr[lines]) == 4) {
            ++lines;
        }
    }
    // impute data for arrays which the file didnt have
    int k;
    for(k=0;k<numProcess;k++){
        // a process starts IO after half of its CPU time has elapsed
        IO_start[k]=(int)(cpu[k]/2);
        done[k]=0;
        elapsed[k]=0;
        cycle_started[k]=0;
        cycle_finished[k]=0;
        burst_count[k]=0;
    }
    fclose(file);

/////////////////////////////// FCFS //////////////////////////////////////////////////////////////////
    char* fcfs = "0";
    // check if mode asks for FCFS
    if (strcmp(fcfs,mode)==0){
        // init lists for 3 possible process states
        node_t *ready = NULL;
        node_t *running = NULL;
        node_t *blocked = NULL;

        int cycle; cycle=0;
        int togo; togo=numProcess;
        // if runningP==100000 then its NULL
        int i,runningP; runningP=100000;
        double cpu_working; cpu_working=0.0;

        // loop until all processes have been processed/terminated
        while(togo!=0){
            fprintf(output,"%d ", cycle);
            // at start of each cycle, update state of each process
            // for-loop ensures process hierarchy -> process of lower IDs have precedence over those with higher IDs
            for(i=0;i<numProcess;i++){
                // add process to ready if arrival time = current cycle
                if (arr[i]==cycle){
                    enqueue(&ready, i);
                    cycle_started[i]=cycle;
                }
                // process is blocked -> decrement IO time and if IO done, remove from blocked and enqueue into ready
                if (search(&blocked,i)==1){
                    IO[i]--;
                    if (IO[i]<=0){
                        dequeueSpecific(&blocked,i);
                        enqueue(&ready,i);
                    }
                }
                // time for process to start IO -> remove from running and add to blocked
                if (IO_start[i]==elapsed[i]){
                    if (search(&running,i)==1){
                     dequeueSpecific(&running,i);
                        enqueue(&blocked,i);
                    }
                }
                // if process is done, remove from running and take non-null process from ready and into running
                if (search(&running,i)==1){
                    if (cpu[i]<=0){
                        deleteKey(&running,i);
                        runningP=dequeue(&ready);
                        if (runningP!=100000){
                            enqueue(&running,runningP);
                        }
                        else{
                            runningP=100000;
                        }
                    }
                }
            }
            runningP=dequeue(&running);
            // if no process in running list, load wi first non-null process from ready list
            if ((runningP==100000)||(running==NULL)){
                runningP=dequeue(&ready);
                if (runningP!=100000){
                    enqueue(&running,runningP);
                }
                else{
                    runningP=100000;
                }
            }
            // write out to file the current state of each process in the system
            for(i=0;i<numProcess;i++){
                if (done[i]==0){
                    if (search(&blocked,i)==1){
                        fprintf(output,"%d:%s ",i,"blocked");
                    }
                    if (search(&ready,i)==1){
                        fprintf(output,"%d:%s ",i,"ready");
                    }
                    if (runningP==i){
                        fprintf(output,"%d:%s ",i,"running");
                    }
                }
            }
            // run the process in running -> remove from system if terminated
            if (runningP!=100000){
                cpu[runningP]--;
                elapsed[runningP]++;
                cpu_working=cpu_working+1.0;
                if (cpu[runningP]<=0){
                    cycle_finished[runningP]=cycle;
                    done[runningP]=1;
                    togo--;
                    dequeue(&running);
                }
                else{enqueue(&running,runningP);}
            }
            fprintf(output,"\n");
            cycle++;
        }
        // all processes terminated, write out statistics to file
        fprintf(output,"\n\nFinishing time:%d \n",cycle-1);
        fprintf(output,"CPU utilization: %.2f",(cpu_working/cycle));
        for(i=0;i<numProcess;i++){
            fprintf(output,"\nTurnaround process %d: %d",i,(cycle_finished[i]-(cycle_started[i])+1));
        }
        fclose(output);
    }
/////////////////////////////// FCFS //////////////////////////////////////////////////////////////////


/////////////////////////////// RR //////////////////////////////////////////////////////////////////
    char* rr = "1";
    // check if mode desires round robin
    if (strcmp(rr,mode)==0){
        // init 3 lists for 3 possible process states
        node_t *ready = NULL;
        node_t *running = NULL;
        node_t *blocked = NULL;

        int cycle; cycle=0;
        int togo; togo=numProcess;
        int i, runningP; runningP=100000;
        // quantum of 2
        int quantum; quantum=2;
        double cpu_working; cpu_working=0.0;
        bool finished; finished=false;
        int numDone;

        while (finished!=true){
            fprintf(output,"%d ", cycle);
            for(i=0;i<numProcess;i++){
                // add process to ready list if arrival time==cycle
                if (arr[i]==cycle){
                    enqueue(&ready, i);
                    cycle_started[i]=cycle;
                }
                // process is blocked so update -> remove from blocked and enqueue to ready if IO finished
                if (search(&blocked,i)==1){
                    IO[i]--;
                    if (IO[i]<=0){
                        // didn't use dequeue head because each process in blocked have different IO times
                        dequeueSpecific(&blocked,i);
                        enqueue(&ready,i);
                    }
                }
                // time for IO -> dequeue from running and place into blocked list
                if (IO_start[i]==elapsed[i]){
                    if (search(&running,i)==1){
                        deleteKey(&running,i);
                        enqueue(&blocked,i);
                        runningP=100000;
                    }
                }
                // process is in running queue, check if done
                if (search(&running,i)==1){
                    if (cpu[i]<=0){
                        deleteKey(&running,i);
                        done[i]=1;
                        togo--;
                        runningP=100000;
                    }
                    // quantum expired -> no IO, not finished but quantum expired -> preempt
                    else if (burst_count[i]==quantum){
                        deleteKey(&running,i);
                        runningP=100000;
                        enqueue(&ready,i);
                    }
                }
            }
            // no process in running list -> load process from ready if not null
            if ((runningP==100000)||(running==NULL)){
                runningP=dequeue(&ready);
                if (runningP!=100000){
                    enqueue(&running,runningP);
                }
            }
            // write our process states to file
            for(i=0;i<numProcess;i++){
                if (done[i]==0){
                    if (search(&blocked,i)==1){
                        fprintf(output,"%d:%s ",i,"blocked");
                    }
                    if (search(&ready,i)==1){
                        fprintf(output,"%d:%s ",i,"ready");
                    }
                    if (runningP==i){
                        fprintf(output,"%d:%s ",i,"running");
                    }
                }
            }
            // run process in running list
            if (runningP!=100000){
                cpu[runningP]--;
                elapsed[runningP]++;
                burst_count[runningP]++;
                cpu_working=cpu_working+1.0;
                if (cpu[runningP]<=0){
                    cycle_finished[runningP]=cycle;
                    done[runningP]=1;
                    togo--;
                    dequeue(&running);
                }
                else{enqueue(&running,runningP);}
            }
            fprintf(output,"\n");
            cycle++;
            // check if all prrocesses have terminated
            numDone=0;
            for(i=0;i<numProcess;i++){
                if (done[i]==1){
                    numDone++;
                }
            }
            if (numDone==numProcess){
                finished=true;
            }
        }
        // all processes have terminated -> write out statistics to file
        fprintf(output,"\n\nFinishing time:%d\n",cycle-1);
        fprintf(output,"CPU utilization: %.2f",(cpu_working/cycle));
        for(i=0;i<numProcess;i++){
            fprintf(output,"\nTurnaround process %d: %d",i,(cycle_finished[i]-(cycle_started[i])+1));
        }
        fclose(output);
    }
/////////////////////////////// RR //////////////////////////////////////////////////////////////////


/////////////////////////////// SJF //////////////////////////////////////////////////////////////////
    char* sjf = "2";
    // check if mode desires shortest job first
    if (strcmp(sjf,mode)==0){
        // init 3 process state lists
        node_t *ready = NULL;
        node_t *running = NULL;
        node_t *blocked = NULL;

        int cycle; cycle=0;
        int togo; togo=numProcess;
        // if runningP==100000 then it NULL
        int i,runningP; runningP=100000;
        int minCPU;
        double cpu_working; cpu_working=0.0;
        int shortestP;

         while(togo!=0){
            minCPU=1000000;
            fprintf(output,"%d ", cycle);
            for(i=0;i<numProcess;i++){
                // time for process to enter system
                if (arr[i]==cycle){
                    enqueue(&ready, i);
                    cycle_started[i]=cycle;
                }
                // process is blocked so update -> place into ready when IO finished
                if (search(&blocked,i)==1){
                    IO[i]--;
                    if (IO[i]<=0){
                        dequeueSpecific(&blocked,i);
                        enqueue(&ready,i);
                    }
                }
                // time for IO -> remove from running and into blocked
                if (IO_start[i]==elapsed[i]){
                    if (search(&running,i)==1){
                        deleteKey(&running,i);
                        enqueue(&blocked,i);
                        runningP=100000; // IDK -> i think so because this means null
                    }
                }
                // process is in running queue, check if done
                if (search(&running,i)==1){
                    if (cpu[i]<=0){
                        deleteKey(&running,i);
                        done[i]=1;
                        togo--;
                        runningP=100000;
                    }
                }
            }
            // after loop, shortestP stores which one should go next
            // loop computes the cpu tiems of all process in ready and running list
            shortestP=100000;
            for(i=0;i<numProcess;i++){
                if ((search(&ready,i)==1)||(search(&running,i)==1)){
                    if (done[i]==0){
                        if (cpu[i]<minCPU){
                            minCPU=cpu[i];
                            shortestP=i;
                        }
                    }
                }
            }
            // shortestP < runningP so preempt runningP and run shortestP (dequeue from appopriate lsit)
            if (runningP!=shortestP){
                if (search(&running,runningP)==1){
                    deleteKey(&running,runningP);
                }
                if (runningP!=100000){
                    enqueue(&ready,runningP);
                }
                if (search(&ready,shortestP)==1){
                    deleteKey(&ready,shortestP);
                }
                runningP=shortestP;
                enqueue(&running,shortestP);
            }

            // PRINT INFORMATION
            for(i=0;i<numProcess;i++){
                if (done[i]==0){
                    if (search(&blocked,i)==1){
                        fprintf(output,"%d:%s ",i,"blocked");
                    }
                    if (search(&ready,i)==1){
                        fprintf(output,"%d:%s ",i,"ready");
                    }
                    if (runningP==i){
                        fprintf(output,"%d:%s ",i,"running");
                    }
                }
            }
            // run process in running list
            if ((runningP!=100000)&&(running!=NULL)){
                cpu[runningP]--;
                elapsed[runningP]++;
                cpu_working=cpu_working+1.0;
                if (cpu[runningP]<=0){
                    cycle_finished[runningP]=cycle;
                    done[runningP]=1;
                    togo--;
                    deleteKey(&running,runningP);
                    runningP=100000;
                }
                else{
                     deleteKey(&running,runningP);
                    enqueue(&running,runningP);

                }
            }
            fprintf(output,"\n");
            cycle++;
        }
        // write out statistics to file
        fprintf(output,"\n\nFinishing time:%d\n",cycle-1);
        fprintf(output,"CPU utilization: %.2f",(cpu_working/cycle));
        for(i=0;i<numProcess;i++){
            fprintf(output,"\nTurnaround process %d: %d",i,(cycle_finished[i]-(cycle_started[i])+1));
        }
        fclose(output);
    }
/////////////////////////////// SJF //////////////////////////////////////////////////////////////////
}
