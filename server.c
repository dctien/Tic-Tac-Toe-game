#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "linklist.h"

#define BUFF_SIZE 1024

#define SIGNAL_CHECKLOGIN "SIGNAL_CHECKLOGIN"
#define SIGNAL_CREATEUSER "SIGNAL_CREATEUSER"
#define SIGNAL_OK "SIGNAL_OK"
#define SIGNAL_ERROR "SIGNAL_ERROR" 
#define SIGNAL_CLOSE "SIGNAL_CLOSE"

#define SIGNAL_TICTACTOE "SIGNAL_TICTACTOE"
#define SIGNAL_TTT_RESULT "SIGNAL_TTT_RESULT"
#define SIGNAL_TICTACTOE_AI "SIGNAL_TICTACTOE_AI"


// server connect to client
int PORT;
struct sockaddr_in server_addr,client_addr;  
fd_set master;
char send_msg[BUFF_SIZE] , recv_msg[BUFF_SIZE];

// server variable
char token[] ="#";
char *str;
int tttResult;

/*
Xử lí dữ liệu gửi từ client
*/
int handleDataFromClient(int fd){
  char *user, *pass, *id;
  int recieved, col, row;
  ClientInfo *info;

  recieved = recv( fd, recv_msg, BUFF_SIZE, 0);
  recv_msg[recieved] = '\0';
  // printf("%s\n", recv_msg);
  str = strtok( recv_msg, token);
  if( strcmp(str, SIGNAL_CLOSE) == 0){
    FD_CLR(fd, &master); // Clears the bit for the file descriptor fd in the file descriptor set fdset.
    // printf("Close connection from fd = %d\n", fd );    
  }
  else if(strcmp(str, SIGNAL_CREATEUSER) == 0){
    // Create new user
    user = strtok(NULL, token);
    pass = strtok(NULL, token);
    if(isValid(user, NULL)){
      sprintf(send_msg,"%s#%s",SIGNAL_ERROR, "Account existed");
    } else{
      registerUser(user, pass);
      sprintf(send_msg, SIGNAL_OK);
    }

    send( fd, send_msg, strlen(send_msg), 0);
  }
  else if( strcmp(str, SIGNAL_CHECKLOGIN) == 0){   
    // Login
    user = strtok(NULL, token);
    pass = strtok(NULL, token);
    if(isValid(user, pass)) strcpy(send_msg, SIGNAL_OK);
    else sprintf( send_msg,"%s#%s", SIGNAL_ERROR, "Username or Password is incorrect");
    // while(1); // test timeout
    send(fd, send_msg, strlen(send_msg), 0);
  }
  else if(strcmp(str, SIGNAL_TICTACTOE) == 0){
    // Handle tic-tac-toe
    str = strtok(NULL, token);
    id = str;
    printf("TicTacToe game with id = \033[0;33m\%s\033[0;37m\ connecting on server\n", id);
    sprintf(send_msg,"%s#%s", SIGNAL_OK, id);
    send( fd, send_msg, strlen(send_msg), 0);
  }
  else if(strcmp(str, SIGNAL_TICTACTOE_AI) == 0){
    // Handle tic-tac-toe
    str = strtok(NULL, token);
    id = str;
    printf("TicTacToe game with id = %s, computer is processing...\n", id);
    sprintf(send_msg,"%s#%s", SIGNAL_OK, id);
    send( fd, send_msg, strlen(send_msg), 0);
  }
  else if(strcmp(str, SIGNAL_TTT_RESULT) == 0){ // 0 hòa, 1 thua, -1 thắng
    // Handle tic-tac-toe result
    str = strtok(NULL, token);
    id = str;
    str = strtok(NULL, token);
    tttResult = atoi(str);
    char resultString[50];
    if( tttResult == 0) strcpy(resultString, "You Draws");
    else if ( tttResult == 1) strcpy(resultString, "You Lost");
    else if ( tttResult == -1) strcpy(resultString, "You Win");
    printf("TicTacToe game with id = %s, Result: %s\n", id, resultString);
    sprintf( send_msg,"%s#%s", SIGNAL_OK, id);
    send( fd, send_msg, strlen(send_msg), 0);
  }
}

int check_port(char *port){
  int n = strlen(port);
  for(int i=0; i< n; i++){
    if(port[i]<'0' || port[i]>'9')
      return 0;
  }
  return 1;
}

int main(int argc, char *argv[]){
  if(argc != 2){
    printf("Syntax Error.\n");
    printf("Syntax: ./server PortNumber\n");
    return 0;
  }
  if(check_port(argv[1]) == 0){
    printf("Port invalid\n");
    return 0;
  }
  PORT = atoi(argv[1]);

  int sock, connected, sin_size, true = 1;
  int fdmax, i, rc;
  fd_set read_fds;
  
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  initList();
  
  // Step 1: Construct a TCP socket to listen connection request
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket Error!!!\n");
    exit(-1);
  }
  if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
    perror("Setsockopt error!!!\n");
    exit(-2);
  }
  
  //Step 2: Bind address to socket
  server_addr.sin_family = AF_INET;         
  server_addr.sin_port = htons(PORT);     
  server_addr.sin_addr.s_addr = INADDR_ANY; 
  bzero(&(server_addr.sin_zero),8); 
  
  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("Unable to bind\n");
    exit(-3);
  }
  
  //Step 3: Listen request from client
  if (listen(sock, 5) == -1) {
    perror("Listen error\n");
    exit(-4);
  }  
  printf("Conectting on port %d\n", PORT);
  fflush(stdout);
  
  FD_SET(sock, &master);
  fdmax = sock;

  // Set timeout
  struct timeval timeout;
  timeout.tv_sec  = 1000;  // after 1000 seconds will timeout
  timeout.tv_usec = 0;
  //Step 4: Communicate with clients
  while(1){
    read_fds = master;
    rc = select(fdmax + 1, &read_fds, NULL, NULL, &timeout);
    if( rc == -1){
      perror("select() error!\n");
      exit(-6);
    }
    else if (rc == 0){
    //  printf("  select() timed out. End program.\n");
     exit(-5);
    }
    for(i = 0; i <= fdmax; i++){
      if(FD_ISSET(i, &read_fds)){
        if(i == sock){
          sin_size = sizeof(struct sockaddr_in);
          connected = accept(sock, (struct sockaddr*)&client_addr, &sin_size);
          if(connected == -1){
            perror("accept error!\n");
            exit(-7);
          }
          else{
            FD_SET(connected, &master);
            if(connected > fdmax)
              fdmax = connected;
            printf("Got a connection from %s \n", inet_ntoa(client_addr.sin_addr));
            handleDataFromClient(connected);
          }
        }
        else{
          handleDataFromClient(i);
        }
      }
    }
  }
  close(sock);
  return 0;
}

/*
Kiểm tra username, password
Output: 1 - user, pass hợp lệ; 0 - không hợp lệ
*/
int isValid(char* username, char* password){
  FILE* f = fopen("user.txt", "r+");
  if(f == NULL ){
    printf("Error open file!!!\n");
    return 0;
  }
  char line[100];
  char* temp;  
  if( password != NULL){
    while(fgets( line, 100, f) != NULL){
      temp = line;
      while(temp[0] != '#') temp++; // get user pass, gap # thi dung
      temp[0] = '\0';
      temp++;
      if(temp[strlen(temp) - 1] == '\n') temp[strlen(temp) - 1] = '\0';
      if(strcmp(line, username) == 0 && strcmp(temp, password) == 0){
      	fclose(f);
      	return 1;
      }
    }
    fclose(f);
    return 0;
  } else{
    while(fgets(line, 100, f) != NULL){
      temp = line;
      while(temp[0] != '#') temp++;
      temp[0] = '\0';      
      if(strcmp(line, username) == 0){
      	fclose(f);
      	return 1;
      }
    }
    fclose(f);
    return 0;
  }  
}

/*
Đăng kí user mới
*/
void registerUser(char* username, char* password){
  FILE* f = fopen("user.txt", "a");
  fprintf(f, "%s#%s\n", username, password);
  fclose(f);
}