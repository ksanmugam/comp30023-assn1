/*
   Name       : Kirentheren Ravindra Sanmugam
   Login ID   : ksanmugam
   Student ID : 823188
 */
 
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<string.h>
#include<strings.h>
#include<unistd.h>
#include<sys/types.h>
#include<pthread.h>
#include<syslog.h>
#include<ctype.h>
#include<sys/socket.h>
#include<sys/stat.h>

/* pDebug Macro defination
 * When Debugging, just print out the error message on the screen.
 * When not debugging, log the error message to a logfile for future retrival and analysis */
#ifdef DEBUG
/* When DEBUG mode is on, print to standard error */
    #define pdebug(format, args...) fprintf(stderr, format, ##args)
#else
/* When the DEBUG mode is off, print to log */
    #define pdebug(format, args...) syslog(LOG_ERR, format, ##args)
#endif

#define ISspace(x) isspace((int)(x))
#define BUFSIZE 2048 //SIZE OF BUFFER

/* Defination of various Error message and  their respective codes */
#define LISTEN_ERR_MSG "Listen Error"
#define LISTEN_ERR_CODE 43
#define ARG_ERR_MSG "Parameter input error!\n"
#define ARG_ERR_CODE 10
#define PCREATE_ERR_MSG "pthread_create error"
#define PCREATE_ERR_CODE 60
#define SOCKET_ERR_MEG "Socket err!"
#define SOCKET_ERR_CODE 40
#define GETSOCKNAME_ERR_MSG "Get Sock Name error"
#define GETSOCKNAME_ERR_CODE 42
#define ACCEPT_ERR_MSG "Accept Error"
#define ACCEPT_ERR_CODE 44
#define BIND_ERR_MSG "Bind err"
#define BIND_ERR_CODE 41

char * WebsitePath;

/* Functions Declaration */

int startup(unsigned short*);
void error_die(const char  *,int );
static void * accept_request(void * );
int get_line(int sock, char * buf,int size);
void unimplemented(int client);
void not_found(int client);
void serve_file(int client,const char *path);
void headers(int client,const char * filename);
void cat(int client,FILE* resource);
int is_file_ext(const char * filename,const char * ext);

/* Implement the functions functions */
void error_die(const char * msg,int code)
{   //print message and exit with code
    perror(msg);
    exit(code);
}

/* Start socket server */
int startup(unsigned short * port)
{
    int server=0;
    struct sockaddr_in name;

    server=socket(PF_INET,SOCK_STREAM,0);
    if (server==-1)
        error_die(SOCKET_ERR_MEG,SOCKET_ERR_CODE);
    memset(&name,0,sizeof(name));
    name.sin_family=AF_INET;
    name.sin_port=htons(*port);
    name.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(server,(struct sockaddr *)&name,sizeof(name))<0)
        error_die(BIND_ERR_MSG,BIND_ERR_CODE);
    if (*port==0)
    {
        int namelen=sizeof(name);
        if(getsockname(server,(struct sockaddr *)&name,&namelen))
           error_die(GETSOCKNAME_ERR_MSG,GETSOCKNAME_ERR_CODE);
        *port=ntohs(name.sin_port);
    }
    if(listen(server,5)<0)
           error_die(LISTEN_ERR_MSG,LISTEN_ERR_CODE);
    return (server);

}

/* Accept request and Respond to client */
static void * accept_request(void * arg)
{
    int client=*(int*)arg;
    printf("Accept Client  ID: %d\n",client);

    char buf[BUFSIZE];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i,j;
    struct stat st;
    int cgi = 0;
    char *query_string=NULL;

    numchars=get_line(client,buf,sizeof(buf));
    pdebug("%s",buf);
    i=0;
    j=0;
    while(!ISspace(buf[j])&&(i<sizeof(method)-1))
    {
        method[i]=buf[j];
        i++;
        j++;
    }
    method[i]='\0';
    //pdebug("%s",method); 
    if(strcasecmp(method,"GET") && strcasecmp(method,"POST"))
    {
        pdebug("Error Method: %s %s \n",method,buf);
        unimplemented(client);
    }
    i=0;
    while(ISspace(buf[j]) && (j<sizeof(buf)))
        j++;
    while(!ISspace(buf[j])&&(i<sizeof(url)-1)&&(j<sizeof(buf)-1))
    {
        url[i]=buf[j];
        i++;
        j++;
    }
    url[i]='\0';
    if(strcasecmp(method,"GET")==0 || strcasecmp(method,"POST")==0)
    {
        query_string=url;
        while((*query_string!='?')&&(*query_string!='\0'))
            query_string++;
        if(*query_string=='?')
        {
            // 404 not found
            // static website support only 
        }

        sprintf(path,"%s%s",WebsitePath,url);
        if(path[strlen(path)-1]=='/')
            strcat(path,"index.html");
        if(stat(path,&st)==-1)
        {
            while((numchars>0)&&strcmp("\n",buf))
                numchars=get_line(client,buf,sizeof(buf));
            not_found(client);
        }
        else 
        {
            if((st.st_mode & S_IFMT)==S_IFDIR)
                strcat(path,"/index.html");
            serve_file(client,path);
        }
        close(client);
    }
}

/* Get one line */
int get_line(int sock, char * buf,int size)
{
    int i=0;
    char c= '\0';
    int n;

    while((i<size-1)&&(c!='\n'))
    {
        n=recv(sock,&c,1,0);
        pdebug("%02X",c);
        if(n>0)
        {
            if(c=='\r')
            {
                n=recv(sock,&c,1,MSG_PEEK);
                pdebug("%02X",c);
                if((n>0)&&(c=='\n'))
                    recv(sock,&c,1,0);
                else
                    c= '\n';
            }
            buf[i]=c;
            i++;
        }
        else
            c='\n';
    }
    buf[i]= '\0';
    pdebug("\n%s\n",buf);
    return (i);

}

 /* Send file response to web client */
void serve_file(int client,const char *filename)
{
    FILE *resource=NULL;
    int numchars=1;
    char buf[1024];

    pdebug("serve_file: %s\n",filename);
    buf[0]='A';
    buf[1]='\0';
    while((numchars>0)&&strcmp("\n",buf))
    {
        numchars=get_line(client,buf,sizeof(buf));
    }
    resource=fopen(filename,"rb+");

    if(resource==NULL)
        not_found(client);
    else{
        headers(client,filename);
        cat(client,resource);
    }
    fclose(resource);
}

/* Test file extention to see if it match given extension */
int is_file_ext(const char * filename,const char * ext){
    return 0==strcmp(filename+strlen(filename)-strlen(ext),ext)?1:0;
}

/* Generate and send header to web client */
void headers(int client,const char * filename)
{
    char buf[1024];
    char type[256];
    if(is_file_ext(filename,".html") || is_file_ext(filename,".htm")){
        strcpy(type,"text/html");
    }else if(is_file_ext(filename,".css")){
        strcpy(type,"text/css");
    }else if(is_file_ext(filename,".js")){
        strcpy(type,"application/javascript");
    }else if(is_file_ext(filename,".jpg")){
        strcpy(type,"image/jpeg");
    }else{
        pdebug("File type unknown! %s \n",filename);
    }
    sprintf(buf,
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Server: server-test\r\n"
            "Connection: close\r\n"
            "\r\n",
            type
           );
    pdebug("%s\n",buf);
    send(client,buf,strlen(buf),0);
}

 /* append file content to web client  */
void cat(int client,FILE* resource)
{
    char buf[BUFSIZE];
    int l;
    do{
        l=fread(buf,sizeof(char),BUFSIZE,resource);
        send(client,buf,l,0);
    }while(l==BUFSIZE);
}
 
/* send 404 error to web client */
void not_found(int client)
{
    char buf[BUFSIZE];
    sprintf(buf,
            "HTTP/1.0 404 NOT FOUND\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<HTML>"
            "<HEAD><TITLE>404 NOT FOUND</TITLE></HEAD>"
            "<BODY><H1>404 NOT FOUND</H1></BODY>"
            "</HTML>"
           );
    send(client,buf,strlen(buf),0);
}

/* Send 501 error to web client */
void unimplemented(int client){
    char buf[1024];
    sprintf(buf,
            "HTTP/1.0 501 Method Not Implemented\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<HTML>"
            "<HEAD>"
            "<TITLE>Method Not Implemented</TITLE>"
            "</HEAD>"
            "<BODY>"
            "<p>HTTP request method not supported.</p>"
            "</BODY>"
            "</HTML>"
           );
    send(client, buf, strlen(buf),0);   
}

int main(int argc,char*argv[]){
    openlog("myServer",LOG_NDELAY,LOG_USER); //logs to myServer
    pdebug("Debug mode start\n"); //Activate Debugging
    int i;
    
    //arg count must be 2 because ./server [port] [path] 
    if (!(argc==3)) 
    {
        error_die(ARG_ERR_MSG,ARG_ERR_CODE);
    }
    // Transform string to unsigned short
    unsigned short port=(unsigned short)strtoul(argv[1],NULL,10);
    WebsitePath=argv[2];
    
    struct stat st;
    if(stat(WebsitePath,&st)==-1){
        error_die("ERROR: Website path not found\n",11);
    }
    if((st.st_mode & S_IFMT)!= S_IFDIR)
        error_die("ERROR: Website path is not a directory\n",12);
    if(WebsitePath[strlen(WebsitePath)-1]=='/')
        WebsitePath[strlen(WebsitePath)-1]='\0';
    int server_sock=startup(&port);
    printf("Server On 115.146.94.42:%d Started. \n ",port);
    printf("\n");
    
    int client_sock=-1;
    struct sockaddr_in client_name;
    int client_name_len=sizeof(client_name);
    pthread_t newthread;

    //this is an infinite loop that executes until when interrupted
    while(1)
    {
        if(-1==(client_sock=accept(server_sock,
                           (struct sockaddr *)&client_name,
                           &client_name_len)))
           error_die(ACCEPT_ERR_MSG,ACCEPT_ERR_CODE);
        if(-1==pthread_create(&newthread,NULL,accept_request,(void*)&client_sock))
            perror(PCREATE_ERR_MSG);
        usleep(10);
    }
    close(server_sock);
    return 0;
}
