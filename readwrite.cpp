#include<iostream>
#include<fstream>
#include<pthread.h>
#include<string>
#include<stdlib.h>
#include<limits.h>
#include<time.h>
#include<unistd.h>
#include<semaphore.h>
#include<chrono>
#include<thread>

using namespace std;

int readcount=0, writecount=0,readerThread=0, N, R, W;
sem_t rmutex, wmutex, readTry, resource, almostDone;

struct node{
	int data;
	struct node *next;
};

struct list{
	node *head, *tail;
}myList;

void List_Init(list *l){
	l->head=NULL;
	l->tail=NULL;
}

void List_Insert(list *l, int d){
	node *newNode=(node*)malloc(sizeof(node));
	newNode->data=d;
	newNode->next=NULL;
	if(l->head==NULL){
		l->head=newNode;
		l->tail=newNode;
	}else{
		l->tail->next=newNode;
		l->tail=newNode;
	}
}

int List_Traverse(list *l, int n){
	int count=0;
	node *temp=l->head;
	while(temp!=NULL){
		if((temp->data % 10)==n) count++;
		temp=temp->next;
	}
	return count;
}

void *monitor(void *arg){
	sem_wait(&almostDone);
	cout<<"Almost Done!"<<endl;
	return NULL;
}

void *reader(void *arg){
//	cout<<"reader begin"<<endl;
	sem_wait(&rmutex);
	readerThread++;
	sem_post(&rmutex);
	long r=(long)arg;
	int * results=(int*) malloc(N*sizeof(int));

	for(int i=0;i<N;i++){
		sem_wait(&readTry);
		sem_wait(&rmutex);
		readcount++;
		if(readcount==1) sem_wait(&resource);
		sem_post(&rmutex);
		sem_post(&readTry);

		//read
		//cout<<"reading...."<<endl;
		results[i]=List_Traverse(&myList,r);

		//output
		ofstream myfile;
		string name="reader_"+to_string(r)+".txt";
		myfile.open(name);
		for(int i=0;i<N;i++){
			myfile<<"Reader "<<r<<": Read "<<i+1<<": "<<results[i]<<" values ending in "<<r<<endl;
		}
		sem_wait(&rmutex);
		readcount--;
		if(readcount==0) sem_post(&resource);
		sem_post(&rmutex);
		//cout<<"reader end"<<endl;
	}
	sem_wait(&rmutex);
	readerThread--;
	if(readerThread==1) sem_post(&almostDone);
	sem_post(&rmutex);
	return NULL;
}

void *writer(void *arg){
	//cout<<" write begin"<<endl;
	long w=(long)arg;
	int n=rand()%100+1;
	int a,b;
	for(int i=0;i<n;i++){
		sem_wait(&wmutex);
		writecount++;
		if(writecount==1) sem_wait(&readTry);
		sem_post(&wmutex);

	
		//a=rand()%INT_MAX;
		a=rand()%1000000;
		b=a%10;
		a=a-b+w;

		sem_wait(&resource);
		//cout<<"writing...."<<endl;
		List_Insert(&myList, a);	
		sem_post(&resource);
		
		sem_wait(&wmutex);
		writecount--;
		if(writecount==0) sem_post(&readTry);
		sem_post(&wmutex);
		//cout<<"sleeping...."<<endl;
		this_thread::sleep_for(chrono::microseconds(1));//sleep after each write
	}
	//cout<<"writer end"<<endl;
	return NULL;
}

int main(int argc, char * argv[]){
	
	if(argc!=4) cerr<<"The number of arguments is wrong. Usage:./readwrite N R W"<<endl;
	N=stoi(argv[1],  NULL, 10);
	R=stoi(argv[2], NULL, 10);
	W=stoi(argv[3], NULL, 10);

	if(R<1 || R>9){
		cerr<<"The number R must be between 1 and 9."<<endl;
		return 1;
	} 
	if(W<1 || W>9){
		cerr<<"The number W must be between 1 and 9."<<endl;
		return 1;
	} 
	if(N<1 || N>1000){
		cerr<<"The number N must be between 1 and 1000."<<endl;
		return 1;	
	} 
	
	List_Init(&myList);
	srand(time(NULL));
	sem_init(&rmutex, 0, 1);
	sem_init(&wmutex, 0, 1);
	sem_init(&readTry, 0, 1);
	sem_init(&resource, 0, 1);
	sem_init(&almostDone, 0, 0);

	pthread_t *readers=(pthread_t *)malloc(R*sizeof(pthread_t));
	if(readers==NULL){
		cerr<<"Fail to allocate readers."<<endl;
		return 1;
	}
	pthread_t *writers=(pthread_t *)malloc(W*sizeof(pthread_t));
	if(writers==NULL){
		cerr<<"Fail to allocate writers."<<endl;
		return 1;
	}

	pthread_t m;
	//create threads 
	int rc;
	for(int i=1;i<=W;i++){
		rc=pthread_create(&writers[i-1], NULL, writer, (void*)i);
		if(rc){
			cerr<<"Fail to create writer threads."<<endl;
			return 1;
		}
	}

	for(int i=1;i<=R;i++){
		rc=pthread_create(&readers[i-1], NULL, reader, (void *)i);
		if(rc){
			cerr<<"Fail to create reader threads."<<endl;
			return 1;
		}
	}

	rc=pthread_create(&m, NULL, monitor, NULL);
	if(rc){
			cerr<<"Fail to create monitor threads."<<endl;
			return 1;
	}
	//wait for the threads to completion
	for(int i=0;i<W;i++){
		rc=pthread_join(writers[i], NULL);
		if(rc){
			cerr<<"Fail to join for writer threads."<<endl;
			return 1;
		}
	}

	for(int i=0;i<R;i++){
		rc=pthread_join(readers[i], NULL);
		if(rc){
			cerr<<"Fail to join for reader threads."<<endl;
			return 1;
		}
	}

	rc=pthread_join(m, NULL);
	if(rc){
			cerr<<"Fail to join for the monitor thread."<<endl;
			return 1;
	}

	free(readers);
	free(writers);
	return 0;


}
