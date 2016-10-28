README ELE784_2016
Johann Bruneau AN71290 BRUJ20069402
Jordan Moal AN69700 MOAJ16099301

installation du driver :
./installdriver.sh

pour le désinstaller : 
./uninstallDriver.sh

Pour lancer l'application : 
./application
sudo ./application => fonction ioctl setbufsize

contenu dossier src :
charDriver.c : code du driver
ioctlcmd.h : header des numéro de commande ioctl
Makefile : compilation du driver
application.c : code de l'application usager

contenu dossier bin : 
installdriver.sh : script pour l'insertion du driver
uninstallDriver.sh : script retirant le driver
charDriver.ko : code compiler du driver, prêt à être inséré
application : exécutable de l'application usager
