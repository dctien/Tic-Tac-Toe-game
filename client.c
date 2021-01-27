#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <assert.h>
#include <signal.h>
#include <errno.h>
#include "linklist.h"

#define BUFF_SIZE 1024

#define STATUS_START_MENU 0
#define STATUS_GAME 1
#define STATUS_HANDLE_GAME 2
#define SIGNAL_CHECKLOGIN "SIGNAL_CHECKLOGIN"
#define SIGNAL_CREATEUSER "SIGNAL_CREATEUSER"
#define SIGNAL_OK "SIGNAL_OK"
#define SIGNAL_ERROR "SIGNAL_ERROR" 
#define SIGNAL_CLOSE "SIGNAL_CLOSE"

#define SIGNAL_TICTACTOE "SIGNAL_TICTACTOE"
#define SIGNAL_TTT_RESULT "SIGNAL_TTT_RESULT"
#define SIGNAL_TICTACTOE_AI "SIGNAL_TICTACTOE_AI"


// client connect to server
struct sockaddr_in server_addr;
int PORT, sock, recieved, isCommunicating;
char* serverAddress;
char send_msg[BUFF_SIZE], recv_msg[BUFF_SIZE];

// client variable
int status; // status of game
char choice, token[] = "#";
char error[100], user[100], id[30];

// draw table
char *table;
int size, playerTurn, col, row;
int viewC = 0, viewR = 0;

// result of TicTacToe
int resultTTT;


int player;
unsigned turn;
/*
Kết nối với server
*/
int connectToServer(){
  isCommunicating = 1; // tao ket noi
  int errorConnect;
  recieved = -1;
  //Step 1: Construct socket
  if( (sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    strcpy(error,"Error Socket!!!");
    return -1;
  }
  
  //Step 2: Specify server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr(serverAddress);
  
  //Step 3: Request to connect server
  if( connect(sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1){
    errorConnect = errno;
    sprintf(error,"Error! Can not connect to server! %s",strerror(errorConnect));   
    return -1;
  }
  //Step 4: Communicate with server
  send(sock, send_msg, strlen(send_msg), 0);
  recieved = recv(sock, recv_msg, BUFF_SIZE, 0);
  recv_msg[recieved] = '\0';
  strcpy(send_msg, SIGNAL_CLOSE); // gui tin hieu ngat ket noi
  send(sock, send_msg, strlen(send_msg), 0);
  close(sock);
  if(recieved == -1){
    // printf("\nError: Timeout!!!\n");
    return -1;
  }
  isCommunicating = 0; // ngat ket noi
  return 0;
}
/*
Tictactoe 
*/
char gridChar(int i, char c) {
  switch(i) {
  case -1:
    return 'X';
  case 0:
    return c;
  case 1:
    return 'O';
  }
}

void draw(int b[9]) {
  printf("\033[0;34m");
  printf(" %c | %c | %c\n",gridChar(b[0], '1'),gridChar(b[1], '2'),gridChar(b[2], '3'));
  printf("---+---+---\n");
  printf(" %c | %c | %c\n",gridChar(b[3], '4'),gridChar(b[4], '5'),gridChar(b[5], '6'));
  printf("---+---+---\n");
  printf(" %c | %c | %c\n",gridChar(b[6], '7'),gridChar(b[7], '8'),gridChar(b[8], '9'));
}

int win(const int board[9]) {
  unsigned wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  int i;
  for(i = 0; i < 8; ++i) {
    if(board[wins[i][0]] != 0 &&
      board[wins[i][0]] == board[wins[i][1]] &&
      board[wins[i][0]] == board[wins[i][2]])
        return board[wins[i][2]];
  }
  return 0;
}

int minimax(int board[9], int player) {
  int winner = win(board);
  if(winner != 0) return winner*player;

  int move = -1;
  int score = -2;
  int i;
  for(i = 0; i < 9; ++i) {//For all moves,
    if(board[i] == 0) {//If legal,
      board[i] = player;//Try the move
      int thisScore = -minimax(board, player*-1);
      if(thisScore > score) {
        score = thisScore;
        move = i;
      }//Pick the one that's worst for the opponent
      board[i] = 0;//Reset board after try
    }
  }
  if(move == -1) return 0;
  return score;
}

void computerMove(int board[9]) {
  int move = -1;
  int score = -2;
  int i;
  for(i = 0; i < 9; ++i) {
    if(board[i] == 0) {
      board[i] = 1;
      int tempScore = -minimax(board, -1);
      board[i] = 0;
      if(tempScore > score) {
        score = tempScore;
        move = i;
      }
    }
  }
  board[move] = 1;
}

void player_move(int board[9]) {
  int move = 0;
  do {
    printf("\033[0;37m");
    start:
    printf("\nInput move ([1..9]): ");
    scanf("%d", &move);
    move = move -1;
    if(board[move] != 0) {
      printf("Its Already Occupied !");
      goto start;
    }
    printf("\n");
  } while (move >= 9 || move < 0 && board[move] == 0);

  board[move] = -1;
}

void showGame(char *user, int player){
  write(1,"\E[H\E[2J", 7);
  printf("\033[0;37m");
  printf("----------TIC-TAC-TOE GAME----------\n");
  printf("\tUsername: \033[0;34m\%s\033[0;37m\ \n", user);
  printf("\tNEW TicTacToe GAME\n");
  printf("---------------------------\n");
  printf("Computer: O, You: X\n");
  if(player == 1) printf("You play (1)st\n\n");
  else if( player == 2) printf("You play (2)nd\n\n");
}

int handleTicTacToe( char *user) {
  int board[9] = {0,0,0,0,0,0,0,0,0};
  printf("Computer: O, You: X\n");
  printf("You want to Play (1)st or (2)nd: ");
  player=0;
  scanf("%d",&player);
  printf("\n");
  
  for(turn = 0; turn < 9 && win(board) == 0; ++turn) {
    if( turn != 0 ) showGame(user, player);

    if((turn+player) % 2 == 0)
      computerMove(board);
    else {
      draw(board);
      player_move(board);
    }

    if( turn == 0 ) write(1,"\E[H\E[2J", 7);
  }

  switch(win(board)) {
    case 0:
      printf("\033[0;37m");
      printf("A draw. How droll.\n");
      return 0;
    case 1:
      draw(board);
      printf("\033[0;37m");
      printf("You lose.\n");
      return 1;
    case -1:
      printf("\033[0;37m");
      printf("You win. Inconceivable!\n");
      return -1;
  }
}

/*
Hiển thị phần login
*/
int menuSignin(){
  error[0] = '\0';
  char pass[100], *str;
  while(1){
    // clearScreen();
    write(1,"\E[H\E[2J", 7);
    printf("-------SIGN IN GAME--------\n");
    printf("\tSign in\n");
    printf("---------------------------\n");
    if(error[0] != '\0'){
      printf("Error: %s!\n", error);
      printf("Do you want to try again?(y or n): ");
      choice = getchar();
      while(getchar() != '\n');
      if(choice=='n' || choice=='N'){
      	error[0] = '\0';
      	return -1;
      }
      else if(choice !='y' && choice !='Y'){
        continue;
      }
      else{
      	error[0] = '\0';
      	continue;
      }
    }
    printf("Username: ");
    fgets(user, BUFF_SIZE, stdin);
    user[strlen(user)-1] = '\0';

    printf("Password: ");
    getPassword(pass);

    //check username and password
    sprintf(send_msg, "%s#%s#%s", SIGNAL_CHECKLOGIN, user, pass);
    if(connectToServer() == 0){    
      str = strtok(recv_msg, token);
      if(strcmp(str, SIGNAL_OK) == 0){
        // break;
        return 0;
      }
      else if(strcmp(str, SIGNAL_ERROR) == 0){
      	str = strtok(NULL, token);
      	strcpy(error, str);
        return -2;
      }
    }
    else {
      printf("Error! Cant connect to server!\n");
      return -1;
    }
  }  
  // return 0;
}

/*
Hiển thị phần đăng kí
*/
int menuRegister(){
  error[0] = '\0';
  char pass[100], comfirmPass[100], *str;
  while(1){
    // clearScreen();
    write(1,"\E[H\E[2J", 7);
    printf("--------REGISTER GAME------\n");
    printf("\tRegister\n");
    printf("---------------------------\n");
    if(error[0] != '\0'){
      printf("Error: %s!\n", error);
      printf("Do you want to try again?(y or n): ");
      choice = getchar();
      while(getchar() != '\n');
      if(choice =='n' || choice =='N'){
      	error[0] = '\0';
      	return -2;
      }
      else if(choice !='y' && choice !='Y')
        continue;
      else{
      	error[0] = '\0';
      	continue;
      }
    }
    printf("Username: ");
    fgets(user, BUFF_SIZE, stdin);
    user[strlen(user)-1] = '\0';

    printf("Password: ");
    getPassword(pass);
    printf("Confirm password: ");
    getPassword(comfirmPass);
    if(strcmp(pass, comfirmPass) == 0){
      // register new account
      sprintf(send_msg, "%s#%s#%s", SIGNAL_CREATEUSER, user, pass);
      if(connectToServer() == 0){    
      	str = strtok(recv_msg, token);
      	if(strcmp(str, SIGNAL_OK) == 0)
      	  break;
      	else if(strcmp(str, SIGNAL_ERROR) == 0){
      	  str = strtok(NULL, token);
      	  strcpy(error, str);
      	}
      }
      else {
        printf("Error! Cant connect to server!\n");
        return -1;
      }
    }
    else{
      strcpy(error, "Password does not match");
    }
  }  
  return 0;
}

/*
Hiển thị chọn login, create
*/
int menuStart(){
  int checkSignin = 0, checkRegister = 0;
  error[0] = '\0';  
  while(1){
    // clearScreen();
    write(1,"\E[H\E[2J", 7);
    if(error[0] != '\0'){
      printf("Error: %s!\n", error );
      error[0] = '\0';
    }
    printf("-------LOGIN/REGISTER------\n");
    printf("\t1.Sign in\n");
    printf("\t2.Register\n");
    printf("\t3.Exit\n");
    printf("---------------------------\n");
    printf("Your choice: ");
    scanf("%c", &choice);
    while(getchar() != '\n');
    if(choice == '1'){
      checkSignin = menuSignin();
      if( checkSignin == 0){
        break;
      } else if( checkSignin == -1)
        return -1;
    }
    else if(choice == '2'){
      checkRegister = menuRegister();
      if(checkRegister == 0)
        break;
      else if( checkRegister == -1)
        return -1;
    }
    else if(choice == '3') return -1;
    else sprintf(error,"No option %c", choice);
  }

  return 0;
}

/*
TicTacToe AI, Có kết nối với server, để sử dụng copy vào file client.c
*/
int handleTicTacToeAI( char *user) {
  char *str;
  int board[9] = {0,0,0,0,0,0,0,0,0};
  //computer squares are 1, player squares are -1.
  printf("Computer: O, You: X\n");
  printf("You want to Play (1)st or (2)nd: ");
  player=0;
  scanf("%d",&player);
  printf("\n");
  
  for(turn = 0; turn < 9 && win(board) == 0; ++turn) {
    if( turn != 0 ) showGame(user, player);

    if((turn+player) % 2 == 0){
      computerMove(board);
      sprintf(send_msg, "%s#%s", SIGNAL_TICTACTOE_AI, user);
      if(connectToServer() == 0){    
        str = strtok(recv_msg, token);
        if(strcmp(str, SIGNAL_OK) == 0){
          str = strtok(NULL, token);
          strcpy(id, str);
        }
      }
      else {
        printf("Error! Cant connect to server!\n");
      }
    }
    else {
      draw(board);
      player_move(board);
    }
    if( turn == 0 ) write(1,"\E[H\E[2J", 7);
  }

  switch(win(board)) {
    case 0:
      printf("\033[0;37m");
      printf("A draw. How droll.\n");
      return 0;
    case 1:
      draw(board);
      printf("\033[0;37m");
      printf("You lose.\n");
      return 1;
    case -1:
      printf("\033[0;37m");
      printf("You win. Inconceivable!\n");
      return -1;
  }
}

/*
Hiển thị phần tic-tac-toe
*/
int handleTicTacToeGame(){
  char* str;
  error[0] = '\0';
  int i;
  while(1){
    // clearScreen();
    write(1,"\E[H\E[2J", 7);
    printf("------TIC-TAC-TOE GAME-----\n");
    printf("\033[0;34m\tUsername: %s \033[0;37m\n", user);
    printf("\tNEW TicTacToe GAME\n");
    printf("---------------------------\n");
    if(error[0] != '\0'){
      printf("Error: %s!\n", error);
      printf("Do you want to try again? (y or n): ");
      choice = getchar();
      while(getchar() != '\n');
      if(choice == 'n' || choice == 'N'){
        error[0] = '\0';
        return -1;
      }
      else if(choice != 'y' && choice != 'Y')
        continue;
      else{
        error[0] = '\0';
        continue;
      }
    }

    sprintf(send_msg, "%s#%s", SIGNAL_TICTACTOE, user);
    if(connectToServer() == 0){
      str = strtok(recv_msg, token);
      if(strcmp(str, SIGNAL_OK) == 0){
        str = strtok(NULL, token);
        strcpy(id, str);

        // resultTTT = handleTicTacToe(user); // 0 hòa, 1 thua, -1 thắng
        resultTTT = handleTicTacToeAI(user);
        sprintf(send_msg, "%s#%s#%d", SIGNAL_TTT_RESULT, id, resultTTT);
        if(connectToServer() == 0){    
          str = strtok(recv_msg, token);
          if(strcmp(str, SIGNAL_OK) == 0){
            str = strtok(NULL, token);
            strcpy(id, str);
            printf("Do you want to play again? (y or n): ");
            while(getchar() != '\n');
            choice = getchar();
            while(getchar() != '\n');
            
            if(choice == 'n' || choice == 'N'){
              return -1;
            }
            else if(choice != 'y' && choice != 'Y'){
              continue;
            }
            else{
              continue;
            }
          }
          else if(strcmp(str, SIGNAL_ERROR) == 0){
            str = strtok(NULL, token);
            strcpy(error, str);
          }
        }
      }
      else if(strcmp(str, SIGNAL_ERROR) == 0){ // error connect to server
        str = strtok(NULL, token);
        strcpy(error, str);
      }
    }
    else {
      printf("Error! Cant connect to server!\n");
    }
  }
  return 0;
}

/*
Hiển thị phần xử lí game, chọn game caro hay tic-tok-toe
*/
int menuGame(){
  error[0] = '\0';  
  while(1){
    // clearScreen();
    write(1,"\E[H\E[2J", 7);
    if(error[0] != '\0'){
      printf("Error: %s!\n", error);
      error[0] = '\0';
    }
    printf("------TIC-TAC-TOE GAME-----\n");
    printf("\033[0;34m\tWelcome %s \033[0;37m\n", user);
    // printf("\t1.Caro game\n");
    printf("\t1.TicTacToe game\n");
    // printf("\t3.Caro ranking\n");
    // printf("\t4.TicTacToe ranking\n");
    printf("\t2.Sign out\n");
    printf("---------------------------\n");
    printf("Your choice: ");
    scanf("%c", &choice);
    while(getchar() != '\n');
    // if(choice == '1'){
    //   checkCaroOrTicTacToe = 1;
    //   if(menuCaroGame() == 0) break;
    // }
    if(choice == '1'){
      // checkCaroOrTicTacToe = 2;
      if(handleTicTacToeGame() == 0) break;
    }
    // else if(choice == '3'){
    //   checkCaroOrTicTacToe = 3;
    //   break;
    // }
    // else if(choice == '2'){
    //   checkCaroOrTicTacToe = 4;
    //   break;
    // }
    else if(choice == '2'){
      return -1;
    }
    else {
      sprintf(error,"%c Not an optional", choice);      
    }
  }
  return 0;
}

int main(int argc, char* argv[]){     
  int err;
  if(argc != 3){
    printf("Syntax Error.\n");
    printf("Syntax: ./client IPAddress PortNumber\n");
    return 0;
  }
  if(check_IP(argv[1]) == 0){
    printf("IP address invalid\n");
    return 0;
  }
  if(check_port(argv[2]) == 0){
    printf("Port invalid\n");
    return 0;
  }
  serverAddress = argv[1];
  PORT = atoi(argv[2]);
  
  status = STATUS_START_MENU;  
  while(1){
    if(status == STATUS_START_MENU){
      if(menuStart() == -1)
        break;
      else
        status = STATUS_GAME;
    }
    else if(status == STATUS_GAME){
      if(menuGame() == - 1)
        status = STATUS_START_MENU;	
      else
        status = STATUS_HANDLE_GAME;
    }
    else if(status == STATUS_HANDLE_GAME){
      if( handleTicTacToeGame() == -1) {
        status = STATUS_GAME;
      }
    }
  }
  return 0;
}

/* 
Kiểm tra dấu chấm trong địa chỉ ip
Output: 1 - dấu chấm hợp lệ, 0 - dấu chấm lỗi
*/
int check_period(char *string){
  int count_period = 0, n = strlen(string);

  if(string[0] == '.') return 0;
  if(string[n-1] == '.') return 0;
  for (int i = 0; i < n-1; i++){
    if (string[i] == '.')
      count_period++;
    if (string[i] == '.' && string[i + 1] == '.') //Kiểm tra 2 dấu chấm có cạnh nhau không
      return 0;
  }
  if (count_period != 3) //Số lượng dấu chấm khác 3 sẽ fail
    return 0;
  return 1;
}


/* 
Kiểm tra xem string có là địa chỉ IP không
Output: 1 - là địa chỉ IP, 0 - không là địa chỉ IP
*/
int check_IP(char *string){
  int value = 0, n = strlen(string);
  if(check_period(string) == 0){
    return 0;
  }else{
    for(int i=0; i<n; i++){
      if( string[i] == '.'){
        if(value < 0 || value > 255)
          return 0;
        value = 0;
      }else{
        if(string[i] >= '0' && string[i] <= '9'){
          value = value*10 + (string[i] - '0');
          if(i == n-1)
            if(value < 0 || value > 255)
              return 0;
        }else
          return 0;
      }
    }
    return 1;
  }
}

/*
Kiểm tra số hiệu cổng
Output: 1 - cổng hợp lệ, 0 - cổng không hợp lệ
*/
int check_port(char *port){
  int n = strlen(port);
  for(int i=0; i< n; i++){
    if(port[i]<'0' || port[i]>'9')
      return 0;
  }
  return 1;
}

char error[100];

/*
Lấy password
*/
int getPassword(char pass[]){
  int i = 0;
  // setCustomTerminal();
  while((pass[i] = getchar()) != '\n')
    i++;
  pass[i] = '\0';
  // setDefaultTerminal();
  return i;  
}


