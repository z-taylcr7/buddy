#include "buddy.h"
#include <stdlib.h>
#include <stdio.h>
#define NULL ((void *)0)
#define MAX_RANK 16
#define PAGESIZE 4096
void* start_of_pages;
void* end_of_pages;
page *list[MAX_RANK + 1];
page **page_set;
int total_pages, page_cnt[MAX_RANK + 1];
int max_rank;

page* page_init(int rank, void *p,int alloc){
    page *nov=(page*)malloc(sizeof(page));
    nov->rank=rank;
    nov->start=p;
    nov->allocated=alloc;
    nov->size=(PAGESIZE<<(rank-1));
    nov->parent=nov->buddy=nov->prev=nov->next=NULL;
    return nov;
}
void list_append(int rank, page* nov){
    nov->next=list[rank];
    if(list[rank])list[rank]->prev=nov;
    list[rank]=nov;
    page_cnt[rank]++;
}
page* list_pop(int rank){
    page*tmp=list[rank];
    if(tmp->next)tmp->next->prev=NULL;
    list[rank]=tmp->next;
    page_cnt[rank]--;
    return tmp;

}
void* list_delete(int rank, page*nov){
    if(list[rank]==nov){
        if(nov->next) nov->next->prev=NULL;
        list[rank]=nov->next;
    }else{
        nov->prev->next=nov->next;
        if(nov->next)nov->next->prev=nov->prev;
    }
    page_cnt[rank]--;
}


int init_page(void *p, int pgcount){
    
    start_of_pages=p;
    total_pages=pgcount;
    end_of_pages=p+pgcount*PAGESIZE;
    
    max_rank=1;
    while((1<<(max_rank-1))<pgcount)max_rank++;
    
    // printf("%d\n",max_rank);
    list_append(max_rank,page_init(max_rank,p,0));
    page_set=(page**) malloc(pgcount*sizeof(page*));
    for(int i=0;i<pgcount;i++){
        page_set[i]=NULL;
    }
    return OK;
}

void *alloc_pages(int rank){
    int upper_rank=rank;
    while(page_cnt[upper_rank]==0 && upper_rank<=max_rank)upper_rank++;
    if(upper_rank>max_rank)return -ENOSPC;
     
    
    // printf("%d\n",upper_rank);
    page* target=list_pop(upper_rank);
    target->allocated=1;
    
    while(upper_rank>rank){
        page* bud1=page_init(upper_rank-1,target->start,1);
        page* bud2=page_init(upper_rank-1,target->start+(bud1->size),0);
        bud1->buddy=bud2;
        bud2->buddy=bud1;
        bud1->parent=bud2->parent=target;
        list_append(upper_rank-1,bud2);
        target=bud1;
        upper_rank--;
    }
    page_set[(target->start-start_of_pages)/PAGESIZE]=target;
    return target->start;
}

int return_pages(void *p){
    //todo:
    if(p<start_of_pages||p>end_of_pages)return -EINVAL;
    page* target=page_set[(p-start_of_pages)/PAGESIZE];
    if(target!=NULL)page_set[(p-start_of_pages)/PAGESIZE]=NULL;
    else return -EINVAL;
    
    while(target->rank<max_rank){
        if(target->buddy->allocated==1)break;
        // printf("delete here\n");
        list_delete(target->rank,target->buddy);
        
        page* tmp=target->parent;
        free(target->buddy);
        free(target);

        target=tmp;

    }
    target->allocated=0;
    list_append(target->rank,target);
    return OK;
}

int query_ranks(void *p){
    if(p<start_of_pages||p>end_of_pages)return -EINVAL;
    page* x=page_set[(p-start_of_pages)/PAGESIZE];
    if(x!=NULL)return x->rank;
    for(int i=max_rank;i>=1;i--){
        page* cur=list[i];
        while(cur!=NULL){
            if((cur->start)<=p && p<cur->start+cur->size){
                return i;
            }
            cur=cur->next;
        }
    }
    return -EINVAL;
}
int query_page_counts(int rank){
    if(rank<=0||rank>max_rank)return -EINVAL;
    return page_cnt[rank];
}