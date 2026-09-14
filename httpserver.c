#include<stdio.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<netdb.h>
#include<pthread.h>
#include<time.h>
#include<sys/types.h>
#include<sys/syscall.h>
#define _GNU_SOURCE
#define RESPONSE_SIZE 4096
#define BUFF_SIZE 4096
#define FILE_PATH_SIZE 300


const char * status_phrases_arr[] = {"File Not Found" ,"File Found" ,"Bad Request"};
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
//Different MIME types
char * get_mime_type(char * arg){

	if (arg == NULL) return "text/plain";
	if(strcmp(".html",arg) == 0) return "text/html";
	if(strcmp(".css",arg) == 0) return "text/css";
	if(strcmp(".js",arg) == 0 ) return "application/javascript";
	if(strcmp(".png",arg) == 0) return "images/png";
	if(strcmp(".jpg",arg) == 0)  return "images/jpg";
	
	return "application/octet-stream";
}
// Loggin of Users trying to access the server-->
void sendLogData(char *method , char * path, int status_code,pid_t tid,const char * status_phrase){	

	FILE * log_file;
	time_t raw_time;
	time(&raw_time);
	char * str_time = ctime(&raw_time);
	str_time[strlen(str_time) -1] = '\0';

	pthread_mutex_lock(&file_mutex);
	log_file  = fopen("server.log","a+");

	if(log_file== NULL){
		return;
	}


	fprintf(log_file,"Thread ID : %u [%s] %s %s Status Code: %d %s\n",tid,str_time,method,path,status_code,status_phrase);
	
	fclose(log_file);
	pthread_mutex_unlock(&file_mutex);
	

}


void * handle_client(void *arg){
	pid_t tid = syscall(SYS_gettid);
	int status_code;
	int client_socket_fd = *(int *) arg;
	free(arg);
	char request[RESPONSE_SIZE];


	int bytes = recv(client_socket_fd,request,RESPONSE_SIZE-1,0);
	request[bytes] = '\0';

	//--> parsing the request 


	char method[8] , path[255] , version[10];

	sscanf(request,"%7s %254s %9s",method,path,version);


	if (strcmp(method, "GET") != 0) {
        	status_code = 405;
    		const char *response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
        	send(client_socket_fd, response, strlen(response), 0);
		sendLogData(method,path,status_code,tid,status_phrases_arr[2]);
        	close(client_socket_fd);
        	return NULL;
   	 }
	char file_path[FILE_PATH_SIZE];
	if(strcmp(path,"/") == 0){
		strcpy(path,"/index.html");
	}

	sprintf(file_path,".%s",path);

	FILE * file = fopen(file_path,"rb");
	
	if(file == NULL){
		status_code = 404;
		const char *not_found =
           	 "HTTP/1.1 404 Not Found\r\n"
           	 "Content-Type: text/html\r\n"
           	 "Content-Length: 48\r\n"
           	 "\r\n"
        	 "<html><body><h1>404 Not Found</h1></body></html>";
       	 send(client_socket_fd, not_found, strlen(not_found), 0);
	 sendLogData(method,path,status_code,tid,status_phrases_arr[0]);
       	 close(client_socket_fd);
         return NULL;

	}

	// else --> status_code = 200
	status_code = 200;

	fseek(file,0,SEEK_END);
	long file_size = ftell(file);
	rewind(file);
	
	char * request_type = strstr(path,".");

	char * mime_type = get_mime_type(request_type);
	char header[512];
	sprintf(header,"HTTP/1.1 200 OK\r\n"
   		 "Content-Type: %s\r\n"
   		 "Content-Length: %ld\r\n"
    		"\r\n",mime_type,file_size);
	
	send(client_socket_fd,header,strlen(header),0);
	sendLogData(method,path,status_code,tid,status_phrases_arr[1]);
	printf("%s",header);
	char buff[BUFF_SIZE];
	size_t bytesRead;
	while((bytesRead = fread(buff,sizeof(char),BUFF_SIZE,file)) > 0){
		send(client_socket_fd,buff,bytesRead,0);
	}

	fclose(file);
	close(client_socket_fd);

	return NULL;


}



int main(){

	int client_socket , server_socket;

	struct addrinfo hints, *p , *res;


	memset(&hints,0,sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL,"8080",&hints,&res);
	for(p = res; p!=NULL; p = p->ai_next){
		server_socket = socket(p->ai_family,p->ai_socktype,p->ai_protocol);

		if(server_socket == -1) continue;

		if(bind(server_socket,p->ai_addr,p->ai_addrlen) != -1) break;

		close(server_socket);

	}

	listen(server_socket , 5);
	while(1){
		client_socket = accept(server_socket,NULL,NULL);
		pthread_t p;
		int * client_fd = malloc(sizeof(int));
		*client_fd = client_socket;
		pthread_create(&p,NULL,handle_client,client_fd);
		pthread_detach(p);

	}
	close(server_socket);
	freeaddrinfo(res);

}
