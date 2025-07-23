/*
 * Author:      William Chia-Wei Cheng (bill.cheng@usc.edu)
 *
 * @(#)$Id: my402list.h,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cs402.h"

#include "my402list.h"



int  My402ListLength(My402List* listPtr)
{
    return listPtr->num_members;

}


int  My402ListEmpty(My402List* listPtr)
{
    return (listPtr->num_members)<=0;

}


int  My402ListAppend(My402List* listPtr, void* objPtr)
{

    My402ListElem* lastElem=(My402ListElem*)malloc(sizeof(My402ListElem));
    lastElem->obj=objPtr;
    My402ListElem* prevLast=My402ListLast(listPtr);




    if(My402ListEmpty(listPtr))
    {
        (listPtr->anchor).next=lastElem;
        lastElem->prev=&(listPtr->anchor);
        (listPtr->num_members)=1;
    }
    else
    {
         prevLast->next=lastElem;
         lastElem->prev=prevLast;
        (listPtr->num_members)++;
    }
    (listPtr->anchor).prev=lastElem;
    lastElem->next=&(listPtr->anchor);

    return TRUE;

}

int  My402ListPrepend(My402List* listPtr, void* objPtr)
{
    My402ListElem* firstElem=(My402ListElem*)malloc(sizeof(My402ListElem));
    firstElem->obj=objPtr;
    My402ListElem* prevFirst=My402ListFirst(listPtr);

    if(My402ListEmpty(listPtr))
    {
        (listPtr->anchor).prev=firstElem;
        firstElem->next=&(listPtr->anchor);
        (listPtr->num_members)=1;
    }
    else
    {
        prevFirst->prev=firstElem;
        firstElem->next=prevFirst;
        (listPtr->num_members)++;
    }
    (listPtr->anchor).next=firstElem;
    firstElem->prev=&(listPtr->anchor);

    return TRUE;

}
void My402ListUnlink(My402List* listPtr, My402ListElem* listElemPtr)
{
    listElemPtr->next->prev=listElemPtr->prev;
    listElemPtr->prev->next=listElemPtr->next;
    free(listElemPtr);
    (listPtr->num_members)--;
}


void My402ListUnlinkAll(My402List* listPtr)
{
    My402ListElem* listElemPtr=NULL;
    My402ListElem* nextElemPtr=NULL;
    for(listElemPtr=My402ListFirst(listPtr);listElemPtr!=NULL;listElemPtr=nextElemPtr)
    {
        nextElemPtr=My402ListNext(listPtr,listElemPtr);
        free(listElemPtr);
    }
    listPtr->num_members=0;
    (listPtr->anchor).next=&(listPtr->anchor);
    (listPtr->anchor).prev=&(listPtr->anchor);

}
int  My402ListInsertAfter(My402List* listPtr, void* objPtr, My402ListElem* listElemPtr)
{
    if(listElemPtr==NULL)
    {
        My402ListAppend(listPtr, objPtr);
    }
    My402ListElem* elemToInsert=(My402ListElem*)malloc(sizeof(My402ListElem));
    elemToInsert->obj=objPtr;
    elemToInsert->next=listElemPtr->next;
    elemToInsert->prev=listElemPtr;

    listElemPtr->next=elemToInsert;
    elemToInsert->next->prev=elemToInsert;

    listPtr->num_members++;


    return TRUE;

}
int  My402ListInsertBefore(My402List* listPtr, void* objPtr, My402ListElem* listElemPtr)
{
    if(listElemPtr==NULL)
    {
        My402ListPrepend(listPtr, objPtr);
    }
    My402ListElem* elemToInsert=(My402ListElem*)malloc(sizeof(My402ListElem));
    elemToInsert->obj=objPtr;
    elemToInsert->next=listElemPtr;
    elemToInsert->prev=listElemPtr->prev;

    listElemPtr->prev=elemToInsert;
    elemToInsert->prev->next=elemToInsert;

    listPtr->num_members++;



    return TRUE;

}

My402ListElem *My402ListFirst(My402List* listPtr)
{
    if(My402ListEmpty(listPtr))
    {
        return NULL;
    }

    return (listPtr->anchor).next;



}

My402ListElem *My402ListLast(My402List* listPtr)
{
    if(My402ListEmpty(listPtr))
    {
        return NULL;
    }
    return (listPtr->anchor).prev;

}

My402ListElem *My402ListNext(My402List* listPtr, My402ListElem* listElemPtr)
{
    if(listElemPtr->next==&(listPtr->anchor))
    {
        return NULL;
    }
    return listElemPtr->next;

}

My402ListElem *My402ListPrev(My402List* listPtr, My402ListElem* listElemPtr)
{
    if(listElemPtr->prev==&(listPtr->anchor))
    {
        return NULL;  
    }
    return listElemPtr->prev;
}

My402ListElem *My402ListFind(My402List* listPtr, void* objPtr)
{
    My402ListElem* findElem=My402ListFirst(listPtr);
        int found=FALSE;
        while(findElem!=NULL&&!found)
        {
            if(findElem->obj==objPtr)
            {
                found=TRUE;
                return findElem;
            }
            else
            {
                findElem=My402ListNext(listPtr, findElem);
            }
            


        }
    return NULL;

}


int My402ListInit(My402List* listPtr)
{
    listPtr->num_members=0;
    (listPtr->anchor).next=&(listPtr->anchor);
    (listPtr->anchor).prev=&(listPtr->anchor);
    (listPtr->anchor).obj=NULL;
    return TRUE;

}

