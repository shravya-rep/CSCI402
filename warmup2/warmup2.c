/*
 * Author:      William Chia-Wei Cheng (bill.cheng@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<math.h>
#include <sys/time.h>
#include <errno.h>
#include<time.h>
#include<ctype.h>
#include<pthread.h>
#include <unistd.h>
#include<signal.h>
#include "cs402.h"

#include "my402list.h"
#define MAX 2147483647

sigset_t set;
int signalCaught=FALSE;

typedef struct CancelThreads{
    pthread_t* toCancelPacketThread;
    pthread_t* toCancelTokenThread;
}CancelTh;


int flag=-1;

char* tsfile;
FILE* fp;

int B=10;//10
int P=3;//3
int numOfPackets=20;//20

double lambda=1;//1
double mu=0.35;//(0.35)
double r=1.5;//1.5)

struct timeval formattedInterPacketTime;
struct timeval formattedServiceTime;
struct timeval formattedInterTokentime;

int tokensInBucket=0;
int numOfPacketsWhichReachedQ1=0;
int numOfPacketsWhichReachedQ2=0;
int numOfPacketsWhichReachedS1=0;
int numOfPacketsWhichReachedS2=0;
int NoOfPacketsDropped=0;

int allPacketsHaveArrived=FALSE;
//int Alldone=FALSE;
int tokenThreadStopped=FALSE;

typedef struct packet{
    int pID;
    int tokensNeeded;
    struct timeval idealArrivalTime;
    int serviceTime;
    struct timeval actualArrivalTime;
    struct timeval q1EntryTime;
    struct timeval q1LeavingTime;
    struct timeval q2EntryTime;
    struct timeval q2LeavingTime;
    struct timeval serviceBeginTime;
    struct timeval serviceEndTime;
    int servicedBy;
}EachPacket;

struct timeval endTime;
struct timeval startTime;



My402List Q1,Q2;
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv=PTHREAD_COND_INITIALIZER;

//double totalPacketInterArrTime;
double AvgPacketInterArrTime=0;

//double totalPacketServiceTime;
double AvgPacketServiceTime=0;

double totalTimeSpentinQ1=0;
double AvgTimeSpentByPacketsInQ1=0;

double totalTimeSpentinQ2=0;
double AvgTimeSpentByPacketsInQ2=0;

double totalTimeSpentinS1=0;
double AvgTimeSpentByPacketsAtS1=0;

double totalTimeSpentinS2=0;
double AvgTimeSpentByPacketsAtS2=0;

//double totalTimeSpentByPacketsInSystem;
double AvgTimeSpentByPacketsInSystem=0;

double AvgSqTimeSpentByPacketsInSystem=0;

//double TotalTimeSquare;
double StdDev=0;

int packetsTillNow=0;
int packetsCompletedTillNow=0;
int packetInSystemTillNow=0;
int NoOfPacketsCompleted=0;

int tokenNo=0;
int NoOfTokensDropped=0;


double tokenDropProb=0;
double packetDropProb=0;
/* ----------------------- main() ----------------------- */

static 
void printEMulParam()
{
    printf("Emulation Parameters:\n");
    printf("\tnumber to arrive = %d\n",numOfPackets);
    if(flag==0)
    {
        printf("\tlamda = %.6g\n",lambda);
        printf("\tmu = %.6g\n",mu);
    }
    
    printf("\tr = %.6g\n",r);
    printf("\tB = %d\n",B);
    if(flag==0)
    {
        printf("\tP = %d\n",P);
    }
    if(flag==1)
    {
        printf("\ttsfile = %s\n",tsfile);
    }
    
}


static
void calTimeDiff(struct timeval time2, struct timeval time1, struct timeval *difftime)
{
    timersub(&time2,&time1,difftime);
}

static
double printTime(struct timeval temp)
{
    return (temp.tv_sec)*1000+((temp.tv_usec)/1000.0);
}

static 
double printTimeInSec(struct timeval temp)
{
    return temp.tv_sec+(temp.tv_usec/1000000.0);
}

static
void AllocateCorrectTimeFromFile(int aTime, int* sec, int* usec)
{
    double d_time_in_milliseconds=(double)aTime;
    int seconds= (int)(d_time_in_milliseconds/1000.0);
    double d_interval_in_microseconds = d_time_in_milliseconds * ((double)1000.0);
    double d_microseconds = d_interval_in_microseconds - ((double)seconds) * ((double)1000000.0);
    int microseconds = round(d_microseconds);

    (*sec)=seconds;
    (*usec)=microseconds;
}
static 
void AllocateValueForPacket(EachPacket* pointPacket)
{
    if (flag==0)// take default values
    {
        pointPacket->tokensNeeded=P;
        if(formattedInterPacketTime.tv_sec>10)
        {
            pointPacket->idealArrivalTime.tv_sec=10;
            pointPacket->idealArrivalTime.tv_usec=0;
        }
        else
        {
            pointPacket->idealArrivalTime.tv_sec=formattedInterPacketTime.tv_sec;
            pointPacket->idealArrivalTime.tv_usec=formattedInterPacketTime.tv_usec;
        }


        if(formattedServiceTime.tv_sec>10)
        {
            pointPacket->serviceTime=10;
        }
        else
        {
            pointPacket->serviceTime=round(1000.0/mu);

        }
        
        
    }
    if(flag==1)
    {
        //read from file
        char buffer[2000];
        if(fgets(buffer,sizeof(buffer),fp)!=NULL)
        {
            if(strlen(buffer)>1024)
            {
                fprintf(stderr,"\t Wrong file format. Line cannot be is longer than 1,024 characters\n");
                exit(1);
            }
            int aTime=0,noTok=0,serTime=0;
            if(sscanf(buffer,"%d%d%d",&aTime,&noTok,&serTime)==3)
            {
                    int s;
                    int us;
                    AllocateCorrectTimeFromFile(aTime,&s,&us);
                    pointPacket->idealArrivalTime.tv_sec=s;
                    pointPacket->idealArrivalTime.tv_usec=us;

                    pointPacket->tokensNeeded=noTok;
                    pointPacket->serviceTime=serTime;





            }
            else
            {
                fprintf(stderr,"\t Wrong file format. Line does not contain 3 integers\n");
                exit(1);
            }

        }
        else
        {
            fprintf(stderr,"\t Wrong file format. Required line is empty\n");

        }


    }



}

static 
void AllocateValueForToken(struct timeval* interTokenTime)
{
    if(formattedInterTokentime.tv_sec>10)
    {
        (*interTokenTime).tv_sec=10;
        (*interTokenTime).tv_usec=0;

    }
    else
    {
        (*interTokenTime).tv_sec=formattedInterTokentime.tv_sec;
        (*interTokenTime).tv_usec=formattedInterTokentime.tv_usec;

    }

}



static 
void calAvgPacketInterArrTime(struct timeval difftime)
{
    if(packetsTillNow==0)
    {
        packetsTillNow++;
        AvgPacketInterArrTime=printTimeInSec(difftime)/packetsTillNow;


    }
    else
    {
        AvgPacketInterArrTime=((AvgPacketInterArrTime*packetsTillNow)+printTimeInSec(difftime))/(packetsTillNow+1);
        packetsTillNow++;
    }

}



static
void calAvgPacketServiceTime(struct timeval difftime)
{
    if(packetsCompletedTillNow==0)
    {
        packetsCompletedTillNow++;
        AvgPacketServiceTime=printTimeInSec(difftime)/packetsCompletedTillNow;
    }
    else
    {
        AvgPacketServiceTime=((AvgPacketServiceTime*packetsCompletedTillNow)+printTimeInSec(difftime))/(packetsCompletedTillNow+1);
        packetsCompletedTillNow++;
    }
}


static 
void calAvgPacketTimeInQ1(struct timeval difftime)
{
    totalTimeSpentinQ1+=printTimeInSec(difftime);
    

}

static 
void calAvgPacketTimeInQ2(struct timeval difftime)
{
    totalTimeSpentinQ2+=printTimeInSec(difftime);
    

}

static 
void calAvgPacketTimeInS1(struct timeval difftime)
{

    totalTimeSpentinS1+=printTimeInSec(difftime);
    

}

static 
void calAvgPacketTimeInS2(struct timeval difftime)
{
    totalTimeSpentinS2+=printTimeInSec(difftime);
    

}


static
void calAvgTimePacketInSystem(struct timeval difftime)
{
    if(packetInSystemTillNow==0)
    {
        packetInSystemTillNow++;
        AvgTimeSpentByPacketsInSystem=printTimeInSec(difftime)/packetInSystemTillNow;
        AvgSqTimeSpentByPacketsInSystem=pow(printTimeInSec(difftime),2);
    }
    else
    {
        AvgTimeSpentByPacketsInSystem=((AvgTimeSpentByPacketsInSystem*packetInSystemTillNow)+printTimeInSec(difftime))/(packetInSystemTillNow+1);
        AvgSqTimeSpentByPacketsInSystem=(((AvgSqTimeSpentByPacketsInSystem)*packetInSystemTillNow)+pow(printTimeInSec(difftime),2))/(packetInSystemTillNow+1);
        packetInSystemTillNow++;
    }


}



static
void calStdDev(struct timeval difftime)
{

    double var=AvgSqTimeSpentByPacketsInSystem-pow(AvgTimeSpentByPacketsInSystem,2);
    double smallVal=pow(10,-8);
    if(var<smallVal)
    {
        StdDev=0;
    }
    else
    {
        StdDev=sqrt(var);
    }



}


static
void calAvgs(EachPacket* donePacket)
{
    struct timeval diff;
    calTimeDiff(donePacket->q1LeavingTime,donePacket->q1EntryTime,&diff);
    calAvgPacketTimeInQ1(diff);
    calTimeDiff(donePacket->q2LeavingTime,donePacket->q2EntryTime,&diff);
    calAvgPacketTimeInQ2(diff);
    calTimeDiff(donePacket->serviceEndTime,donePacket->serviceBeginTime,&diff);
    if((donePacket->servicedBy)==1)
    {
        calAvgPacketTimeInS1(diff);
    }
    if((donePacket->servicedBy)==2)
    {
        calAvgPacketTimeInS2(diff);
    }

    


}




static
void printStats()
{
    printf("\nStatistics:\n\n");
    if(AvgPacketInterArrTime==0)
    {
        printf("\taverage packet inter-arrival time = N/A, no packets generated\n");
    }
    else
    {
        printf("\taverage packet inter-arrival time =%.6gs\n",AvgPacketInterArrTime);

    }
    if(packetsCompletedTillNow==0)
    {
        printf("\taverage packet service time = N/A, no packet served\n");
    }
    else
    {
        printf("\taverage packet service time =%.6gs\n\n",AvgPacketServiceTime);
    }
    
    printf("\taverage number of packets in Q1 =%.6g\n",AvgTimeSpentByPacketsInQ1);
    printf("\taverage number of packets in Q2 =%.6g\n",AvgTimeSpentByPacketsInQ2);
    printf("\taverage number of packets at S1 =%.6g\n",AvgTimeSpentByPacketsAtS1);
    printf("\taverage number of packets at S2 =%.6g\n\n",AvgTimeSpentByPacketsAtS2);

    if(packetsCompletedTillNow==0)
    {
        printf("\taverage time a packet spent in system = N/A no packet served\n");
    }
    else
    {
        printf("\taverage time a packet spent in system =%.6gs\n",AvgTimeSpentByPacketsInSystem);
    }

    if(packetsCompletedTillNow==0)
    {
        printf("\tstandard deviation for time spent in system = N/A no packet served\n");
    }
    else
    {
        printf("\tstandard deviation for time spent in system =%.6gs\n\n",StdDev);
    }
    
    if(tokenNo==0)
    {
        printf("\ttoken drop probability = N/A, no tokens generated\n");

    }
    else
    {
        tokenDropProb=((double)NoOfTokensDropped)/tokenNo;
        printf("\ttoken drop probability =%.6g\n",tokenDropProb);

    }
    if(packetsTillNow==0)
    {
        printf("\tpacket drop probability = N/A, no packets generated\n");
    }
    else
    {
        packetDropProb=((double)NoOfPacketsDropped)/packetsTillNow;
        printf("\tpacket drop probability =%.6g\n",packetDropProb);

    }




}


static 
void* packet(void* arg)
{

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,0);
    struct timeval sysTime;
    struct timeval relsysTime;
    struct timeval difftime;
    struct timeval prevPacketArrTime={0,0};

    int i=0;
    int packetNo=1;
    for(i=1;i<=numOfPackets;i++)
    {

        EachPacket* pointPacket=(EachPacket*)malloc(sizeof(EachPacket));
        memset(pointPacket,0,sizeof(EachPacket));
        pointPacket->pID=packetNo++;
        AllocateValueForPacket(pointPacket);
        gettimeofday(&sysTime,0);
        calTimeDiff(sysTime,startTime,&relsysTime);
        calTimeDiff(relsysTime,prevPacketArrTime,&difftime);
        struct timeval timeAlreadySpent=difftime;
        if(printTime(timeAlreadySpent)<printTime(pointPacket->idealArrivalTime))
        {
            calTimeDiff(pointPacket->idealArrivalTime,timeAlreadySpent,&difftime);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);
            usleep(printTime(difftime)*1000);
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,0);

        }
        pthread_mutex_lock(&m);
        if(signalCaught==TRUE)
        {
            pthread_mutex_unlock(&m);
            break;

        }
        gettimeofday(&sysTime,0);
        calTimeDiff(sysTime,startTime,&relsysTime);
        pointPacket->actualArrivalTime=relsysTime;
        
        if(pointPacket->tokensNeeded<=B)
        {
            calTimeDiff(pointPacket->actualArrivalTime,prevPacketArrTime,&difftime);

            //calAvgPacketInterArrTime(difftime,i);
            calAvgPacketInterArrTime(difftime);

            printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time =%.3fms \n",printTime(pointPacket->actualArrivalTime),pointPacket->pID,pointPacket->tokensNeeded,printTime(difftime));
            prevPacketArrTime=pointPacket->actualArrivalTime;
            if(!My402ListEmpty(&Q1))
            {
                My402ListAppend(&Q1,pointPacket);
                gettimeofday(&sysTime,0);
                calTimeDiff(sysTime,startTime,&difftime);
                pointPacket->q1EntryTime=difftime;
                numOfPacketsWhichReachedQ1++;
                printf("%012.3fms: p%d enters Q1\n",printTime(pointPacket->q1EntryTime),pointPacket->pID);
                
            }
            else
            {
                My402ListAppend(&Q1,pointPacket);
                gettimeofday(&sysTime,0);
                calTimeDiff(sysTime,startTime,&difftime);
                pointPacket->q1EntryTime=difftime;
                numOfPacketsWhichReachedQ1++;
                printf("%012.3fms: p%d enters Q1\n",printTime(pointPacket->q1EntryTime),pointPacket->pID);
                

                My402ListElem* firstInQ1=My402ListFirst(&Q1);
                if(tokensInBucket>=pointPacket->tokensNeeded)
                {
                    My402ListUnlink(&Q1,firstInQ1);
                    tokensInBucket=tokensInBucket-(pointPacket->tokensNeeded);
                    gettimeofday(&sysTime,0);
                    calTimeDiff(sysTime,startTime,&difftime);
                    pointPacket->q1LeavingTime=difftime;
                    calTimeDiff(pointPacket->q1LeavingTime,pointPacket->q1EntryTime,&difftime);
                    
                    printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n",printTime(pointPacket->q1LeavingTime),pointPacket->pID,printTime(difftime),tokensInBucket);
                    My402ListAppend(&Q2,pointPacket);
                    gettimeofday(&sysTime,0);
                    calTimeDiff(sysTime,startTime,&difftime);
                    pointPacket->q2EntryTime=difftime;
                    numOfPacketsWhichReachedQ2++;
                    printf("%012.3fms: p%d enters Q2\n",printTime(pointPacket->q2EntryTime),pointPacket->pID);
                    
                    if(!My402ListEmpty(&Q2))
                    {
                        pthread_cond_broadcast(&cv);
                    }

                }

            }
            


        }
        else
        {
            calTimeDiff(pointPacket->actualArrivalTime,prevPacketArrTime,&difftime);

            //calAvgPacketInterArrTime(difftime,i);
            calAvgPacketInterArrTime(difftime);
            printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time =%.3fms, dropped\n",printTime(pointPacket->actualArrivalTime),pointPacket->pID,pointPacket->tokensNeeded,printTime(difftime));
            prevPacketArrTime=pointPacket->actualArrivalTime;  
            NoOfPacketsDropped++;
            


        }
        



        pthread_mutex_unlock(&m);



    }

    pthread_mutex_lock(&m);
    if(signalCaught==FALSE)
    {
        allPacketsHaveArrived=TRUE;
        pthread_mutex_unlock(&m);
    }
    else
    {
        pthread_mutex_unlock(&m);

    }

return(0);
}

int ShouldTokenThreadTerminate()
{
    int returnVal=-1;
    pthread_mutex_lock(&m);
    returnVal=allPacketsHaveArrived&&My402ListEmpty(&Q1);
    pthread_mutex_unlock(&m);
    return returnVal;
}

static
void* token(void* arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,0);

    struct timeval sysTime;
    struct timeval relsysTime;
    struct timeval difftime;
    struct timeval prevTokenArrTime={0,0};
    struct timeval interTokenTime;
    struct timeval timeAlreadySpent;
    AllocateValueForToken(&interTokenTime);
    while(!ShouldTokenThreadTerminate())
    {
        gettimeofday(&sysTime,0);
        calTimeDiff(sysTime,startTime,&relsysTime);
        calTimeDiff(relsysTime,prevTokenArrTime,&difftime);
        timeAlreadySpent=difftime;
        if(printTime(timeAlreadySpent)<printTime(interTokenTime))
        {
            calTimeDiff(interTokenTime,timeAlreadySpent,&difftime);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);
            usleep(printTime(interTokenTime)*1000);
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,0);
        }
        
        
        pthread_mutex_lock(&m);
        if(signalCaught==TRUE)
        {
            pthread_mutex_unlock(&m);
            break;
        }
        tokenNo++;
        if(tokensInBucket<B)
        {
            tokensInBucket++;
            gettimeofday(&sysTime,0);
            calTimeDiff(sysTime,startTime,&difftime);
            prevTokenArrTime=difftime;
            printf("%012.3fms: token t%d arrives, token bucket now has %d token\n",printTime(difftime),tokenNo,tokensInBucket);

        }
        else
        {
            gettimeofday(&sysTime,0);
            calTimeDiff(sysTime,startTime,&difftime);
            prevTokenArrTime=difftime;
            printf("%012.3fms: token t%d arrives,dropped\n",printTime(difftime),tokenNo);
            NoOfTokensDropped++;
            

        }
        while(!My402ListEmpty(&Q1))
        {
            My402ListElem* firstInQ1=My402ListFirst(&Q1);
            if(tokensInBucket>=((EachPacket*)(firstInQ1->obj))->tokensNeeded)
            {
                EachPacket* firstInQ1Packet=(EachPacket*)firstInQ1->obj;
                My402ListUnlink(&Q1,firstInQ1);
                tokensInBucket=tokensInBucket-(firstInQ1Packet->tokensNeeded);
                gettimeofday(&sysTime,0);
                calTimeDiff(sysTime,startTime,&difftime);
                firstInQ1Packet->q1LeavingTime=difftime;
                calTimeDiff(firstInQ1Packet->q1LeavingTime,firstInQ1Packet->q1EntryTime,&difftime);
                
                printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n",printTime(firstInQ1Packet->q1LeavingTime),firstInQ1Packet->pID,printTime(difftime),tokensInBucket);
                My402ListAppend(&Q2,firstInQ1Packet);
                gettimeofday(&sysTime,0);
                calTimeDiff(sysTime,startTime,&difftime);
                firstInQ1Packet->q2EntryTime=difftime;
                numOfPacketsWhichReachedQ2++;
                printf("%012.3fms: p%d enters Q2\n",printTime(firstInQ1Packet->q2EntryTime),firstInQ1Packet->pID);
                
                if(!My402ListEmpty(&Q2))
                {
                    pthread_cond_broadcast(&cv);
                }



            }
            else
            {
                break;
            }
        }
        
        
        pthread_mutex_unlock(&m);
    }

    pthread_mutex_lock(&m);
    if(signalCaught==TRUE)
    {
        pthread_mutex_unlock(&m);

    }
    else
    {
        tokenThreadStopped=TRUE;
        pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&m);

    }


return(0);
}

static
void* server(void* arg)
{
    struct timeval sysTime;
    struct timeval difftime;
    int serverNo=(int)arg;
    
    
    while(TRUE)
    {

        pthread_mutex_lock(&m);
        while(My402ListEmpty(&Q2)&&!tokenThreadStopped&&!signalCaught)
        {
            pthread_cond_wait(&cv,&m);
        }

        if(signalCaught==TRUE)
        {
            pthread_mutex_unlock(&m);
            break;
        }

        if(My402ListEmpty(&Q2))
        {
            pthread_mutex_unlock(&m);
            break;

        }
        else
        {

            My402ListElem* firstInQ2=My402ListFirst(&Q2);
            EachPacket* firstInQ2Packet=(EachPacket*)firstInQ2->obj;
            My402ListUnlink(&Q2,firstInQ2);
            gettimeofday(&sysTime,0);
            calTimeDiff(sysTime,startTime,&difftime);
            firstInQ2Packet->q2LeavingTime=difftime;
            calTimeDiff(firstInQ2Packet->q2LeavingTime,firstInQ2Packet->q2EntryTime,&difftime);
            
            printf("%012.3fms: p%d leaves Q2, time in Q2 = %.3fms\n",printTime(firstInQ2Packet->q2LeavingTime),firstInQ2Packet->pID,printTime(difftime));
            gettimeofday(&sysTime,0);
            calTimeDiff(sysTime,startTime,&difftime);
            firstInQ2Packet->serviceBeginTime=difftime;
            if(serverNo==1)
            {
                numOfPacketsWhichReachedS1++;
                firstInQ2Packet->servicedBy=1;
            }
            if(serverNo==2)
            {
                numOfPacketsWhichReachedS2++;
                firstInQ2Packet->servicedBy=2;
            }
            printf("%012.3fms: p%d begins service at S%d, requesting %dms of service\n",printTime(firstInQ2Packet->serviceBeginTime),firstInQ2Packet->pID,serverNo,firstInQ2Packet->serviceTime);
            pthread_mutex_unlock(&m);
            //printf("Server has now unlocked the mutex\n");
            usleep((firstInQ2Packet->serviceTime)*1000);
            pthread_mutex_lock(&m);
            gettimeofday(&sysTime,0);
            calTimeDiff(sysTime,startTime,&difftime);
            firstInQ2Packet->serviceEndTime=difftime;
            calTimeDiff(firstInQ2Packet->serviceEndTime,firstInQ2Packet->serviceBeginTime,&difftime);
            struct timeval timeinsys;
            calTimeDiff(firstInQ2Packet->serviceEndTime,firstInQ2Packet->actualArrivalTime,&timeinsys);

            NoOfPacketsCompleted++;
            calAvgPacketServiceTime(difftime);
            calAvgTimePacketInSystem(timeinsys);
            calStdDev(timeinsys);
            calAvgs(firstInQ2Packet);


            printf("%012.3fms: p%d departs from S%d, service time = %.3fms, time in system = %.3fms\n",printTime(firstInQ2Packet->serviceEndTime),firstInQ2Packet->pID,serverNo,printTime(difftime),printTime(timeinsys));
            free(firstInQ2Packet);
            pthread_mutex_unlock(&m);

        }

    }

    pthread_mutex_lock(&m);
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&m);

    return(0);

}

static
void processInput(int argc, char *argv[])
{
   
    if(argc==1)
    {
        flag=0;
    }

    else
    {
    flag=0;
    int i;
    for(i=1;i<argc;i=i+2)
    {
        
        if(strcmp(argv[i],"-lambda")==0)
            {

                if(argc>=i+2)
                {
                    if(argv[i+1][0]!='-')
                    {
                        if(sscanf(argv[i+1],"%lf",&lambda)!=1)
                        {
                            fprintf(stderr,"\t(malformed command, the value for \"-lambda\" is incorrect.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                            exit(1);
                        }
                        else
                        {
                            sscanf(argv[i+1],"%lf",&lambda);
                        }

                    }
                    else
                    {
                        fprintf(stderr,"\t(malformed command, value for \"-lambda\" is missing or incorrect. It must be positive real no.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                        exit(1);

                    }
                }
                else
                {
                    fprintf(stderr,"\t(malformed command, value for \"-lambda\" is not given.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                    exit(1);

                }
            }
        else if(strcmp(argv[i],"-mu")==0)
            {
                

                if(argc>=i+2)
                {
                    if(argv[i+1][0]!='-')
                    {
                        if(sscanf(argv[i+1],"%lf",&mu)!=1)
                        {
                            fprintf(stderr,"\t(malformed command, the value for \"-mu\" is incorrect.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                            exit(1);
                        }
                        else
                        {
                            sscanf(argv[i+1],"%lf",&mu);
                        }

                    }
                    else
                    {
                        fprintf(stderr,"\t(malformed command, value for \"-mu\" is missing or incorrect. It must be positive real no.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                        exit(1);

                    }
                }
                else
                {
                    fprintf(stderr,"\t(malformed command, value for \"-mu\" is not given.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                    exit(1);

                }
            }
        else if(strcmp(argv[i],"-r")==0)
            {
               

                if(argc>=i+2)
                {
                    if(argv[i+1][0]!='-')
                    {
                        if(sscanf(argv[i+1],"%lf",&r)!=1)
                        {
                            fprintf(stderr,"\t(malformed command, the value for \"-r\" is incorrect.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                            exit(1);
                        }
                        else
                        {
                            sscanf(argv[i+1],"%lf",&r);
                        }

                    }
                    else
                    {
                        fprintf(stderr,"\t(malformed command, value for \"-r\" is missing or incorrect. It must be positive real no.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                        exit(1);

                    }
                }
                else
                {
                    fprintf(stderr,"\t(malformed command, value for \"-r\" is not given.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                    exit(1);

                }
            }
        else if(strcmp(argv[i],"-B")==0)
            {
                

                if(argc>=i+2)
                {
                    if(argv[i+1][0]!='-')
                    {
                        if(sscanf(argv[i+1],"%i",&B)!=1)
                        {
                            fprintf(stderr,"\t(malformed command, the value for \"-B\" is incorrect.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                            exit(1);
                        }
                        else
                        {
                            sscanf(argv[i+1],"%i",&B);
                            if(B>MAX)
                            {
                                fprintf(stderr,"\t(malformed command, the value for \"-B\" is incorrect.It cannot be greater than 2147483647)\n");
                                exit(1);

                            }

                        }

                    }
                    else
                    {
                        fprintf(stderr,"\t(malformed command, value for \"-B\" is missing or incorrect. It must be a positive integer.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                        exit(1);

                    }
                }
                else
                {
                    fprintf(stderr,"\t(malformed command, value for \"-B\" is not given.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                    exit(1);

                }
            }
        else if (strcmp(argv[i],"-P")==0)
            {
                

                if(argc>=i+2)
                {
                    if(argv[i+1][0]!='-')
                    {
                        if(sscanf(argv[i+1],"%i",&P)!=1)
                        {
                            fprintf(stderr,"\t(malformed command, the value for \"-P\" is incorrect.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                            exit(1);
                        }
                        else
                        {
                            sscanf(argv[i+1],"%i",&P);
                            if(P>MAX)
                            {
                                fprintf(stderr,"\t(malformed command, the value for \"-P\" is incorrect.It cannot be greater than 2147483647)\n");
                                exit(1);

                            }

                        }

                    }
                    else
                    {
                        fprintf(stderr,"\t(malformed command, value for \"-P\" is missing or incorrect. It must be a positive integer.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                        exit(1);

                    }
                }
                else
                {
                    fprintf(stderr,"\t(malformed command, value for \"-P\" is not given.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                    exit(1);

                }
            }
        else if(strcmp(argv[i],"-n")==0)
            {
                

                if(argc>=i+2)
                {
                    if(argv[i+1][0]!='-')
                    {
                        if(sscanf(argv[i+1],"%i",&numOfPackets)!=1)
                        {
                            fprintf(stderr,"\t(malformed command, the value for \"-n\" is incorrect.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                            exit(1);
                        }
                        else
                        {
                            sscanf(argv[i+1],"%i",&numOfPackets);
                            if(numOfPackets>MAX)
                            {
                                fprintf(stderr,"\t(malformed command, the value for \"-P\" is incorrect.It cannot be greater than 2147483647)\n");
                                exit(1);

                            }
                        }

                    }
                    else
                    {
                        fprintf(stderr,"\t(malformed command, value for \"-n\" is missing or incorrect.It must be a positive integer.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                        exit(1);

                    }
                }
                else
                {
                    fprintf(stderr,"\t(malformed command, value for \"-n\" is not given.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                    exit(1);

                }
            }
        else if(strcmp(argv[i],"-t")==0)
        {
            if(argc>=i+2)
            {
                flag=1;
                tsfile=argv[i+1];
                fp=fopen(tsfile,"r");
                if(fp==NULL)
                {
                    if(errno==2)
                   {
                        fprintf(stderr,"\t(input file \"%s\" does not exist)\n",argv[i+1]);
                        exit(1);;
                    }
                    if(errno==13)
                    {
                        fprintf(stderr,"\t(\"%s\" cannot be opened - access denied)\n",argv[i+1]);
                        exit(1);
                    }
            

                }
                char buffer[2000];
                if(fgets(buffer,sizeof(buffer),fp)==NULL)
                {
                    if(errno==0)
                    {
                        fprintf(stderr,"\t(Wrong file format. Input file \"%s\" is empty)\n",argv[i+1]);
                        exit(1);
                    }
                    if(errno==21)
                    {
                        fprintf(stderr,"\t(Wrong file format. Input file \"%s\" is a directory)\n",argv[i+1]);
                        exit(1);
                    }
                    
                }
                else
                {
                    char *stptr=buffer;
                    char *tabptr=strchr(stptr,'\t');
                    char *spptr=strchr(stptr,' ');
                    if(tabptr!=NULL||spptr!=NULL)
                    {
                        fprintf(stderr,"\t(Wrong file format.line 1 is not just a number)\n");
                        exit(1);
                    }


                    if(strlen(buffer)>1024)
                    {
                        fprintf(stderr,"\t(Wrong file format. line cannot be longer than 1,024 characters)\n");
                        exit(1);
                    }
                    char *dig;
                    dig= strtok(buffer,"\n");
                    int len=strlen(dig);
                    if((strcmp(dig," ")==0)||(strcmp(dig,"\t")==0)||(strcmp(dig+len-1," ")==0)||(strcmp(dig+len-1,"\t")==0))
                    {
                        fprintf(stderr,"\t(Wrong file format. There must be no leading or trailing space or tab characters in a line)\n");
                        exit(1);

                    }
                    numOfPackets=atoi(dig);
            
                    if(numOfPackets>MAX||numOfPackets<=0)
                    {
                        fprintf(stderr,"\t(Wrong file format.line 1 should be a number greater than 0 and lesser than 2147483647)\n");
                        exit(1);

                    }
            
                }
            }
            else
            {
                fprintf(stderr,"\t(malformed command, \"tsfile\" name missing)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n");
                exit(1);
            }
        }
        else
        {
            fprintf(stderr,"\t(malformed command,\"%s\" is not a valid commandline option.)\n\t(Correct usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile])\n",argv[i]);
            exit(1);

        }
        
    }
    }
}

static 
void correctTimeFormat()
{
    double d_interval_in_seconds = ((double)1.0) / r;
    int seconds = (int)d_interval_in_seconds;
    double d_interval_in_microseconds = d_interval_in_seconds * ((double)1000000.0);
    double d_microseconds = d_interval_in_microseconds - ((double)seconds) * ((double)1000000.0);
    int microseconds = round(d_microseconds);

    formattedInterTokentime.tv_sec=seconds;
    formattedInterTokentime.tv_usec=microseconds;

    d_interval_in_seconds = ((double)1.0) / lambda;
    seconds = (int)d_interval_in_seconds;
    d_interval_in_microseconds = d_interval_in_seconds * ((double)1000000.0);
    d_microseconds = d_interval_in_microseconds - ((double)seconds) * ((double)1000000.0);
    microseconds = round(d_microseconds);

    formattedInterPacketTime.tv_sec=seconds;
    formattedInterPacketTime.tv_usec=microseconds;

    d_interval_in_seconds = ((double)1.0) / mu;
    seconds = (int)d_interval_in_seconds;
    d_interval_in_microseconds = d_interval_in_seconds * ((double)1000000.0);
    d_microseconds = d_interval_in_microseconds - ((double)seconds) * ((double)1000000.0);
    microseconds = round(d_microseconds);

    formattedServiceTime.tv_sec=seconds;
    formattedServiceTime.tv_usec=microseconds;


}

static
void* monitorsignal(void* arg)
{


    CancelTh *p=(CancelTh*)arg;
    int sig;
    sigwait(&set,&sig);
    pthread_mutex_lock(&m);
    signalCaught=TRUE;
    pthread_cancel(*(p->toCancelPacketThread));
    pthread_cancel(*(p->toCancelTokenThread));
    pthread_cond_broadcast(&cv);


    struct timeval sysTime;
    struct timeval difftime;
    gettimeofday(&sysTime,0);
    calTimeDiff(sysTime,startTime,&difftime);
    printf("%012.3fms: SIGINT caught, no new packets or tokens will be allowed\n",printTime(difftime));
    pthread_mutex_unlock(&m);


    return(0);
}
    
    



int main(int argc, char *argv[])
{

    processInput(argc,argv);
    correctTimeFormat();
    printEMulParam();

    pthread_t packetThread;
    pthread_t tokenThread;
    pthread_t s1Thread;
    pthread_t s2Thread;
    My402ListInit(&Q1);
    My402ListInit(&Q2);

    pthread_t sigthread;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK,&set,0);

    CancelTh threadHandles={&packetThread,&tokenThread};
    pthread_create(&sigthread,0,monitorsignal,&threadHandles);


    gettimeofday(&startTime,0);
    struct timeval difftime;
    calTimeDiff(startTime,startTime,&difftime);
    printf("%012.3fms: emulation begins\n",printTime(difftime));

    if(pthread_create(&packetThread,0,packet,0)!=0)
    {
        perror("Error creating packet thread");
        exit(1);
    }
    if(pthread_create(&tokenThread,0,token,0)!=0)
    {
        perror("Error creating token thread");
        exit(1);
    }
    if(pthread_create(&s1Thread,0,server,(void*)1)!=0)
    {
        perror("Error creating server thread 1");
        exit(1);
    }
    if(pthread_create(&s2Thread,0,server,(void*)2)!=0)
    {
        perror("Error creating server thread 2");
        exit(1);
    }



    pthread_join(packetThread,0);
    pthread_join(tokenThread,0);
    pthread_join(s1Thread,0);
    pthread_join(s2Thread,0);

    if(signalCaught==TRUE)
    {
        My402ListElem* elem=NULL;
        My402ListElem* nextele=NULL;
        for(elem=My402ListFirst(&Q1);elem!=NULL;elem=nextele)
        {
            EachPacket* packetToRemove=(EachPacket*)elem->obj;
            struct timeval sysTime;
            gettimeofday(&sysTime,0);
            calTimeDiff(sysTime,startTime,&difftime);
            printf("%012.3fms: p%d removed from Q1\n",printTime(difftime),packetToRemove->pID);
            nextele=My402ListNext(&Q1,elem);
            My402ListUnlink(&Q1,elem);
            free(packetToRemove);
        }

        for(elem=My402ListFirst(&Q2);elem!=NULL;elem=nextele)
        {
            EachPacket* packetToRemove=(EachPacket*)elem->obj;
            struct timeval sysTime;
            gettimeofday(&sysTime,0);
            calTimeDiff(sysTime,startTime,&difftime);
            printf("%012.3fms: p%d removed from Q2\n",printTime(difftime),packetToRemove->pID);
            nextele=My402ListNext(&Q2,elem);
            My402ListUnlink(&Q2,elem);
            free(packetToRemove);
        }



    }


    gettimeofday(&endTime,0);
    calTimeDiff(endTime,startTime,&difftime);
    printf("%012.3fms: emulation ends\n",printTime(difftime));
    if(printTime(difftime)!=0)
    {
        AvgTimeSpentByPacketsInQ1=totalTimeSpentinQ1/printTimeInSec(difftime);
        AvgTimeSpentByPacketsInQ2=totalTimeSpentinQ2/printTimeInSec(difftime);
        AvgTimeSpentByPacketsAtS1=totalTimeSpentinS1/printTimeInSec(difftime);
        AvgTimeSpentByPacketsAtS2=totalTimeSpentinS2/printTimeInSec(difftime);
    }
    printStats();






    return(0);
}
