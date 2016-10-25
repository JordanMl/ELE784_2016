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

void clrBuffer(void) {
    int c;
    while ((c=getchar()) != '\n' && c != EOF)
        ;
}

int main(void) {

		char userChoice = '0';

        while (userChoice != '3'){
            printf("---Pilote Char | Application de test---\n\n");
            printf("Menu Principal : \n 1) Ouvrir et utiliser le pilote char\n 2) Relacher le pilote char\n 3) Quitter \n");
            scanf("%c", &userChoice);

            printf("\033[H\033[J"); //Clear screen
            //Open the char device
            if (userChoice=='1'){

                //Ouverture du module :
                fd = open("/dev/pts/0", O_RDWR);
                //fd = open("/dev/pts/0", O_RDWR);

                userChoice='0';
                while (userChoice!='4'){
                    printf("----------------------------------\n");
                    printf("--Ouvrir et utiliser le pilote:---\n");
                    printf("----------------------------------\n");
                    printf("Operations possibles :\n 1) Read Buf\n 2) Write Buf\n 3)Configure Buf\n \n 4) pour retourner au menu\n");
                    scanf("%c", &userChoice);
                    clrBuffer();

                    switch (userChoice)
                    {
                        case '1':  //Read from Buffer

                            //ret = read(fd, &BufIn[n], 9-n);
                            break;

                        case '2':  //Write to buffer

                            //ret = write(fd, &BufOut[n], 1);
                            break;

                        case '3':  //Configure the module with IOCTL
                            break;

                        case '4':  //return to the main menu
                            printf("Return to the main menu \n \n \n");
                            break;
                    }
                }

            }

            //Close the char device
            else if(userChoice=='2'){
                printf("L'application relache le pilote \n");

                //close(fd);
                return EXIT_SUCCESS;
            }
            //App close
            else if(userChoice=='3'){
                printf("Fermeture de l'application \n");
            }
            else{
                printf("Touche incorrecte \n");
            }
        }
        printf("Fermeture biim");
        return EXIT_SUCCESS;
}

