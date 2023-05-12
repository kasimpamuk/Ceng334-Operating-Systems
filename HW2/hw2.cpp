#include <pthread.h>
#include <semaphore.h>
#include <iostream>

int A_rows, A_cols, B_rows, B_cols, C_rows, C_cols, D_rows, D_cols;
int ** A, ** B, ** C, ** D, ** J, ** L, ** R;
//CREATE THREADS
pthread_t * JThreads;
pthread_t * LThreads;
pthread_t * RThreads;   

//CREATE SEMAPHORES
sem_t * JSemaphores;
sem_t * LSemaphores;
sem_t * RSemaphores;

//CREATE THREAD FUNCTIONS
int Sum_Thread_J(int row){
    for(int i = 0; i < A_cols; i++){
        J[row][i]= A[row][i] + B[row][i];
    }
    //sem_post(&JSemaphores[row]);
    return 0;
}
int Sum_Thread_L(int row){
    for(int i = 0; i < C_cols; i++){
        L[row][i]= C[row][i] + D[row][i];
    }
    //sem_post(&LSemaphores[row]);
    return 0;
}
int Multiply_Thread(int row){ //matrix multiplication J * L
    int sum;
    for(int i = 0; i < C_cols; i++){
        sum = 0;
        for(int j = 0; j < C_rows; j++){
            sum += J[row][j] * L[j][i];
        }
        R[row][i] =sum;
    }
    //sem_post(&RSemaphores[row]);
    return 0;
}


int main(){
    //TAKE INPUT
    std::cin >> A_rows >> A_cols;
    A = new int*[A_rows];
    for(int i = 0; i < A_rows; i++){
        A[i] = new int[A_cols];
    }
    for(int i = 0; i < A_rows; i++){
        for(int j = 0; j < A_cols; j++){
            std::cin >> A[i][j];
        }
    }
    std::cin >> B_rows >> B_cols;
    if(A_rows != B_rows || A_cols != B_cols){
        std::cout << "Invalid input" << std::endl;
        return 0;
    }
    B = new int*[B_rows];
    for(int i = 0; i < B_rows; i++){
        B[i] = new int[B_cols];
    }
    for(int i = 0; i < B_rows; i++){
        for(int j = 0; j < B_cols; j++){
            std::cin >> B[i][j];
        }
    }
    std::cin >> C_rows >> C_cols;
    if(A_cols != C_rows){
        std::cout << "Invalid input" << std::endl;
        return 0;
    }
    C = new int*[C_rows];
    for(int i = 0; i < C_rows; i++){
        C[i] = new int[C_cols];
    }
    for(int i = 0; i < C_rows; i++){
        for(int j = 0; j < C_cols; j++){
            std::cin >> C[i][j];
        }
    }
    std::cin >> D_rows >> D_cols;
    if(C_rows != D_rows || C_cols != D_cols){
        std::cout << "Invalid input" << std::endl;
        return 0;
    }
    D = new int*[D_rows];
    for(int i = 0; i < D_rows; i++){
        D[i] = new int[D_cols];
    }
    for(int i = 0; i < D_rows; i++){
        for(int j = 0; j < D_cols; j++){
            std::cin >> D[i][j];
        }
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

    //CREATE SEMAPHORES
    JSemaphores = new sem_t[A_rows];
    LSemaphores = new sem_t[C_rows];
    RSemaphores = new sem_t[A_rows];

    for(int i = 0; i < A_rows; i++){
        Sum_Thread_J(i);
        Sum_Thread_L(i);
    }
    for(int i = 0; i < A_rows; i++){
        Multiply_Thread(i);
    }

    //print J
    std::cout << "J" << std::endl;
    for(int i = 0; i < A_rows; i++){
        for(int j = 0; j < A_cols; j++){
            std::cout << J[i][j] << " ";
        }
        std::cout << std::endl;
    }




    return 0;
}