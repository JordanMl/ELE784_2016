/*
 ============================================================================
 Name        : application.c
 Author      :
 Version     :
 Copyright   :
 Description : Test application for char device module
 ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/sched.h>

#include "ioctlcmd.h"


void clrBuffer(void) {
    int c;
    while ((c=getchar()) != '\n' && c != EOF)
        ;
}

void clrTerminal(void) {
     printf("\033[H\033[J"); //Clear terminal screen
}

//Open and return the file descriptor
int openDriverRead(char blocking){

	 int devFd =  0; //File descriptor for the char device
	 if(blocking==1){ //Blocking
		  printf("|Read Buffer : Blocking IO|_ Device opening...\n");
        devFd = open("/dev/charDriverDev_node", O_RDONLY);
    }
    else{ //Non Blocking
        printf("|Read Buffer : NonBlocking IO|_ Device opening...\n");
		  devFd = open("/dev/charDriverDev_node", O_RDONLY|O_NONBLOCK);

    }

	 if(devFd > 0){
		 	printf("|Read Buffer : Device is open\n");
	 }
	 else{
			printf("|Read Buffer : Error : Device is not open\n");
            printf(" Press a key to continue \n");
            getchar();
	 }
	 return devFd;
}


//Open driver in Write Mode and return the file descriptor
int openDriverWrite(char blocking){

	 int devFd =  0; //File descriptor for the char device
	 if(blocking){ //Blocking
			printf("|Write Buffer : Blocking IO|_ Device opening...\n");
			devFd = open("/dev/charDriverDev_node", O_WRONLY);
    }
    else{   //NonBlocking
        printf("|Write Buffer : NonBlocking IO|_ Device opening...\n");
		  devFd = open("/dev/charDriverDev_node", O_WRONLY | O_NONBLOCK);
    }

	 if(devFd > 0){
		 	printf("|Write Buffer : Device is open\n");
	 }
	 else{
			printf("|Write Buffer : Error : Device is not open\n");
            printf(" Press a key to continue \n");
            getchar();
	 }
	 return devFd;
}

//Open driver in Read/Write Mode and return the file descriptor
int openDriverReadWrite(char blocking){

	 int devFd =  0; //File descriptor for the char device
	 if(blocking){ //Blocking
			printf("|Read/Write Buffer : Blocking IO|_ Device opening...\n");
			devFd = open("/dev/charDriverDev_node", O_RDWR);
    }
    else{   //NonBlocking
        printf("|Read/Write Buffer : NonBlocking IO|_ Device opening...\n");
		  devFd = open("/dev/charDriverDev_node", O_RDWR | O_NONBLOCK);
    }

	 if(devFd > 0){
		 	printf("|Read/Write Buffer : Device is open\n");
	 }
	 else{
			printf("|Read/Write Buffer : Error : Device is not open\n");
            printf(" Press a key to continue \n");
            getchar();
	 }
	 return devFd;
}


void readFunction(int devFd,char blocking) {

    int i=0;
    int status = 0;
    unsigned int nbCharToRead = 0;
	 char *bufRead;
	 char usrCharBuf[256];


     printf("|Read Buffer|_ Type the number of characters to read (1 to 256):\n");
     scanf("%d", &nbCharToRead);
     clrBuffer();

     //Nombre incorrecte de caractère à lire
     if ((nbCharToRead>256)||(nbCharToRead<=0)){
         printf("|Read Buffer|_ Incorrect Number of character to read (must be <256 & >0 \n");
     }
     //Lecture
     else{

         printf("|Read Buffer|_ Reading %d character(s) \n",nbCharToRead);
         status = read(devFd, &usrCharBuf, nbCharToRead);
			printf("|Read Buffer|_ read() function return status = %d  \n",status);
			// if ((status == -EAGAIN)&&(blocking==0)){
         if (status == EAGAIN){
             printf("|Read Buffer|_ EAGAIN, Error : Buffer is empty (NonBlocking read)\n");
         }
         //else if (status == ERESTARTSYS){
         //    printf("|Read Buffer|_ ERESTARTSYS, Blocking read : Error in wait_event_interruptible when read fall asleep\n");
         //}
         else if (status < 0){
             printf("|Read Buffer|_ Error Read return an unknown negative value\n");
         }
         else if (status >= 0){
             printf("|Read Buffer|_ Number of read character(s) = %d \n", status);
             printf("|Read Buffer|_ read character(s) : ");
             for(i=0;i<nbCharToRead;i++){ //show read characters
                 printf("%c",usrCharBuf[i]);
             }
				 printf("\n");
         }
     }

}

long ioctlFunction(int fd, int cmd,unsigned long arg){
    long retval = 0;
    switch(cmd){
        // fonction getnumdata
        case 1 :   retval = ioctl(fd,CHARDRIVER_IOC_GETNUMDATA,arg);
                   break;
        // fonction getnumreader
        case 2 :    retval = ioctl(fd,CHARDRIVER_IOC_GETNUMREADER,arg);
                    break;
        // fonction getbufsize
        case 3 :    retval = ioctl(fd,CHARDRIVER_IOC_GETBUFSIZE,arg);
                    break;
        //fonction getbufsize
        case 4 :    retval = ioctl(fd,CHARDRIVER_IOC_SETBUFSIZE,arg);
                    break;
        default : return -1;
    }
    return retval;
}



void writeFunction(int devFd,char blocking) {

    int i=0;
    int status = 0;
    char usrChar = 0;
    int  nbCharToWrite = 0;
    char usrCharBuf[256];


		printf("|Write Buffer|_ Type characters to write in the buffer [Enter to finish]:\n");
		scanf("%s", &usrCharBuf);
		//usrChar = getchar();
		printf("chaine tapée : %s\n",usrCharBuf);
		nbCharToWrite = strlen(usrCharBuf);
		printf("nombre de caractère : %d\n",nbCharToWrite);
		clrBuffer();

	  //Nombre incorrecte de caractère à écrire
	  if ((nbCharToWrite>256)||(nbCharToWrite<=0)){
		  //Aucun caractère à écrire
		  if (nbCharToWrite==0){
		      printf("|Write Buffer|_ 0 character to write: %d \n",nbCharToWrite);
		  }
		  else{
		  		printf("|Write Buffer|_ Incorrect Number of character to write (must be <256 & >0 \n");
		  }
	  }
	  //Ecriture
	  else{
	      //bufWrite = malloc(nbCharToWrite);
	      //memset(bufWrite,0,nbCharToWrite); //initialize a memory buffer of size nbCharToRead

	      printf("|Write Buffer|_ Writing %d character(s) \n",nbCharToWrite);
	      //status = write(devFd, &usrCharBuf, nbCharToWrite);
			status = write(devFd, &usrCharBuf, nbCharToWrite);
			printf("|Write Buffer|_ write() function return status = %d  \n",status);
	      if ((status == -EAGAIN)&&(blocking==0)){
	          printf("|Write Buffer|_ EAGAIN, Error : Buffer is full (NonBlocking Write)\n");
	      }

	      else if (status < 0){
	          printf("|Write Buffer|_ Error Read return an unknown negative value\n");
	      }
	      else if (status >= 0){
               printf("|Write Buffer|_ %d characters were written successfully\n",status);
	          //printf("|Write Buffer|_ %d characters were written successfully\n",nbCharToWrite);
	      }
	      //free(bufWrite); //free memory buffer
	  }

}


int main(void) {
		int i = 0;
		unsigned int nbCharToWrite = 0;
		char usrChar = 0;
		int devFd =  0; //File descriptor for the char device
		int status = 0;
        unsigned long bufferSize = 0;

		char userChoice = '0';
		char blocking = '0'; //0=nonBlocking 1=Blocking

		unsigned int incorrectKeyCounter = 0; //While loop security

        while (userChoice!='5'){

            printf("----------------------------------\n");
            printf("------Char Driver Application-----\n");
            printf("----------------------------------\n \n");
            printf("MENU :\n ----- \n 1) Read \n 2) Write\n 3) Read & Write \n 4) IOCTL Commands\n \n 5) Exit\n-> ");
            scanf("%c", &userChoice);
            clrBuffer();

            switch (userChoice)
            {

                case '1':  //Read from Buffer
                    clrTerminal();
                    printf("------------Read Buffer-----------\n");
                    printf("----------------------------------\n \n");
                    printf("    1) Blocking Read \n    2) NonBlocking Read \n-> ");
                    scanf("%c", &userChoice);
                    clrBuffer();

                    switch (userChoice){
                        case '1' :
                            blocking = 1;
                            devFd = openDriverRead(blocking); //Open driver in Blocking Read Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    readFunction(devFd,blocking); //Read in bloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }
                            break;

                        case '2' :
                            blocking = 0;
                            devFd = openDriverRead(0); //Open driver in NonBlocking Read Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    readFunction(devFd,blocking); //Read in Nonbloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }
                            break;
                    }
                    break;

                case '2':  //Write to buffer
                    clrTerminal();
                    printf("------------Write Buffer-----------\n");
                    printf("----------------------------------\n \n");
                    printf("    1) Block write \n    2) Non Block Write \n-> ");
                    scanf("%c", &userChoice);
                    clrBuffer();

                    switch (userChoice){
                        case '1' :
                            blocking = 1;
                            devFd = openDriverWrite(blocking); //Open driver in Blocking Write Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    writeFunction(devFd,blocking); //Write in Nonbloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }

                            break;
                        case '2' :
                            blocking = 0;
                            devFd = openDriverWrite(blocking); //Open driver in Blocking Write Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    writeFunction(devFd,blocking); //Write in Nonbloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }
                            break;
                    }
                    break;

                case '3':  // RW (Read/Write access)
                    clrTerminal();
                    printf("------------Read/Write Buffer-----------\n");
                    printf("----------------------------------\n \n");
                    printf("    1) Block Read\n    2) NonBlock Read \n    3) Block Write\n    4) NonBlock Write\n ");
                    scanf("%c", &userChoice);
                    clrBuffer();

                    switch (userChoice){
                        case '1' :
                            blocking = 1;
                            devFd = openDriverReadWrite(blocking); //Open driver in Blocking Read Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    readFunction(devFd,blocking); //Read in bloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }
                            break;

                        case '2' :
                            blocking = 0;
                            devFd = openDriverReadWrite(0); //Open driver in NonBlocking Read Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    readFunction(devFd,blocking); //Read in Nonbloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }
                            break;

                        case '3' :
                            blocking = 1;
                            devFd = openDriverReadWrite(blocking); //Open driver in Blocking Write Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    writeFunction(devFd,blocking); //Write in Nonbloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }

                            break;
                        case '4' :
                            blocking = 0;
                            devFd = openDriverReadWrite(blocking); //Open driver in Blocking Write Mode

                            if(devFd>0){
                                while(userChoice!='q'){
                                    writeFunction(devFd,blocking); //Write in Nonbloking mode
                                    printf("q to leave \nOther key to stay\n->");
                                    scanf("%c", &userChoice);
                                clrBuffer();
                                }
                                close(devFd);
                            }
                            break;
                        }
                    break;

                case '4':  //Commandes IOCTL


                    clrTerminal();
                    printf("------------IOCTL Commands--------\n");
                    printf("----------------------------------\n \n");

                    blocking = 0; //NonBlocking
                    devFd = openDriverWrite(blocking); //Open driver in Blocking Write Mode

                    if(devFd>0){
                        while(userChoice!='q'){
                            printf("    1) Get Number of Data\n    2) Get Number of Reader\n    3) Get Size of Circular Buffer\n    4) Set Size of the Buffer\n \n  q)Back to menu\n-> ");
                            scanf("%c", &userChoice);
                            clrBuffer();
                            switch (userChoice){
                            case '1' :
                                 printf("-------Get Num of Data-------\n");
                                 printf("Number of Data in the buffer : %ld \n",ioctlFunction(devFd, 1,0));
                                break;

                            case '2' :
                                printf("-------Get Num of Reader-------\n");
                                printf("Number of Reader : %ld \n",ioctlFunction(devFd, 2,0));
                                break;

                            case '3' :
                                printf("-------Get Buffer Size--------\n");
                                printf("Size of the buffer : %ld \n",ioctlFunction(devFd, 3,0));
                                break;

                            case '4' :
                                printf("-------Set Buffer Size--------\n");
                                printf("Enter the new buffer size\n-> ");
                                scanf("%lu", &bufferSize);
                                clrBuffer();
                                if((bufferSize<0) || (bufferSize>512)){
                                    printf("Error : Enter a value between 0 and 512 \n");
                                }
                                else{
                                    if((ioctlFunction(devFd, 4,bufferSize))<0){
                                        printf("Error : ioctl function return an error value\n");
                                    }
                                    else{
                                        printf("Buffer size change to : %lu\n",bufferSize);
                                    }
                                }
                                break;
                            }
                            printf("\n\n\n");
                        }
                        close(devFd);
                    }

                    break;


                case '5':  //Exit Application
                    printf("Return to the main menu \n \n \n");
                    break;

                default :
                    printf("Incorrect key\n");
                    incorrectKeyCounter ++;
                    break;

            }
            if ((userChoice=='5') || (incorrectKeyCounter >= 5)){
                userChoice = '5'; //choice : exit application
            }
            else{
                userChoice = '0'; //Reset user choice to continue
            }
            clrTerminal();

        }

        printf("Fermeture de l'application ... Goodbye \n\n");
        return EXIT_SUCCESS;
}

