#define _GUN_SOURCE
#define _DEFAULT_SOURCE

#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libgen.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

pthread_mutex_t q_lock;
pthread_mutex_t activies_lock;
pthread_mutex_t found_counter_lock;
pthread_cond_t cond;
char* term;
long* active_threads;
int num_threads;
int num_of_files_found;
int num_of_active_threads;
int sigint_flag;
int error_exit;
struct stat dir_stat;

/*------------------- queue implementation --------------------*/ 
typedef struct Queue_node {
    char* path; 
    struct Queue_node *next; 
} queue_node;

typedef struct Queue {
    struct Queue_node *front, *rear; 
} queue;
  
queue_node* newNode(char* path) {
    queue_node* temp = (queue_node*) malloc(sizeof(queue_node)); 
    temp->path = path; 
    temp->next = NULL; 
    return temp; 
}

queue* createQueue() {
    queue* q = (queue*)malloc(sizeof(queue)); 
    q->front = NULL;
    q->rear = NULL; 
    return q; 
}

int is_empty(queue* q){
    if (q->front == NULL){
        return 1;
    }
    return 0;
}

void enqueue(queue* q, char* path) {  
    queue_node* temp;
    temp = newNode(path); 
  
    /* If queue is empty, then new node is front and rear both */
    if (is_empty(q)) {
        q->front = temp;
        q->rear = temp;
        return;
    }
    (q->rear)->next = temp; 
    q->rear = temp; 
}

char* dequeue(queue* q) { 
    /* If queue is empty, return NULL. */
    if (is_empty(q)) {
        return NULL;
    }
    char * path;
    queue_node* temp;
    
    temp = q->front; 
    path = strdup(temp->path);
    
    q->front = (queue_node*) (q->front)->next; 
  
    /* If front becomes NULL, then change rear also as NULL */
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);     
    return path; 
}

queue* q;

/*----------------------------------------------------------------*/

void handle_sigint(){
    long thread_id;
    int t;
    sigint_flag = 1;
    for(t=0; t<num_threads; t++){
        thread_id = active_threads[t];
        if(thread_id !=0){
            pthread_cancel(thread_id);    
        }
    }
    pthread_cond_broadcast(&cond);
}

char* concat(char* path, char* file_name){
    char *result;
    const char *last_char;
    last_char = path + strlen(path)-1;
    result = malloc(strlen(path) + strlen(file_name) + 2);
    if(result == NULL){
        fprintf(stderr,"ERROR: malloc failed while concat path with file name\n"); 
        return NULL;
    }
    strcpy(result, path);
    if(0 != strcmp(last_char,"/")){
        strcat(result, "/");
    }
    strcat(result, file_name);
    return result;
}
    

void* find_term(void* t){
    char *path, *subdir_addr, *compare_result;
    struct dirent *entry;
    long my_id = (long)t;
    active_threads[my_id] = pthread_self();
    if(sigint_flag){
        pthread_testcancel();
    }
    signal(SIGINT, handle_sigint);
    while(1){
        if(sigint_flag){
            pthread_testcancel();
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        pthread_mutex_lock(&q_lock);
        while (is_empty(q)) {
            pthread_mutex_lock(&activies_lock);
            num_of_active_threads--;
            /* the last active thread left, so he wake up all the sleeping thread and exit */
            if(num_of_active_threads == 0){
                pthread_mutex_unlock(&activies_lock);
                pthread_mutex_unlock(&q_lock);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                pthread_cond_broadcast(&cond);
                active_threads[my_id] = 0;
                pthread_exit((void*)0);
            } else {
                pthread_mutex_unlock(&activies_lock);
                pthread_cond_wait(&cond, &q_lock);   
                pthread_mutex_unlock(&q_lock);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                if(sigint_flag){
                    pthread_testcancel();
                }  
                pthread_mutex_lock(&activies_lock);
                num_of_active_threads++;
                pthread_mutex_unlock(&activies_lock);
                pthread_mutex_lock(&q_lock);
            }
        }
        path = dequeue(q);
        pthread_mutex_unlock(&q_lock);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);   
        DIR *dr = opendir(path); 
        if (dr == NULL) {
            fprintf(stderr,"ERROR: Could not open %s directory, thread exit\n", path); 
            error_exit++;
            pthread_exit((void*)1);
        }
        while ((entry = readdir(dr)) != NULL) {
            if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) {
                continue;
            }
            if(entry->d_type == DT_DIR){
                subdir_addr = concat(path, entry->d_name);
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                pthread_mutex_lock(&q_lock);
                enqueue(q, subdir_addr);
                pthread_mutex_unlock(&q_lock);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                pthread_cond_broadcast(&cond);
            }
            else{
                compare_result = strstr(entry->d_name, term);
                if(NULL != compare_result){
                    subdir_addr = concat(path, entry->d_name);
                    if(subdir_addr == NULL){
                        error_exit++;
                        pthread_exit((void*)1); 
                    }
                    printf("%s\n",subdir_addr);
                    pthread_mutex_lock(&found_counter_lock);
                    num_of_files_found++;
                    pthread_mutex_unlock(&found_counter_lock);
                    free(subdir_addr);
                }
            }
        }
        closedir(dr);
        free(path);
    }
}

int main (int argc, char *argv[]) {
    char* root, *p;
    int thread_result;
    long t;
    
/*--------------- validate the input ---------------*/ 
    if(argc != 4) {
		exit(1);
    }
    root = (char*) argv[1];
    if ((stat(root, &dir_stat) < 0) || !S_ISDIR(dir_stat.st_mode)) {
		exit(1);
    }
/* ----- initilize the variables and the locks -----*/
    term = argv[2];
    num_threads = strtol(argv[3], &p, 10);
    q = createQueue();
    enqueue(q,root);
    pthread_t thread[num_threads];
    
    pthread_mutex_init(&q_lock, NULL);
    pthread_mutex_init(&activies_lock, NULL);
    pthread_mutex_init(&found_counter_lock, NULL);
    pthread_cond_init (&cond, NULL);
    
    sigint_flag = 0;
    num_of_files_found = 0;
    error_exit = 0;
    num_of_active_threads = 0;
    active_threads = (long*)calloc(num_threads, sizeof(long));
/*--------------------------------------------------*/

	for(t=0; t<num_threads; t++) {
        num_of_active_threads++;
		thread_result = pthread_create(&thread[t], NULL, find_term, (void*)t); 
		if (thread_result){
			 printf("$$$$$$$$$$4################33exit\n");
		}
	}
	for(t=0; t<num_threads; t++){
		thread_result = pthread_join(thread[t], NULL);
		if (thread_result){
			printf("$$$$$$$$$$$$$$$4exit\n");
		}
	}
/* ----------- free and destroy the locks ------------*/
    free(active_threads);
    pthread_mutex_destroy(&q_lock);
    pthread_mutex_destroy(&found_counter_lock);
    pthread_mutex_destroy(&activies_lock);
    pthread_cond_destroy(&cond);
    
    if(sigint_flag == 1){
        printf("Search stopped, found %d files\n", num_of_files_found);
    }
    else {
        printf("Done searching, found %d files\n", num_of_files_found);
        if (error_exit == num_threads){
            pthread_exit((void*)1);
        }
    }
    pthread_exit((void*)0);
}