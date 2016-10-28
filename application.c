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
#include <unistd.h>

#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/sched.h>


void clrBuffer(void) {
    int c;
    while ((c=getchar()) != '\n' && c != EOF)
        ;
}

void clrTerminal(void) {
     printf("\033[H\033[J"); //Clear terminal screen
}

void readFunction(char blocking,char * bufRead) {

    int i=0;
    int status = 0;
    //char userChoice = '0';
    int devFd =  0; //File descriptor for the char device
    unsigned int nbCharToRead = 0;

    if(blocking){ //Blocking
        devFd = open("/dev/charDriverDev", O_RDWR);
        printf("|Read Buffer : Blocking IO|_ Device opening\n");
    }
    else{ //Non Blocking
        devFd = open("/dev/charDriverDev", O_RDWR|O_NONBLOCK);
        printf("|Read Buffer : NonBlocking IO|_ Device opening\n");
    }

    if (devFd>=0){
        //read in blocking mode
        printf("|Read Buffer|_ Device open : Read Only\n");

        printf("|Read Buffer|_ Type the number of characters to read (1 to 256):\n");
        scanf("%i", &nbCharToRead);
        clrBuffer();

        //Nombre incorrecte de caractère à lire
        if ((nbCharToRead>256)||(nbCharToRead<=0)){
            printf("|Read Buffer|_ Incorrect Number of character to read : %d \n",nbCharToRead);
        }
        //Lecture
        else{
            bufRead = malloc(nbCharToRead);
            memset(bufRead,0,nbCharToRead); //initialize a memory buffer of size nbCharToRead

            printf("|Read Buffer|_ Reading %d character(s) \n",nbCharToRead);
            status = read(devFd, &bufRead, nbCharToRead);
            if ((status == EAGAIN)&&(blocking==1)){
                printf("|Read Buffer|_ EAGAIN, Error : Read is blocking but it has not blocked\n");
            }
            //else if (status == ERESTARTSYS){
            //    printf("|Read Buffer|_ ERESTARTSYS, Blocking read : Error in wait_event_interruptible when read fall asleep\n");
            //}
            else if (status < 0){
                printf("|Read Buffer|_ Error Read return an unknown negative value\n");
            }
            else if (status >= 0){
                printf("|Read Buffer|_ Number of read character(s) = %d", status);
                printf("|Read Buffer|_ read character(s) : ");
                for(i=0;i<nbCharToRead;i++){ //show read characters
                    printf("%c",*bufRead+i);
                }
            }
            free(bufRead); //free memory buffer
        }

    }
    else{ //error during open file
        printf("|Read Buffer : Blocking IO|_ Fail to open the device file\n");
    }
}

void writeFunction(char blocking,char * bufWrite) {

    int i=0;
    int status = 0;
    char usrChar = 0;
    int  nbCharToWrite = 0;
    char usrCharBuf[256];
    //char userChoice = '0';
    int devFd =  0; //File descriptor for the char device

    if(blocking){ //Blocking
        devFd = open("/dev/charDriverDev", O_WRONLY);
        printf("|Write Buffer : Blocking IO|_ Device opening\n");
    }
    else{   //NonBlocking
        devFd = open("/dev/charDriverDev", O_WRONLY | O_NONBLOCK);
        printf("|Write Buffer : NonBlocking IO|_ Device opening\n");
    }

    if (devFd>=0){
        //read in blocking mode
        printf("|Write Buffer|_ Device open\n");

        while(usrChar!='\n'){
            printf("|Write Buffer|_ Type a character to write in the buffer [Enter to Leave]:\n");
            scanf("%c", &usrChar);
            usrCharBuf[nbCharToWrite-1]=usrChar;
            clrBuffer();
            nbCharToWrite++;
        }

        //Nombre incorrecte de caractère à lire
        if ((nbCharToWrite>256)||(nbCharToWrite<=0)){
            printf("|Write Buffer|_ Incorrect Number of character to read : %d \n",nbCharToWrite);
        }
        //Ecriture
        else{
            bufWrite = malloc(nbCharToWrite);
            memset(bufWrite,0,nbCharToWrite); //initialize a memory buffer of size nbCharToRead

            printf("|Write Buffer|_ Writing %d character(s) \n",nbCharToWrite);
            status = write(devFd, &bufWrite, nbCharToWrite);
            if ((status == EAGAIN)&&(blocking==1)){
                printf("|Write Buffer|_ EAGAIN, Error : Read is blocking but it has not blocked\n");
            }

            else if (status < 0){
                printf("|Write Buffer|_ Error Read return an unknown negative value\n");
            }
            else if (status >= 0){
                printf("|Write Buffer|_ %d characters were written successfully\n",nbCharToWrite);
            }
            free(bufWrite); //free memory buffer
        }
    }
    else{ //error during open file
        printf("|Write Buffer : Blocking IO|_ Fail to open the device file\n");
    }

}


int main(void) {
        int i = 0;
		unsigned int nbCharToWrite = 0;
        char usrChar = 0;
        int devFd =  0; //File descriptor for the char device
        int status = 0;

		char userChoice = '0';
		char blocking = '0'; //0=nonBlocking 1=Blocking

        char bufWrite[20];
        char bufRead[20];

		unsigned int incorrectKeyCounter = 0; //While loop security

        while (userChoice!='5'){

            printf("----------------------------------\n");
            printf("------Char Driver Application-----\n");
            printf("----------------------------------\n \n");
            printf("MENU :\n ----- \n 0) DEBUG \n 1) Read \n 2) Write\n 3)Read & Write \n 4)Configure Buffer\n \n 5) Exit\n");
            scanf("%c", &userChoice);
            clrBuffer();

            switch (userChoice)
            {
                case '0':    //DEBUG
                    devFd = open("/dev/charDriverDev_node", O_WRONLY);
                    if (devFd<=0){
                        printf("|DEBUG WRITE|_ Error : Open Fail\n");
                    }
                    printf("|DEBUG WRITE|_  DevFd value : %d",devFd);
                   // printf("|DEBUG WRITE|_ Type a character to write in the buffer:\n");
                    printf("|DEBUG READ|_ Write in buffer ... \n");
                    //scanf("%s", &bufWrite);
                    status = write(devFd, "abcd", 4);
                    if ((status <= 0 )){
                        printf("|DEBUG WRITE|_ Error : Write Fail\n");
                    }
                    close (devFd);

                    devFd = open("/dev/charDriverDev_node", O_RDONLY);
                    if (devFd<=0){
                        printf("|DEBUG READ|_ Error : Open Fail\n");
                    }
                    printf("|DEBUG READ|_  DevFd value : %d",devFd);
                     printf("|DEBUG READ|_ Read buffer ... \n");
                    status = read(devFd, &bufRead, 4);

                    printf("|DEBUG READ|_ Read Return value : %c%c%c%c ... \n",bufRead[0],bufRead[1],bufRead[2],bufRead[3]);

                    printf("WAIT...",devFd);
                    getchar();
                    clrTerminal();
                    break;

                case '1':  //Read from Buffer
                    clrTerminal();
                    printf("------------Read Buffer-----------\n");
                    printf("----------------------------------\n \n");
                    printf("    1) Blocking Read \n    2) NonBlocking Read \n)");
                    scanf("%c", &userChoice);
                    clrBuffer();

                    switch (userChoice){
                        case '1' :
                                readFunction(1, bufRead); //Read in bloking mode

                            break;
                        case '2' :
                                readFunction(0, bufRead); //Read in Nonbloking mode
                            break;
                    }

                    break;

                case '2':  //Write to buffer
                    clrTerminal();
                    printf("------------Write Buffer-----------\n");
                    printf("----------------------------------\n \n");
                    printf("    1) Block write \n    2) Non Block Write \n) ");
                    scanf("%c", &userChoice);
                    clrBuffer();

                    switch (userChoice){
                        case '1' :
                                writeFunction(1, bufWrite);  //Write in bloking mode
                            break;
                        case '2' :
                                writeFunction(0, bufWrite);  //Write in Nonbloking mode
                            break;
                    }

                    break;

                case '3':  //Configure the module with IOCTL
                    break;

                case '4':  //Configure the module with IOCTL
                    break;


                case '5':  //Exit Application
                    printf("Return to the main menu \n \n \n");
                    break;

                default :
                    printf("Incorrect key");
                    incorrectKeyCounter ++;
                    break;

            }
            if (incorrectKeyCounter >= 5){
                userChoice = 5; //choice : exit application
            }
            else{
                userChoice = 0; //Reset user choice to continue
            }
            //clrTerminal();

        }

        printf("Fermeture de l'application ... Goodbye");
        return EXIT_SUCCESS;
}

