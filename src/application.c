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

void clrTerminal(void) {
     printf("\033[H\033[J"); //Clear terminal screen
}

int main(void) {

		char modeChoice = '0';
		char actionChoice = '0';

        while (modeChoice != '3'){
            printf("---Pilote Char | Application de test---\n\n");
            printf("Menu Principal : \n 1) Ouvrir et utiliser le pilote char\n 2) Relacher le pilote char\n 3) Quitter \n");
            scanf("%c", &modeChoice);

            clrTerminal();

            if (modeChoice=='1'){

                while (modeChoice!='5'){
                    printf("----------------------------------\n");
                    printf("--Ouvrir et utiliser le pilote:---\n");
                    printf("----------------------------------\n \n");
                    printf("Operations possibles :\n 1) Read \n 2) Write\n 3)Read & Write \n 4)Configure Buffer\n \n 5) pour retourner au menu\n");
                    scanf("%c", &modeChoice);
                    clrBuffer();

                    switch (modeChoice)
                    {
                        case '1':  //Read from Buffer
                            clrTerminal();
                            printf("------------Read Buffer-----------\n");
                            printf("----------------------------------\n \n");
                            printf("    1) Block Read \n    2) Non Block Read \n)");
                            scanf("%c", &actionChoice);
                            clrBuffer();

                            switch (actionChoice){
                                case '1' :
                                        //Ouverture device en bloquant      devFile = open("/dev/charDriverDev", O_RDWR);
                                        //Fonction/Bloucle de lecture bloquante
                                    break;
                                case '2' :
                                        //ouverture device en non bloquant
                                        //Fonction/Boucle de lecture non bloquante
                                    break;
                            }

                            /*
                            if (devFile < 0){
                                printf("Error : charDriverDev open");
                                userChoice = 4;
                            }
                            printf("Module charDriverDev open");
                            */

                            break;

                        case '2':  //Write to buffer
                            clrTerminal();
                            printf("------------Write Buffer-----------\n");
                            printf("----------------------------------\n \n");
                            printf("    1) Block write \n    2) Non Block Write \n) ");
                            scanf("%c", &actionChoice);
                            clrBuffer();

                            switch (actionChoice){
                                case '1' :
                                        //Ouverture device en bloquant      devFile = open("/dev/charDriverDev", O_RDWR);
                                        //Fonction/Bloucle d'ecriture bloquante
                                    break;
                                case '2' :
                                        //ouverture device en non bloquant
                                        //Fonction/Boucle d'ecriture non bloquante
                                    break;
                            }
                            //ret = write(fd, &BufOut[n], 1);
                            break;

                        case '3':  //Configure the module with IOCTL
                            break;

                        case '4':  //Configure the module with IOCTL
                            break;


                        case '5':  //return to the main menu
                            printf("Return to the main menu \n \n \n");
                            break;
                    }

                    modeChoice = 0; //clear user choice
                    clrTerminal();

                }

            }

            //Close the char device
            else if(modeChoice=='2'){
                printf("L'application relache le pilote \n");

                //close(fd);
                return EXIT_SUCCESS;
            }
            //App close
            else if(modeChoice=='3'){
                printf("Fermeture de l'application \n");
            }
            else{
                printf("Touche incorrecte \n");
            }
        }
        printf("Fermeture biim");
        return EXIT_SUCCESS;
}

