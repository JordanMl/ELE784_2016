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

int main(void) { 	\n 1) Read buf\n 2) Write buf\n 3)"

		char userChoice;
		
		printf("---Pilote Char | Application de test---\n\n");
		printf("Menu Principal : \n 1) Ouvrir et utiliser le pilote char\n 2) Fermer le pilote char\n");
		scanf("%s", userChoice);
		//Open the char device		
		if (userChoice=='1'){
			
			userChoice=0;
			while (userChoice!='q'){
				printf("Operations possibles :\n 1) Read Buf\n 2) Write Buf\n 3)Configure Buf\n");
				scanf("%s", userChoice);
			}
			
		}
		//Close the char device
		else if(userChoice=='2'){

		}
		else{
			printf("Fermeture de l'application");
		}
		
	   return EXIT_SUCCESS;
}
