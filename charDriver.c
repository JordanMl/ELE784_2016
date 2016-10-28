///Includes d'origine
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

///Nouveaux includes :
#include <linux/rwsem.h>  ///semaphore lecteur/ecrivain
#include <linux/sched.h> // Required for task states (TASK_INTERRUPTIBLE etc )
#include <linux/ioctl.h> // Used for ioctl command
#include "ioctlcmd.h"

#define READWRITE_BUFSIZE 16
#define DEFAULT_BUFSIZE 256

MODULE_LICENSE("Dual BSD/GPL");

//----Function prototypes-----
int buf_init(void);
void buf_exit(void);
int buf_open (struct inode *inode, struct file *filp);
int buf_release (struct inode *inode, struct file *filp);
ssize_t buf_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops);
ssize_t buf_write (struct file *filp, const char __user *ubuf, size_t count,
                   loff_t *f_ops);
long buf_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

//----Data structures-----
struct BufStruct
{
    unsigned int InIdx;
    unsigned int OutIdx;
    unsigned short BufFull;
    unsigned short BufEmpty;
    unsigned int BufSize;
    unsigned char *Buffer;
} Buffer;

struct Buf_Dev
{
    wait_queue_head_t inq, outq;
    unsigned char *ReadBuf;
    unsigned char *WriteBuf;
    struct rw_semaphore rw_semBuf;
    unsigned short numWriter;
    unsigned short numReader;
    unsigned short numData;
    dev_t dev;  							/* Device number */
    struct cdev cdev;  					/* Char device structure */
    struct class *BufferDev_class;
} BDev;

//Structure à peupler, fonctions accessibles par l'utilisateur
struct file_operations Buf_fops =
{
    .owner = THIS_MODULE,
    .open = buf_open,
    .release = buf_release,
    .read = buf_read,
    .write = buf_write,
    .unlocked_ioctl = buf_ioctl,
};


//----Functions-----


int BufIn (struct BufStruct *Buf, unsigned char *Data)
{
    if (Buf->BufFull)
        return -1;
    Buf->BufEmpty = 0;
    Buf->Buffer[Buf->InIdx] = *Data;
    Buf->InIdx = (Buf->InIdx + 1) % Buf->BufSize;
    if (Buf->InIdx == Buf->OutIdx)
        Buf->BufFull = 1;
    return 0;
}

int BufOut (struct BufStruct *Buf, unsigned char *Data)
{
    if (Buf->BufEmpty)
        return -1;
    Buf->BufFull = 0;
    *Data = Buf->Buffer[Buf->OutIdx];
    Buf->OutIdx = (Buf->OutIdx + 1) % Buf->BufSize;
    if (Buf->OutIdx == Buf->InIdx)
        Buf->BufEmpty = 1;
    return 0;
}


//Initialisation du pilote
int buf_init(void) {

    //unsigned short BufTab[DEFAULT_BUFSIZE]; //Tableau -> Buffer circulaire
    int result = 0;
    printk(KERN_ALERT"Buffer_init (%s:%u) => CharPilote is Alive !!\n", __FUNCTION__, __LINE__);

    //Init du Buffer
    Buffer.InIdx = 0;
    Buffer.OutIdx = 0;
    Buffer.BufFull = 0;
    Buffer.BufEmpty = 1;
    Buffer.BufSize = DEFAULT_BUFSIZE;

    Buffer.Buffer = (unsigned char*) kmalloc(DEFAULT_BUFSIZE * sizeof(unsigned char), GFP_KERNEL); //dynamic allocation to create the circular buffer
    if (!Buffer.Buffer)
        return -ENOMEM;
    BDev.WriteBuf = (unsigned char*) kmalloc(READWRITE_BUFSIZE * sizeof(unsigned char), GFP_KERNEL);
	if (!BDev.WriteBuf) {
		return -ENOMEM;
    }
    BDev.ReadBuf = (unsigned char*) kmalloc(READWRITE_BUFSIZE * sizeof(unsigned char), GFP_KERNEL);
	if (!BDev.WriteBuf) {
		return -ENOMEM;
    }

    //Init rw semaphore pour BDev
    init_rwsem (&BDev.rw_semBuf);

	 //Init du BDev
	 BDev.numWriter = 0;
     BDev.numReader = 0;
     BDev.numData = 0;

     //Init des files d'attentes
    init_waitqueue_head (&BDev.inq);
    init_waitqueue_head (&BDev.outq);

    //Allocation dynamique de l'unité-matériel
    result = alloc_chrdev_region (&BDev.dev, 0, 1, "charDriverDev" );
    if (result< 0)
        printk(KERN_WARNING"buf_init ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
    else
        printk(KERN_WARNING"buf_init : MAJOR = %u MINOR = %u \n", MAJOR(BDev.dev), MINOR(BDev.dev));

    //Création de notre noeud d'unité-matériel
    BDev.BufferDev_class =  class_create(THIS_MODULE, "charDriverDev");
    device_create(BDev.BufferDev_class, NULL, BDev.dev, NULL, "charDriverDev_node");
    cdev_init(&BDev.cdev, &Buf_fops);
    BDev.cdev.owner = THIS_MODULE;
    if(cdev_add(&BDev.cdev, BDev.dev, 1) < 0)
        printk(KERN_WARNING"BDev ERROR IN cdev_add (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

    return 0;
}

///Fermeture du pilote
void buf_exit(void) {
    kfree(Buffer.Buffer); //Free cicular buffer memory
    kfree(BDev.ReadBuf);
    kfree(BDev.WriteBuf);

    cdev_del(&BDev.cdev);   //remove cdev from the system
    unregister_chrdev_region(BDev.dev, 1);   //free major number allocation
    device_destroy(BDev.BufferDev_class, BDev.dev);
    class_destroy(BDev.BufferDev_class);

    printk(KERN_ALERT"Buffer_exit (%s:%u) => CharPilote is dead !!\n", __FUNCTION__, __LINE__);
}

//Accès au pilote par l'usager
int buf_open (struct inode *inode, struct file *filp){

    //Ouverture pour ecriture ou lecture/ecriture
    if ( ((filp->f_flags & O_ACCMODE) == O_WRONLY) || ((filp->f_flags & O_ACCMODE) == O_RDWR) ){

          if (BDev.numWriter!=0){
			printk(KERN_ALERT"Buffer_open (%s:%u) \n => numWriter!=0 -> ENOTTY \n", __FUNCTION__, __LINE__);
			return -ENOTTY;
		  }
		  down_write (&BDev.rw_semBuf); //Capture le verrou d'ecriture
		  printk(KERN_ALERT"Buffer_open (%s:%u) \n => Capture le verrou d'ecriture \n", __FUNCTION__, __LINE__);
		  BDev.numWriter++;
		  up_write (&BDev.rw_semBuf); //Relache le verrou d'ecriture

		  if((filp->f_flags & O_ACCMODE) == O_RDWR)
		  		printk(KERN_WARNING"buf_open in read/write mode (%s:%u)\n", __FUNCTION__, __LINE__);
		  if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
				printk(KERN_WARNING"buf_open in write only mode (%s:%u)\n", __FUNCTION__, __LINE__);
    }

    //Ouveture en lecture seule
    else if ( (filp->f_flags & O_ACCMODE) == O_RDONLY){
		  down_read (&BDev.rw_semBuf); //Capture un verrou de lecture
        BDev.numReader++;
		  up_read (&BDev.rw_semBuf); //Relache le verrou de lecture
		  printk(KERN_WARNING"buf_open in read only mode (%s:%u)\n", __FUNCTION__, __LINE__);
    }

    printk(KERN_WARNING"buf_open success (%s:%u)\n", __FUNCTION__, __LINE__);
	 return 0;
}

//Libération du pilote par l'usager
int buf_release (struct inode *inode, struct file *filp) {

	 //struct Buf_Dev *dev = filp->private_data;

	 if (filp->f_mode & FMODE_READ){
	 	 down_read (&BDev.rw_semBuf);
	     printk(KERN_ALERT"Buffer_release (%s:%u) \n => Capture le verrou de lecture (FMODE_READ)  \n", __FUNCTION__, __LINE__);
		 BDev.numReader--;
	     up_read (&BDev.rw_semBuf);
	 }
	 if (filp->f_mode & FMODE_WRITE){
	 	 down_write (&BDev.rw_semBuf);
	     printk(KERN_ALERT"Buffer_release (%s:%u) \n => Capture le verrou d'ecriture (FMODE_READ))  \n", __FUNCTION__, __LINE__);
		 BDev.numWriter--;
		 up_write (&BDev.rw_semBuf);
	 }

	 printk(KERN_WARNING"buf_release (%s:%u)\n", __FUNCTION__, __LINE__);
	 return 0;
}

ssize_t buf_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops){

	 int nbReadChar=0;

     //Debut de region critique
	 down_read (&BDev.rw_semBuf);
     printk(KERN_ALERT"Buffer_read (%s:%u) \n => Capture le verrou de lecture  \n", __FUNCTION__, __LINE__);

     //Boucle : tant que le buffer circulaire est vide
     while(Buffer.BufEmpty){
        up_read (&BDev.rw_semBuf); //libère le sémaphore avant de dormir
        printk(KERN_ALERT"buf_read : read sleep (%s:%u) \n", __FUNCTION__, __LINE__);
        if(filp->f_flags & O_NONBLOCK){
            return -EAGAIN;
        }
        if(wait_event_interruptible (BDev.outq,Buffer.BufEmpty)){  //endort la tache
            return -ERESTARTSYS;
        }
        down_read (&BDev.rw_semBuf);
     }

    printk(KERN_ALERT"Buffer_read (%s:%u) \n => Buffer.InIdx=%d  \n", __FUNCTION__, __LINE__,Buffer.InIdx);
    printk(KERN_ALERT"Buffer_read (%s:%u) \n => Buffer.OutIdx=%d  \n", __FUNCTION__, __LINE__,Buffer.OutIdx);
    for(nbReadChar=0; nbReadChar<count; nbReadChar++){
    printk(KERN_ALERT"Buffer_read (%s:%u) \n => Lecture dans le buffer : Buffer.Buffer[%d] = %c  \n", __FUNCTION__, __LINE__,nbReadChar,Buffer.Buffer[nbReadChar]);
         if (BufOut(&Buffer, &BDev.ReadBuf[nbReadChar])){
             printk(KERN_ALERT"Buffer_read (%s:%u) \n => Lecture dans le buffer : BDev.ReadBuf[%d] = %c  \n", __FUNCTION__, __LINE__,nbReadChar,BDev.ReadBuf[nbReadChar]);
             break;
         }
         BDev.numData--;
     }

    printk(KERN_ALERT"Buffer_read (%s:%u) \n => lecture de %d caracters  \n", __FUNCTION__, __LINE__,nbReadChar);
     //Tentative d'envoie des caractères lu à l'utilisateur
     if (copy_to_user(ubuf, BDev.ReadBuf, nbReadChar)) {
         //Tous les bits n'ont pas été echangé
         up_read (&BDev.rw_semBuf); //libère le sémaphore avant de renvoyer l'erreur
         return -EFAULT;
     }
    printk(KERN_ALERT"Buffer_read (%s:%u) \n => Copy to user OK  \n", __FUNCTION__, __LINE__);
    ///Fin de region critique
    up_read (&BDev.rw_semBuf);
    return nbReadChar;
}

ssize_t buf_write (struct file *filp, const char __user *ubuf, size_t count,
                   loff_t *f_ops){

     int nbWriteChar = 0;

     printk(KERN_WARNING"buf_write start(%s:%u)\n", __FUNCTION__, __LINE__);

     //Début région critique
     down_write (&BDev.rw_semBuf);

     //Boucle : tant que le buffer d'ecriture est plein
     while(Buffer.BufFull){
        up_write (&BDev.rw_semBuf); //libère le sémaphore avant de dormir
        printk(KERN_WARNING"buf_write sleep(%s:%u)\n", __FUNCTION__, __LINE__);
        if(filp->f_flags & O_NONBLOCK){
            return -EAGAIN;
        }
        if (wait_event_interruptible (BDev.inq,Buffer.BufFull)){ //endort la tâche
            return -ERESTARTSYS;
        }
        down_write (&BDev.rw_semBuf);
     }

     printk(KERN_ALERT"buf_write : data copy from user... (%s:%u) \n", __FUNCTION__, __LINE__);
     //Tentative de récupération des caractères envoyé par l'utilisateur
     if(copy_from_user(BDev.WriteBuf, ubuf, count)){
        //Tous les bits n'ont pas été echangé
        up_write (&BDev.rw_semBuf); //libère le sémaphore avant de renvoyer l'erreur
        return -EFAULT;
     }

     for(nbWriteChar=0;nbWriteChar<count;nbWriteChar++){
        if(BufIn(&Buffer, &BDev.WriteBuf[nbWriteChar])){
            printk(KERN_WARNING"buf_write (%s:%u)\n  Bufin return < 0 ", __FUNCTION__, __LINE__);
        }
        else{
            printk(KERN_WARNING"buf_write (%s:%u)\n  WriteBuf[%d]= %c copie dans Buffer\n", __FUNCTION__, __LINE__,nbWriteChar,BDev.WriteBuf[nbWriteChar]);
            break;
        }
        BDev.numData++;
     }
	 printk(KERN_ALERT"buf_write : function reads %d caracter(s)(%s:%u) \n",nbWriteChar, __FUNCTION__, __LINE__);

	 printk(KERN_WARNING"buf_write (%s:%u)\n  END  \n   ", __FUNCTION__, __LINE__);
	 ///Fin de region critique
	 up_write (&BDev.rw_semBuf);
	 return count;
}

long buf_ioctl (struct file *flip, unsigned int cmd, unsigned long arg){

    int err = 0, i = 0;
    long retval = 0;
    unsigned long tmp;
    unsigned char *newBuf;

    /* verification que la commande corespond à notre pilote */
    if(_IOC_TYPE(cmd) != CHARDRIVER_IOC_MAGIC){
        printk(KERN_WARNING"ioctl(%s:%u) Magic number invalid %c", __FUNCTION__, __LINE__,_IOC_TYPE(cmd));
        return -ENOTTY;
    }
    if(_IOC_NR(cmd) > CHARDRIVER_IOC_MAXNR) {
        printk(KERN_WARNING"ioctl(%s:%u) numéro de commande non valide %d", __FUNCTION__, __LINE__,_IOC_NR(cmd));
        return -ENOTTY;
    }

    /* verification de la possibilité de lire ou ecrire */
    if(_IOC_DIR(cmd) & _IOC_READ){
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
        printk(KERN_WARNING"ioctl(%s:%u) erreur impossible de lire %d", __FUNCTION__, __LINE__,err);
    }
    else if(_IOC_DIR(cmd) & _IOC_READ){
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
        printk(KERN_WARNING"ioctl(%s:%u) erreur impossible d'écrire %d", __FUNCTION__, __LINE__,err);
    }
    if(err){
        return -EFAULT;
    }

    switch(cmd){
        case CHARDRIVER_IOC_GETNUMDATA :   retval = BDev.numData;
                                           printk(KERN_WARNING"ioctl(%s:%u) GetNumData : %d /n", __FUNCTION__, __LINE__,BDev.numData);
                                           break;
        case CHARDRIVER_IOC_GETNUMREADER : retval = BDev.numReader;
                                           printk(KERN_WARNING"ioctl(%s:%u) GetNumReader : %d /n", __FUNCTION__, __LINE__,BDev.numReader);
                                           break;
        case CHARDRIVER_IOC_GETBUFSIZE :   retval = Buffer.BufSize;
                                           printk(KERN_WARNING"ioctl(%s:%u) GetBufSize : %d /n", __FUNCTION__, __LINE__,Buffer.BufSize);
                                           break;
        case CHARDRIVER_IOC_SETBUFSIZE :   if(!capable(CAP_SYS_ADMIN)){
                                                printk(KERN_WARNING"ioctl(%s:%u) Permission non accordé/n", __FUNCTION__, __LINE__);
                                                return -EPERM;
                                           }
                                           tmp = arg;

                                           ///Debut région critique
                                           down_write(&BDev.rw_semBuf);
                                           //Vérifie que la taille du nouveau Buffer puisse contenir toute les données présentes
                                           if(tmp<(BDev.numData)){
                                                up_write(&BDev.rw_semBuf); //libère le sémaphore avant de retourner un code d'erreur
                                                printk(KERN_WARNING"ioctl(%s:%u) BufSize %d < numData %d", __FUNCTION__, __LINE__,tmp,BDev.numData);
                                                return -EADV;
                                           }

                                           //allocation d'une nouvelle zone mémoire
                                           newBuf = kmalloc(tmp, GFP_KERNEL);
                                           //remplis la nouvelle zone mémoire avec les données de notre buffer
                                           for (i=0;i<(BDev.numData);i++){
                                            newBuf[i] = Buffer.Buffer[i];
                                           }
                                           //libère l'ancienne zone mémoire de notre buffer
                                           kfree(Buffer.Buffer);
                                           //fait pointer notre buffer vers la nouvelle zone mémoire
                                           Buffer.Buffer = newBuf;
                                           Buffer.BufSize = tmp;

                                           ///Fin région critique
                                           up_write(&BDev.rw_semBuf);
                                           printk(KERN_WARNING"ioctl(%s:%u) SetBufSize : %d", __FUNCTION__, __LINE__,Buffer.BufSize);
                                           retval = 0;
                                           break;
        default : return -ENOTTY;
    }

    printk(KERN_WARNING"buf_ioctl (%s:%u)\n   END /n ", __FUNCTION__, __LINE__);
	return retval;
}

///Point d'entree pilote
module_init(buf_init);
///Point de sortie pilote
module_exit(buf_exit);
