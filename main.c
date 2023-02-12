#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>

int primeNumbersTillN(int n, int arr[]){
    bool prime[n+1];
    memset(prime, true, sizeof(prime));
    for(int k = 2; k*k<=n;k++){
        if(prime[k] == true){
            for(int j = k*k;j<=n; j+=k)
                prime[j] = false;
        }
    }
    int j =0;
    for(int i = 2; i<=n; i++){
        if(prime[i]){
            arr[j++]=i;
        }
    }
    return j;
}
bool validateX(int x, int a, int b){
    if(x>=a && x<=b)
        return true;
    return false;
}
int flag=0;
int child_pid[100]={0};
void handler(int sig){
    //printf("IN SIGNAL HANDLER\nValue of flag: %d\n",flag);
    if(flag == -1){
        perror("SIGCHILD detected early! Worker children terminated!!!\n\n");
        for(int ps=0;ps<10;ps++){
            if(child_pid>0)
                kill(child_pid[ps],SIGTERM);
        }
        exit(1);
    }
}

struct params {
    int j;
    int p;
    int x;
    int *pri;
    int sizeofpri;
    int size_px;
    int *px;
    int thapx;
};
void* runner(void* args){
    struct params *pi = (struct params*)args;
    //5. After the worker thread starts executing
    printf("Worker thread %d executing\n",pi->j);
    int p = pi->p; int x = pi->x;
    int* pri = pi->pri; int sizeofpri = pi->sizeofpri;
    int px[2*(pi->p)+1];
    int indx=0;
    for(int i =0;i<pi->sizeofpri;i++){
        if(pi->pri[i]>=x){
            indx=i;
            break;
        }
    }
    int start_indx=0,end_indx=0,k,pc;
    if(indx>=pi->p)
        start_indx=indx-pi->p;
    for(k = start_indx,pc=0;pi->pri[k]<=x;k++,pc++){
        px[pc]=pi->pri[k];
    }
    int prime_indx=pc;
    while(p--){
        if(k<pi->sizeofpri){
            px[pc++] = pi->pri[k++];
        }
    }
    //6. At each level of discovery of a prime number
    printf("For thread: %d\nPrimes before x: %d\nPrimes after x: %d\n",pi->j,indx-start_indx+1,pc-prime_indx);
    /// left to fix according to specifics


    pi->size_px=pc;
    //7. After all, px is calculated.
    printf("For thread: %d, all of px is generated succesfully!!!\n",pi->j);
    int sum=0;
    pi->px = (int*) malloc(sizeof(int)*(2*pi->p)+1);
    for(int l =0; l<pi->size_px;l++){
        pi->px[l] = px[l];
        sum+=px[l];
    }
    pi->thapx=sum/pc;
    //8. After thapx is calculated
    printf("For thread %d, thapx calculation is done!\n\n",pi->j);
}

int main(int argc,char** argv){
    // controller process with cmd line args handled
    if(argc < 5){
        perror("Invalid number of arguments!!!\n");
        return(EXIT_FAILURE);
    }
    int n = atoi(argv[1]);
    int a= atoi(argv[2]), b= atoi(argv[3]), p = atoi(argv[4]);
    if(argc >=5 && argc<(n*n)+5){
        perror("All elements not provided for the matrix!!!\n");
        return(EXIT_FAILURE);
    }
    // matrix initialised
    int mat[15][15],cnt = 5;
    for(int i = 0; i< n; i++){
        for(int j = 0; j<n; j++){
            mat[i][j] = atoi(argv[cnt++]);
        }
    }
    //1. After reading the input
    printf("INPUT MATRIX: \n");
    for(int i = 0; i< n; i++){
        for(int j = 0; j<n; j++){
            printf("%d ",mat[i][j]);
        }
        printf("\n");
    }

    //declare n file descripters for n pipes and prime array
    int primes[b];
    int sizeOfPrimes = primeNumbersTillN(b,primes);
    // add pipe error checks later
    int ptoc_fd[n][2];
    int ctop_fd[n][2];
    for(int i = 0; i<n; i++){
        if(pipe(ptoc_fd[i]) == -1){
            perror("Pipe from Parent to Child failed!!!\n");
            return (EXIT_FAILURE);
        }
    }
    for(int i = 0; i<n; i++){
        if(pipe(ctop_fd[i]) == -1){
            perror("Pipe from Parent to Child failed!!!\n");
            return (EXIT_FAILURE);
        }
    }
    //2. After creating the pipe and worker processes
    printf("Pipes created succesfully!!!\n\n");

    //result variables
    //////////////////////////////////////////
    int wpapx[n];
    int fapx;
    
    //spawn n child processes
    for(int i =0; i<n; i++){
        signal(SIGCHLD,handler);
        pid_t pro = fork();
        if(pro<0){
            perror("Failed to create child process\n");
            return(EXIT_FAILURE);
        }
        //parent process
        else if(pro>0){
            printf("Parent/controller id: %d process working\n",getpid());
            close(ptoc_fd[i][0]); // close reading end for parent
            write(ptoc_fd[i][1], mat[i],n*sizeof(int));
            close(ptoc_fd[i][1]); // close write end after writing

            wait(NULL); // waiting for child to finish its work
            //printf("updated value of temp: %d\n",temp);
            close(ctop_fd[i][1]); //close write end of child
            //int results[n],count=0;
            int wpap;
            read(ctop_fd[i][0], &wpap, sizeof(int));
            if(wpap==-1){
                flag=-1;
                exit(0);
            }
            //13. After Controller captures a wpapx
            printf("Controller received a wpapx for row %d\n\n",i);
            wpapx[i]=wpap;   
        }
        //worker child processes
        else{
            printf("Child/worker id: %d process working\n",getpid());
            child_pid[i]=getpid();
            close(ptoc_fd[i][1]); // close writing end of ith ptoc pipe
            int values[20];
            read(ptoc_fd[i][0], values,20*sizeof(int));
            close(ptoc_fd[i][0]); // close reading ends of both pipes
            close(ctop_fd[i][0]);
            for(int x = 0; x<n; x++){
                if(validateX(values[x],a,b) == false){
                    perror("Value of X is out of bounds!!!\n");
                    flag = -1;
                    write(ctop_fd[i][1],&flag,sizeof(int));
                    //printf("Value of flag in child updated to %d\n",flag);
                    exit(0);
                }
            }
            //3. After the worker process begins execution, it should report the row it is processing
            printf("Worker process working on row %d\n",i);
            // worker threads created and calculate px and thapx
            pthread_t workers[n];
            struct params pi[n];
            int *px[n];
            int size_px[n];
            int thapx[n];
            for(int j =0; j<n;j++){
                pi[j].p = p;pi[j].j=j; 
                pi[j].x =values[j];pi[j].pri = primes; pi[j].sizeofpri=sizeOfPrimes;
                //4. After creation of worker thread
                printf("Worker thread no %d created!!!\n",j);
                pthread_create(&workers[j],NULL,runner,(void*)(&pi[j]));
            }
            for(int j=0;j<n;j++){
                //9. After thread termination/thread join
                printf("%dth thread joined\n",j);
                pthread_join(workers[j],NULL);
                px[j] = pi[j].px; size_px[j]=pi[j].size_px;thapx[j] =pi[j].thapx;
            }
            //10. After all thapx is calculated
            printf("All threads have calculated thapx successfully!!\n\n");

            // printf("%d size px\n",size_px[0]);
            for(int j = 0; j<n; j++){
                printf("%dth px set: ",j);
                for(int k = 0;k<size_px[j];k++){
                    printf("%d ",px[j][k]);
                }
                printf("\nthapx: %d\n",thapx[j]);
            }
            int wpapx=0;
            for(int j=0;j<n;j++)
                wpapx+=thapx[j];
            wpapx/=n;
            //11. After wpapx is calculated
            printf("wpapx for row %d is calculated in the worker process: %d\n",i,wpapx);
            write(ctop_fd[i][1],&wpapx,sizeof(int));
            //12. After wpapx is written to the controller
            printf("wpapx for row %d written successfully to the controller!!!\n\n",i);
            close(ctop_fd[i][1]); // close write and after sending to parent;
            for(int j=0;j<n;j++){
                free(px[j]);
            }
            exit(0);
        }
    }
    for(int i =0; i<n;i++){
        printf("wpapx for %dth row: %d\n",i,wpapx[i]);
    }
    int sums=0;
    for(int i=0;i<n; i++){
        sums+=wpapx[i];
    }
    fapx=sums/n;
    //14. After the controller calculates the fapx
    printf("\n\nRESULT fapx value: %d calculated by controller id: %d\n\n",fapx,getpid());
    return 0;
}
