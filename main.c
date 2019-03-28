/******************************************************************************/
/*                                                                            */  
/* Gozde DOGAN - 131044019                                                    */
/* CSE344 System Programming - Homework 2                                     */
/*                                                                            */ 
/* Her directory icin fork yapildi. (input olarak girilen directory dahil)    */
/* Bu nedenle child process sayisi hesabinda input olarak girilen directory   */
/* icin yapilan fork islemi sonucu olusan child process de dahildir.          */
/*                                                                            */   
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>  //wait fonksiyonlari icin
#include <dirent.h> 
#include <errno.h>    
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_PATH_SIZE 1024
#define DEBUG

/****************************** Global Veriables ******************************/
int iIs_z = 0;
int iFirstSize = 0;
int iIndex = 0;
char sOldFname[MAX_PATH_SIZE] = "";
int iOldSizeOfDir = 0;
static int iNumOfChildProcess = 0;
char sTempFile[MAX_PATH_SIZE] = "tempChildProcess.txt";
FILE *fPtrTemp;
char sSizeFiles[MAX_PATH_SIZE] = "131044019sizes.txt";
FILE *fPtrSizeFile;

char lockFname[256] = "131044019_LockFile";
int fdLockFile = -1;
struct flock lock = {
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = 1,
  };
/******************************************************************************/


/**************************** Function Definitions ****************************/
void doOperation(int argc, char *argv[]);
int postOrderApply(char *path, int pathfun (char *path1));
int sizepathfun (char *path);
void usage();
int isInSameDirectory(char *sString1, char *sString2);
void readFile(char filename[MAX_PATH_SIZE]);
/******************************************************************************/


/******************************** START OF MAIN *******************************/
int main(int argc, char *argv[])
{
	if((argc != 2 && argc != 3) || (argc == 3 && strcmp(argv[1], "-z") != 0))
	{
		usage();
		return 0;
	}
	
    doOperation(argc, argv);
    		
	return 0;
}
/********************************** END OF MAIN *******************************/

/************************** Function Declaration ******************************/

/*
 * Kullanici tarafindan girilen compiler kodu kontrol edilerek 
 * islemler gerceklestiriliyor.
 * Kullanicinin calistiriken "-z" komutunu girip girmedigi kontrol eidliyor.
 * Bunun sonucuna gore ekrana yazdirilacak seyler ve klasor boyutlarinin 
 * toplanma sekli degismektedir.
 *
 * input: 
 *  argc: a number of string in the compiler code
 *  argv: strings in the compiler code
 */
void doOperation(int argc, char *argv[])
{
    int totalSize = 0;
    pid_t mainProcessPid = getpid();
    sprintf(sOldFname, "%s", argv[argc-1]);
    
    fdLockFile = open(lockFname, O_RDWR | O_CREAT, 0666);
    if(fdLockFile == -1)
    {
        perror("open: ");
        exit(EXIT_FAILURE);
    }
    
    if(unlink(lockFname) == -1)
    {
        perror("unlink: ");
        exit(EXIT_FAILURE);
    }    
    
    if(argc == 3 && strcmp(argv[1], "-z") == 0)
        iIs_z = 1;
     
    if(argc == 2)
    {
        #ifndef DEBUG
            fprintf(stderr, "Output of \"buNeDuFork %s\" don't add subdirectory sizes:\n", argv[1]);
            fprintf(stderr, "PID \t SIZE \t\t PATH \n");
        #endif
        
        fPtrSizeFile = fopen(sSizeFiles, "a+");
        fprintf(fPtrSizeFile, "Output of \"buNeDu %s\" don't add subdirectory sizes:\n", argv[1]);
        fprintf(fPtrSizeFile, "PID \t SIZE \t\t PATH \n");
        fclose(fPtrSizeFile);
        
        totalSize = postOrderApply(argv[argc-1], sizepathfun);
        
        #ifndef DEBUG
            fprintf(stderr, "%d\t%d\t\t%s\n", mainProcessPid, iFirstSize, argv[argc-1]); 
        #endif
        
        fPtrSizeFile = fopen(sSizeFiles, "a+");
        fprintf(fPtrSizeFile, "%d\t%d\t\t%s\n", mainProcessPid, iFirstSize, argv[argc-1]); 
        fclose(fPtrSizeFile);     
    }
    
    else if(argc == 3)
    {
        #ifndef DEBUG
            fprintf(stderr, "Output of \"buNeDuFork %s %s\" gives total sizes:\n", argv[1], argv[2]);
            fprintf(stderr, "PID \t SIZE \t\t PATH \n");
        #endif
        
        fPtrSizeFile = fopen(sSizeFiles, "a+");
        fprintf(fPtrSizeFile, "Output of \"buNeDuFork %s %s\" gives total sizes:\n", argv[1], argv[2]);
        fprintf(fPtrSizeFile, "PID \t SIZE \t\t PATH \n");
        fclose(fPtrSizeFile);
        
        totalSize = postOrderApply(argv[argc-1], sizepathfun);
        
        #ifndef DEBUG
            fprintf(stderr, "%d\t%d\t\t%s\n", mainProcessPid, totalSize+iFirstSize, argv[argc-1]);
        #endif 
        
        fPtrSizeFile = fopen(sSizeFiles, "a+");
        fprintf(fPtrSizeFile, "%d\t%d\t\t%s\n", mainProcessPid, totalSize+iFirstSize, argv[argc-1]); 
        fclose(fPtrSizeFile);
    }
    else
    {
        usage();
        exit(EXIT_FAILURE);
    }
    
    fPtrTemp = fopen(sTempFile, "r");
    fscanf(fPtrTemp, "%d", &iNumOfChildProcess);
    fclose(fPtrTemp);
    remove(sTempFile);
    
    #ifndef DEBUG
        fprintf(stderr, "%d child processes created. Main process is %d.\n", iNumOfChildProcess, mainProcessPid);
    #endif
    
    fPtrSizeFile = fopen(sSizeFiles, "a+");
    fprintf(fPtrSizeFile, "%d child processes created. Main process is %d.\n\n\n", iNumOfChildProcess, mainProcessPid);    
    fclose(fPtrSizeFile);
    
    readFile(sSizeFiles);
}


/*
 * Directory icindeki directory'lerin boyutlarini buluyor.
 * Kullanici tarafindan girilen calistirma kodundaki "-z" degerinin 
 * varligina gore hesaplama yapilmakta.
 *
 * input: 
 *  path: directory name
 *  int pathfun(char *path1): function name
 * output:
 *  int: total size of directories in input directory
 */
int postOrderApply(char *path, int pathfun (char *path1))
{
    DIR *dir;
    struct dirent *dosya;																	
	struct stat status, status_1;  //Degisen fname ile birlikte status durumunu alir ve S_ISDIR fonksiyonunda kullanir

	//Variables
	int iSizeFname = 0;
	char fname[MAX_PATH_SIZE], path_1[MAX_PATH_SIZE]; //dosyanin adini tutar
	static int iSizeOfTotalDir = 0;
	int iSizeOfDir = 0;
	int iPrintSize = 0;
	pid_t childPid;
	int status1;
	static int iTotalSize = 0;
	int err = -1000;
	
    if (lstat(path, &status_1) < 0)
    {
        perror ("lstat");
        exit(EXIT_FAILURE); 
    }

    if (!S_ISDIR(status_1.st_mode))
    {
        fprintf(stderr, "%s not a directory!!\n", path);
        exit(EXIT_FAILURE); 
    }
	// input olarak girilen klasorun icindeki dosyalarin boyutlari bulundu.
	if(iIndex == 0)
	{
	    iFirstSize = pathfun(path);
	    iOldSizeOfDir = iFirstSize;
	    iIndex++;
	}
   
   childPid = fork();
		                
    if(childPid == -1)
    {
        perror("fork: ");
        exit(EXIT_FAILURE);
    }

    int iNumOfChildProcessOld = 0;

    fPtrTemp = fopen(sTempFile, "r");
    if(fPtrTemp != NULL)
    {
        fscanf(fPtrTemp, "%d", &iNumOfChildProcessOld);
        fclose(fPtrTemp);
        
        //fprintf(stderr, "--iNumOfChildProcessOld: %d\n", iNumOfChildProcessOld);
        
        if(iNumOfChildProcessOld > iNumOfChildProcess+1)
            iNumOfChildProcess = iNumOfChildProcessOld + iNumOfChildProcess;
    }                 

    iNumOfChildProcess++; // child process sayisi arttirildi		                

    if(childPid == 0)
    { 
        lock.l_type = F_WRLCK;
        err = fcntl(fdLockFile, F_SETLKW, &lock);
        if(err == -1)
        {
            perror("fcntl: ");
            exit(EXIT_FAILURE);
        }
        
        fPtrTemp = fopen(sTempFile, "w");
        fprintf(fPtrTemp, "%d", iNumOfChildProcess);
        fclose(fPtrTemp);
                            
        //fprintf(stderr, " CHILD - iNumOfChildProcess: %d\n", iNumOfChildProcess);
        
        //Directory acilabiliyor mu kontrolu yaptim
        if ((dir = opendir(path)) == NULL) {
		    //perror("opendir");
		    fprintf(stderr, "%d\t%s\n", pathfun(path), path);
		    exit(EXIT_FAILURE); 
	    }	

	    while ((dosya = readdir(dir)) != NULL) 
	    {	
	        //Dosya adi "." veya ".." olmadiginda islem yapilacak.
		    if ((strcmp(dosya->d_name, ".") != 0) && (strcmp(dosya->d_name, "..") != 0 )) 
		    {        		
		        sprintf(fname, "%s/%s", path, dosya->d_name); //dosya ismini fname'e attim
		        //fprintf(stderr, "%s\n", fname);

		        iSizeFname=strlen(fname);
		        if( fname[iSizeFname-1] != '~'  )
		        {   
		            #ifndef DEBUG
			            puts(fname);
		            #endif
			        if (lstat(fname, &status) == -1) 
			        {                            
				        //perror("lstat");                                  
				        break;
			        }
			        if (S_ISDIR(status.st_mode))   //File OR Directory kontrolu
			        {		                       // Directory ise islem yapilmaktadir.
			        
		                if(iIs_z != 0) // "-z" komutunun girildiginde add subdirectory
		                {
		                    iSizeOfDir = 0;
	                    	iSizeOfDir = pathfun(fname);
	                    	
	                    	int flag = 0;
	                    	
	                    	char sString[MAX_PATH_SIZE];
			                sprintf(sString, "%s", fname);
		                    
			                if(isInSameDirectory(sOldFname, sString) == 1) // ayni klasor icindeki klasorlerin boyutlari toplanmamali
			                {
			                    iPrintSize -= iOldSizeOfDir;
			                    flag = 1;
			                }
			                if(iSizeOfTotalDir + iPrintSize > 0) // en icteki klasore kadar ilk deger 0 olmaliydi
		                        iPrintSize -= iOldSizeOfDir;
		                    
		                    if(flag != 1)
		                        iSizeOfTotalDir += iSizeOfDir + postOrderApply(fname, pathfun);
	                        else
		                        iSizeOfTotalDir = iSizeOfDir - iOldSizeOfDir + postOrderApply(fname, pathfun);
		                   
		                    #ifndef DEBUG
			                    fprintf(stderr, "%d\t%d\t\t%s\n", getpid(), iSizeOfTotalDir, fname);
			                #endif
			                
			                fPtrSizeFile = fopen(sSizeFiles, "a+");
			                fprintf(fPtrSizeFile, "%d\t%d\t\t%s\n", getpid(), iSizeOfTotalDir, fname);
			                fclose(fPtrSizeFile);
		                    
		                    iOldSizeOfDir = iSizeOfDir;
			                    
			                sprintf(sOldFname, "%s", fname);  
		                }
		                else // "-z girilmediginde don't add subdirectory"
		                {	
		                
		                    iSizeOfTotalDir = 0;
		                    iSizeOfTotalDir = pathfun(fname);

		                    postOrderApply(fname, pathfun);
		                    
		                    #ifndef DEBUG
			                    fprintf(stderr, "%d\t%d\t\t%s\n", getpid(), iSizeOfTotalDir, fname);
			                #endif
		                	
		                	fPtrSizeFile = fopen(sSizeFiles, "a+");
		                	fprintf(fPtrSizeFile, "%d\t%d\t\t%s\n", getpid(), iSizeOfTotalDir, fname);
                            fclose(fPtrSizeFile);
		                }		
			        }																						
                }   				    
		    }
	    }  
	    while ((closedir(dir) == -1) && (errno == EINTR)) ;
	   
	    lock.l_type = F_UNLCK;
        err = fcntl(fdLockFile, F_SETLK, &lock);
        if(err == -1)
        {
            perror("fcntl: ");
            exit(EXIT_FAILURE);
        }
		
		_exit(iSizeOfTotalDir);
    }
    else
    {  
        lock.l_type = F_RDLCK;
        err = fcntl(fdLockFile, F_SETLK, &lock);
        if(err == -1)
        {
            perror("fcntl: ");
            exit(EXIT_FAILURE);
        }
    
        fPtrTemp = fopen(sTempFile, "r");
        if(fPtrTemp != NULL)
        {
            fscanf(fPtrTemp, "%d", &iNumOfChildProcess);
            fclose(fPtrTemp);
        
            //fprintf(stderr, " PARENT - iNumOfChildProcess: %d\n", iNumOfChildProcess); 
        }
        
        lock.l_type = F_UNLCK;
        err = fcntl(fdLockFile, F_SETLK, &lock);
        if(err == -1)
        {
            perror("fcntl: ");
            exit(EXIT_FAILURE);
        }
        
        wait(&status1);	
		iSizeOfTotalDir += WEXITSTATUS(status1);
    }		
    
    //fprintf(stderr, "SON - iSizeOfTotalDir: %d\n", iSizeOfTotalDir);
	return iSizeOfTotalDir;
}


/*
 * Directory icindeki dosyalarin boyutlarinin toplamini buluyor.
 * link ya da fifo gibi ozel bir file'a denk gelindiginde
 * dosyanın ozel oldugu yazilarak boyutu 0 kabul ediliyor.
 *
 * input: 
 *  path: directory name
 * output:
 *  int: total size of files in directory
 */
int sizepathfun (char *path)
{
    DIR *dir;
    struct dirent *dit;
    struct stat st;

    int iSizeFilePath = 0;
    int iSizeOfFile = 0;

    char filePath[MAX_PATH_SIZE];

    if ((dir = opendir(path)) == NULL) 
    {
		//perror("opendir");
		return 0;
	}
    
    while ((dit = readdir(dir)) != NULL)
    {
        if ((strcmp(dit->d_name, ".") != 0) && (strcmp(dit->d_name, "..") != 0 )) 
		{        
			sprintf(filePath, "%s/%s", path, dit->d_name);
			//lstat kullanildi, cunku ozel dosyalar boyle gorulebilmektedir.
            if (lstat(filePath, &st) == -1) 
			{                            
				//perror("stat");                                  
				break;
			}

			iSizeFilePath=strlen(filePath);
			if(filePath[iSizeFilePath-1] != '~')
			{ 
                if(!S_ISDIR(st.st_mode))
                {
			        if(S_ISLNK(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode))
			        {
			            #ifndef DEBUG
			                fprintf(stderr, "%d\tSpecial file\t%s\n", getpid(), filePath);
			            #endif
			            
			            fPtrSizeFile = fopen(sSizeFiles, "a+");
			            fprintf(fPtrSizeFile, "%d\tSpecial file\t%s\n", getpid(), filePath);
			            fclose(fPtrSizeFile);
		            }
		            else
		            {
                        #ifndef DEBUG
                            fprintf(stderr, "\tFile: %s --- Size: %d\n", filePath, (int)st.st_size);
                        #endif
                        iSizeOfFile += st.st_size;
                    }
                }
            }
        }
    }
    
    #ifndef DEBUG
        fprintf(stderr, "\n\tDir: %s --- Size: %d\n\n", path, iSizeOfFile);
    #endif
    
    closedir(dir);
    return iSizeOfFile;
}


/*
 * Iki path'in ayni directory icinde olup olamdigini buluyor.
 * Gelen iki path icindeki "/" karakteri sayisi bulundu.
 * Gelen iki path icinde de ayni degerde "/" karakteri varsa 
 * ayni directory icinde yer aldiklari dogrulanmis olur.
 *
 * input: 
 *  sString1: old directory name
 *  sString2: new directory name
 * output:
 *  int: returns 1 if they are in the same directory
 *       returns 0 if they are not in the same directory
 */
int isInSameDirectory(char *sString1, char *sString2)
{
    #ifndef DEBUG
        fprintf(stderr, "\n\nsString1: %s ---- sString2: %s\n", sString1, sString2);
    #endif
    
    const char s[2] = "/";
    int iCount1 = 0, iCount2 = 0;
    char *sToken1, *sToken2;
    
    sToken1 = strtok(sString1, s);
    while(sToken1 != NULL) 
    {
        iCount1++;
        sToken1 = strtok(NULL, s);
    }
    
    sToken2 = strtok(sString2, s);
    while(sToken2 != NULL) 
    {
        iCount2++;
        sToken2 = strtok(NULL, s);
    }
    
    #ifndef DEBUG
        fprintf(stderr, "iCount1: %d ---- iCount2:%d\n\n", iCount1, iCount2);
    #endif
    
    if(iCount1 == iCount2)
        return 1;
    return 0;    
}

void readFile(char filename[MAX_PATH_SIZE])
{
    char sTempStr[MAX_PATH_SIZE] = "";
    FILE *fPtrSizeFile = fopen(sSizeFiles, "a+");

    if (fPtrSizeFile == NULL)
    {
        perror("fopen: ");
        exit(EXIT_FAILURE);
    }

    while((fgets(sTempStr, 80, fPtrSizeFile)) != NULL)
      fprintf(stderr, "%s", sTempStr);
 

    fclose(fPtrSizeFile);
}


/*
 * Yanlis derleme sonrasi kullaniciya dogrusu gosterilmektedir
 */
void usage()
{
    fprintf(stderr, "Usage:\n");
	fprintf(stderr, "-----------------------------------\n");
	fprintf(stderr, "\t./buNeDuFork \"filename\" \n \t\tOR \n\t./buNeDu -z \"filename\"\n");
	fprintf(stderr, "-----------------------------------\n");
}

/******************************************************************************/
