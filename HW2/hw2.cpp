#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include "hw2_output.c"

int A_rows, A_cols, B_rows, B_cols, C_rows, C_cols, D_rows, D_cols;
int ** A, ** B, ** C, ** D, ** J, ** L, ** R;
int * L_columns;
//CREATE THREADS
pthread_t * JThreads;
pthread_t * LThreads;
pthread_t * RThreads;   
pthread_mutex_t * LMutexes;

//CREATE SEMAPHORES
sem_t * JSemaphores;
sem_t * LSemaphores;

using namespace std;

//CREATE THREAD FUNCTIONS
void* Sum_Thread_J(void* arg){
    int row = *((int *)arg);
    for(int i = 0; i < A_cols; i++){
        J[row][i]= A[row][i] + B[row][i];
        hw2_write_output(0, row+1, i+1 , J[row][i]);
    }
    sem_post(&JSemaphores[row]);
    delete (int *)arg;
    return NULL;
}
void* Sum_Thread_L(void* arg){
    int row = *((int *)arg);
    for(int i = 0; i < C_cols; i++){
        L[row][i]= C[row][i] + D[row][i];
        hw2_write_output(1, row+1, i+1 , L[row][i]);
        pthread_mutex_lock(&LMutexes[i]);
        L_columns[i]++;
        if(L_columns[i] == C_rows){
            sem_post(&LSemaphores[i]);
        }
        pthread_mutex_unlock(&LMutexes[i]);

    }
    delete (int *)arg;
    return NULL;
}
void* Multiply_Thread(void* arg){ //matrix multiplication J * L
    int row = *((int *)arg);
    int sum;
    sem_wait(&JSemaphores[row]);
    for(int i = 0; i < C_cols; i++){
        sum = 0;
        sem_wait(&LSemaphores[i]);
        sem_post(&LSemaphores[i]);
        for(int j = 0; j < C_rows; j++){
            sum += J[row][j] * L[j][i];
        }
        R[row][i] =sum;
        hw2_write_output(2, row+1, i+1 , sum);
    }
    delete (int *)arg;
    return NULL;
}

int main(){
    hw2_init_output();
    //TAKE INPUT
    cin >> A_rows >> A_cols;
    A = new int*[A_rows];
    for(int i = 0; i < A_rows; i++){
        A[i] = new int[A_cols];
    }
    for(int i = 0; i < A_rows; i++){
        for(int j = 0; j < A_cols; j++){
            cin >> A[i][j];
        }
    }
    cin >> B_rows >> B_cols;
    if(A_rows != B_rows || A_cols != B_cols){
        cerr << "Invalid input" << endl;
        return 0;
    }
    B = new int*[B_rows];
    for(int i = 0; i < B_rows; i++){
        B[i] = new int[B_cols];
    }
    for(int i = 0; i < B_rows; i++){
        for(int j = 0; j < B_cols; j++){
            cin >> B[i][j];
        }
    }
    cin >> C_rows >> C_cols;
    if(A_cols != C_rows){
        cerr << "Invalid input" << endl;
        return 0;
    }
    C = new int*[C_rows];
    for(int i = 0; i < C_rows; i++){
        C[i] = new int[C_cols];
    }
    for(int i = 0; i < C_rows; i++){
        for(int j = 0; j < C_cols; j++){
            cin >> C[i][j];
        }
    }
    cin >> D_rows >> D_cols;
    if(C_rows != D_rows || C_cols != D_cols){
        cerr << "Invalid input" << endl;
        return 0;
    }
    D = new int*[D_rows];
    for(int i = 0; i < D_rows; i++){
        D[i] = new int[D_cols];
    }
    for(int i = 0; i < D_rows; i++){
        for(int j = 0; j < D_cols; j++){
            cin >> D[i][j];
        }
    }

    L_columns = new int[C_cols];
    for(int i = 0; i < C_cols; i++){
        L_columns[i] = 0;
    }

    J = new int*[A_rows];  // A + B matrix
    for(int i = 0; i < A_rows; i++){
        J[i] = new int[A_cols];
    }
    L = new int*[C_rows];  // C + D matrix
    for(int i = 0; i < C_rows; i++){
        L[i] = new int[C_cols];
    }
    R = new int*[A_rows];  // J * L matrix
    for(int i = 0; i < A_rows; i++){
        R[i] = new int[C_cols];
    }

    //CREATE THREADS
    JThreads = new pthread_t[A_rows];
    LThreads = new pthread_t[C_rows];
    RThreads = new pthread_t[A_rows];

    LMutexes = new pthread_mutex_t[C_cols];
    for(int i = 0; i < C_cols; i++){
        pthread_mutex_init(&LMutexes[i], NULL);
    }
    //CREATE AND INITIALIZE SEMAPHORES
    JSemaphores = new sem_t[A_rows];    
    LSemaphores = new sem_t[C_cols];
    for(int i = 0; i < A_rows; i++){
        if (sem_init(&JSemaphores[i], 0, 0) != 0) {
            cerr << "sem_init error" << endl;
            return 1;
        }
    }
    for(int i = 0; i < C_cols; i++){
        if (sem_init(&LSemaphores[i], 0, 0) != 0) {
            cerr << "sem_init error" << endl;
            return 1;
        }
    }

    for(int i = 0; i < A_rows; i++){    // INITIALIZE THREADS        
        pthread_create(&JThreads[i], NULL, Sum_Thread_J, (void *) new int(i));
        pthread_create(&RThreads[i], NULL, Multiply_Thread, (void *) new int(i));
    }
    for(int i = 0; i < C_rows; i++){
        pthread_create(&LThreads[i], NULL, Sum_Thread_L, (void *) new int(i));
    }

    int ret;
    for(int i = 0; i < A_rows; i++){
        //cerr << "Main thread waiting for thread " << i <<" in J to finish..." << endl;
        ret = pthread_join(JThreads[i], NULL);
        if (ret != 0) {
            printf("pthread_join error: %d\n", ret);
            return 1;
        }

        //cerr << "Main thread waiting for thread " << i <<" in L to finish..." << endl;
        ret = pthread_join(RThreads[i], NULL);
        if (ret != 0) {
            printf("pthread_join error: %d\n", ret);
            return 1;
        }
    }
    for(int i = 0; i < C_rows; i++){
        //cerr << "Main thread waiting for thread " << i <<" in L to finish..." << endl;
        ret = pthread_join(LThreads[i], NULL);
        if (ret != 0) {
            printf("pthread_join error: %d\n", ret);
            return 1;
        }
    }

    //print R
    cerr << "R" << endl;
    for(int i = 0; i < A_rows; i++){
        for(int j = 0; j < C_cols; j++){
            cerr << R[i][j] << " ";
        }
        cerr << endl;
    }
    //  Destroy Mutexes
    for(int i = 0; i < C_cols; i++){
        pthread_mutex_destroy(&LMutexes[i]);
    }
    return 0;
}