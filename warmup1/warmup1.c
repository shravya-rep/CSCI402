/*
 * Author:      William Chia-Wei Cheng (bill.cheng@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include<time.h>
#include<ctype.h>
#include "cs402.h"

#include "my402list.h"

typedef struct EachTransaction
{
    char drawOrDeposit;
    time_t timeStamp;
    int amountInCents;
    char* description;
}TransactionFields;

static
void BubbleForward(My402List *pList, My402ListElem **pp_elem1, My402ListElem **pp_elem2)
    /* (*pp_elem1) must be closer to First() than (*pp_elem2) */
{
    My402ListElem *elem1=(*pp_elem1), *elem2=(*pp_elem2);
    void *obj1=elem1->obj, *obj2=elem2->obj;
    My402ListElem *elem1prev=My402ListPrev(pList, elem1);
    My402ListElem *elem2next=My402ListNext(pList, elem2);

    My402ListUnlink(pList, elem1);
    My402ListUnlink(pList, elem2);
    if (elem1prev == NULL) {
        (void)My402ListPrepend(pList, obj2);
        *pp_elem1 = My402ListFirst(pList);
    } else {
        (void)My402ListInsertAfter(pList, obj2, elem1prev);
        *pp_elem1 = My402ListNext(pList, elem1prev);
    }
    if (elem2next == NULL) {
        (void)My402ListAppend(pList, obj1);
        *pp_elem2 = My402ListLast(pList);
    } else {
        (void)My402ListInsertBefore(pList, obj1, elem2next);
        *pp_elem2 = My402ListPrev(pList, elem2next);
    }
}

static
void BubbleSortForwardList(My402List *pList, int num_items)
{
    My402ListElem *elem=NULL;
    int i=0;

    if (My402ListLength(pList) != num_items) {
        fprintf(stderr, "List length is not %1d in BubbleSortForwardList().\n", num_items);
        exit(1);
    }
    for (i=0; i < num_items; i++) {
        int j=0, something_swapped=FALSE;
        My402ListElem *next_elem=NULL;

        for (elem=My402ListFirst(pList), j=0; j < num_items-i-1; elem=next_elem, j++) {
            time_t cur_val=((TransactionFields*)(elem->obj))->timeStamp;

            next_elem=My402ListNext(pList, elem);
            time_t next_val =((TransactionFields*)(next_elem->obj))->timeStamp;

            if(cur_val==next_val)
            {
                fprintf(stderr, "\t(Wrong file format. Two timestamps cannot be identical)\n");
                exit(1);
            }

            if (cur_val > next_val) {
                BubbleForward(pList, &elem, &next_elem);
                something_swapped = TRUE;
            }
        }
        if (!something_swapped) break;
    }
}

static
void CheckCmdline(int argc, char *argv[])
{
    if(argc==1)
    {
        fprintf(stderr,"\t(malformed commandline - incorrect number of commandline arguments)\n\t(usage: warmup1 sort [tfile])\n");
        exit(1);
    }
    else
    {
        if(strcmp(*(argv+1),"sort")!=0)
        {
            fprintf(stderr,"\t(malformed commandline -\"%s\" is not a valid commandline argument)\n\t(usage: warmup1 sort [tfile])\n",*(argv+1));
            exit(1);
        }
    }

}

static
void ExtractTheField(char* fBuffer, int bufferSize, char* startOfField,int lineNo, int fieldNo, My402List* list,TransactionFields* everyTransaction)
{
    strncpy(fBuffer, startOfField, bufferSize);
    fBuffer[bufferSize-1]='\0';
    if(strlen(fBuffer)>0)
    {
                  if(fieldNo==1)
                  {
                    int i=0;
                    for(i=0;i<strlen(fBuffer);i++)
                    {
                        if(fBuffer[i]!='+'&&fBuffer[i]!='-')
                        {
                            fprintf(stderr,"\t(Wrong file format. The transaction field in line %d should contain +/-)\n", lineNo);
                            exit(1);
                        }

                            everyTransaction->drawOrDeposit=fBuffer[i];

                    }


                  }

                  if(fieldNo==2)
                  {
                        if(strlen(fBuffer)>=11)
                        {
                            fprintf(stderr,"\t(Wrong file format. Time field in line %d cannot contain more than 11 digits)\n", lineNo);
                            exit(1);
                        }

                        time_t TransactionTime=atoi(fBuffer);
                        time_t currentTime=time(NULL);
                        if(TransactionTime<0 ||TransactionTime>currentTime)
                        {
                            fprintf(stderr,"\t(Wrong file format. Time field in line %d is corrupted)\n", lineNo);
                            exit(1);
                        }
                        everyTransaction->timeStamp=TransactionTime;


                  }
                  if(fieldNo==3)
                  {
                    int i=0;
                    int numLeft=0;
                    int numRight=0;

                    while(fBuffer[i]!='.'&&i<strlen(fBuffer))
                    {
                        numLeft++;
                        i++;
                    }
                    if(numLeft>7)
                    {
                        fprintf(stderr,"\t(Wrong file format. The number to the left of the decimal point in transaction amount in line %d can be at most 7 digits (i.e., < 10,000,000)\n", lineNo);
                        exit(1);
                    }
                    i++;
                    while(i<strlen(fBuffer))
                    {
                        numRight++;
                        i++;
                    }
                    if(numRight!=2)
                    {
                        fprintf(stderr,"\t(Wrong file format. The decimal part should be exactly two digits in line %d)\n", lineNo);
                        exit(1);

                    }
                    if(fBuffer[0]==0 &&strlen(fBuffer)>3)
                    {
                        fprintf(stderr,"\t(Wrong file format. The first digit cannot be zero in line %d\n", lineNo);
                    }

                    char* timeInString=strtok(fBuffer,".");
                    int firstPartAmount=atoi(timeInString);
                    timeInString=strtok(NULL,".");
                    int decimalPartAmount=atoi(timeInString);

                    if(firstPartAmount<0)
                    {
                        fprintf(stderr,"\t(Wrong file format. The transaction amount must have a positive value in line %d)\n", lineNo);
                        exit(1);
                    }

                    int transactionAmountInCents=(firstPartAmount*100)+decimalPartAmount;
                    everyTransaction->amountInCents=transactionAmountInCents;

                  }
                  if(fieldNo==4)
                  {
                    int i=0;
                    while(isspace(fBuffer[i]))
                    {
                        i++;
                    }
                    char* descriptionField=strdup(&fBuffer[i]);
                    everyTransaction->description=descriptionField;
                    My402ListAppend(list,everyTransaction);
                        
                  }


    }
    else
    {
                if(fieldNo==1)
                {
                    fprintf(stderr,"\t(Wrong file format. Transaction type field in Line %d is empty)\n",lineNo);
                    exit(1);
                }
                if(fieldNo==2)
                {
                        fprintf(stderr,"\t(Wrong file format. Transaction time field in Line %d is empty)\n",lineNo);
                        exit(1);
                }

                if(fieldNo==3)
                {
                            fprintf(stderr,"\t(Wrong file format. Transaction amount field in Line %d is empty)\n",lineNo);
                            exit(1);
                }
                if(fieldNo==4)
                {
                             fprintf(stderr,"\t(Wrong file format. Decription field in Line %d is empty)\n",lineNo);
                              exit(1);
                }


    }

}
static 
void SortTime(My402List* list,int num)
{
    BubbleSortForwardList(list, num);
    
}

static 
void PrintAllTransaction(My402List* list)
{
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    printf("|       Date      | Description              |         Amount |        Balance |\n"); 
    printf("+-----------------+--------------------------+----------------+----------------+\n");


    My402ListElem* elem=NULL;
    int processTransNo=0;
    int balAfterEveryTans=0;
    for(elem=My402ListFirst(list);elem!=NULL;elem=My402ListNext(list,elem))
    {
        processTransNo++;
        TransactionFields* pToTran=NULL;
        pToTran=elem->obj;
        char typeOfTransaction=pToTran->drawOrDeposit;
        time_t timeOfTrans=pToTran->timeStamp;
        int amount=pToTran->amountInCents;
        char* finalDescription=pToTran->description;

        int amountForBalCal=amount;


        //creating buffer for time
        char tbuf[26];
        char date[16];
        strncpy(tbuf,ctime(&timeOfTrans),sizeof(tbuf));

        int i=0;
        for(i=0;i<10;i++)
        {
            date[i]=tbuf[i];
        }
        for(i=10;i<=14;i++)
        {
            date[i]=tbuf[i+9];
        }
        date[15]='\0';


        //creating buffer for description
        char descBuffer[25];
        memset(descBuffer,' ',sizeof(descBuffer));
        strncpy(descBuffer,finalDescription,strlen(finalDescription));
        descBuffer[24]='\0';



        //creating buffer for the amount
        char dispAmount[15];
        memset(dispAmount,' ',sizeof(dispAmount));
        dispAmount[14]='\0';
        dispAmount[10]='.';
        if(typeOfTransaction=='-')
        {
            dispAmount[0]='(';
            dispAmount[13]=')';
        }

        dispAmount[12]=amount%10+'0';
        amount=amount/10;
        dispAmount[11]=amount%10+'0';
        amount=amount/10;
        if(amount==0)
        {
            dispAmount[9]='0';
        }
        else
        {
            i=9;
            while(amount!=0&&i>0)
            {
                dispAmount[i]=amount%10+'0';
                amount=amount/10;
                i--;
                
                if((i==6||i==2)&&amount!=0)
                {
                    dispAmount[i]=',';
                    i--;
                }
                

            }
        }
        
        int bal=0;
        char balanceAmount[15];
        memset(balanceAmount,' ',sizeof(balanceAmount));
        balanceAmount[14]='\0';
        balanceAmount[10]='.';
        bal=balAfterEveryTans;    
        if(processTransNo==1)
        {   
            if(typeOfTransaction=='-')
            {
               bal=-amountForBalCal;               
            }
            else
            {
                bal=amountForBalCal;
            }
        }
        else
        {
            if(typeOfTransaction=='+')
            {
                bal=bal+amountForBalCal;
            }
            else
            {
                bal=bal-amountForBalCal;
            }
              
        }
        balAfterEveryTans=bal;
        if(bal<0)
        {
            balanceAmount[0]='(';
            balanceAmount[13]=')';
            bal=abs(bal);
        }
        if(bal>=1000000000)
        {
            balanceAmount[11]='?';
            balanceAmount[12]='?';
            int j;
            for(j=1;j<10;j++)
            {
                balanceAmount[j]='?';
            }

        }
        balanceAmount[12]=bal%10+'0';
        bal=bal/10;
        balanceAmount[11]=bal%10+'0';
        bal=bal/10;
        if(bal==0)
        {
            balanceAmount[9]='0';
        }
        else
        {
            i=9;
            while(bal!=0&&i>0)
            {
                balanceAmount[i]=bal%10+'0';
                bal=bal/10;
                i--;
                if((i==6||i==2)&&bal!=0)
                {
                    balanceAmount[i]=',';
                    i--;
                }

            }
        }
        
        printf("| %s | %s | %s | %s |\n",date,descBuffer,dispAmount,balanceAmount);
        


    }

   printf("+-----------------+--------------------------+----------------+----------------+\n");

}


static
int ReadInput(FILE *fp, My402List* list)
{
    char buffer[2000];
    int numOfLinesRead=0;
    while(fgets(buffer,sizeof(buffer),fp)!=NULL)
    {
        numOfLinesRead++;
        if(strlen(buffer)>1024)
        {
            fprintf(stderr,"\t(Wrong file format. Line %d cannot have more than 1024 characters)\n",numOfLinesRead);
            exit(1);
        }

        char *start=buffer;
        char *end=buffer;
        int numOfTabs=0;
        while((end=strchr(start,'\t'))!=NULL)
        {
            end++;
            numOfTabs++;
            start=end;

        }

        if(numOfTabs!=3)
        {
           fprintf(stderr,"\t(Wrong file format. Line %d must contain exactly 4 fields.)\n",numOfLinesRead);
           exit(1);
        }
        else
        {
           start=buffer;
           end=buffer;
           char fieldBuffer[1024];
           int numOfField=0;
           TransactionFields* everyTransaction=(TransactionFields*)malloc(sizeof(TransactionFields));
           memset(everyTransaction,0,sizeof(TransactionFields));
           while((end=strchr(start,'\t'))!=NULL)
           {
               numOfField++;
               *end++='\0';
               ExtractTheField(fieldBuffer,sizeof(fieldBuffer), start,numOfLinesRead,numOfField, list, everyTransaction);
               start=end;

           }
           if((end=strchr(start,'\n'))!=NULL)
           {
               *end='\0';
               ExtractTheField(fieldBuffer,sizeof(fieldBuffer), start,numOfLinesRead,4, list,everyTransaction);
            }

        }



    }
    if(numOfLinesRead==0)
    {
        fprintf(stderr,"\t(Wrong file format. Input file is empty)\n");
        exit(1);

    }
    SortTime(list,numOfLinesRead);
    PrintAllTransaction(list);

    /*
    My402ListElem* elem=NULL;
    for(elem=My402ListFirst(list);elem!=NULL;elem=My402ListNext(list,elem))
    {
        
        TransactionFields* pToTran=NULL;
        pToTran=elem->obj;
        char* finalDescription=pToTran->description;
        free(finalDescription);
    }
    My402ListUnlinkAll(list);
    */
    

    return TRUE;

}






/* ----------------------- main() ----------------------- */

int main(int argc, char *argv[])
{
    CheckCmdline(argc,argv);
    FILE* fp;
    if(argc==2)
    {
        fp=stdin;
    }
    else
    {
        fp=fopen(argv[2],"r");
    }
        if(fp==NULL)
        {
            if(errno==2)
            {
                fprintf(stderr,"\t(input file \"%s\" does not exist)\n",argv[2]);
            }
            if(errno==13)
            {
                fprintf(stderr,"\t(\"%s\" cannot be opened - access denied)\n",argv[2]);
            }
            exit(1);

        }
        else
        {
            My402List list;
            memset(&list,0,sizeof(My402List));
            if(!My402ListInit(&list))
            {
                fprintf(stderr,"\tError in the list initialisation\n");
            }
            if(!ReadInput(fp,&list))
            {
                fprintf(stderr,"\tError reading the input\n");
            }
            if(fp!=stdin)
            {
                fclose(fp);
            }
        }
    return(0);
}
