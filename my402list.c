#include"my402list.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<locale.h>

int My402ListLength ( My402List *a )
{
	return a->num_members;
}

int My402ListEmpty ( My402List *a )
{
	if ( a->num_members == 0 )
	{
		return 1;
	}
	else 
		return 0;
}

int My402ListAppend ( My402List *a, void *obj )
{
	My402ListElem *new = ( My402ListElem* )malloc(sizeof(My402ListElem));
	if ( new == NULL )
	{
		return 0;
	}
	new->obj = obj;
	if ( My402ListEmpty(a) )
	{
		(a->anchor).next = new;
		(a->anchor).prev = new;
		new->next = &a->anchor;
		new->prev = &a->anchor;
		a->num_members++;
		return 1;
	}
	else
	{
		My402ListLast(a)->next = new;
		new->next = &a->anchor;
		new->prev = My402ListLast(a);
		(a->anchor).prev = new;
		a->num_members++;
		return 1;
	}
}

int My402ListPrepend ( My402List *a, void *obj )
{
	My402ListElem *new = ( My402ListElem* )malloc(sizeof(My402ListElem));
	if ( new == NULL )
	{
		return 0;
	}
	new->obj = obj;
	if ( My402ListEmpty(a) )
	{
		a->anchor.next = new;
		a->anchor.prev = new;
		new->next = &a->anchor;
		new->prev = &a->anchor;
		a->num_members++;
		return 1;
	}
	else
	{
		My402ListFirst(a)->prev = new;
		new->next = My402ListFirst(a);
		new->prev = &a->anchor;
		(a->anchor).next = new;
		a->num_members++;
		return 1;
	}
	
}

void My402ListUnlink ( My402List *a, My402ListElem *b )
{
	(b->next)->prev = b->prev;
	(b->prev)->next = b->next; 
	b->next = b->prev = '\0';
	
	if( --a->num_members == 0 )
	{
		(a->anchor).next = (a->anchor).prev = NULL;
	}
}

void My402ListUnlinkAll( My402List *a )
{
	while(!My402ListEmpty(a))
	{
		My402ListUnlink( a, My402ListFirst(a) );
	}
	(a->anchor).next = (a->anchor).prev = NULL;
}

int My402ListInsertBefore ( My402List *a, void *obj, My402ListElem *b )
{
	My402ListElem *new = ( My402ListElem* )malloc(sizeof(My402ListElem));
	if ( new == NULL )
	{
		return 0;
	}
	new->obj = obj;
	new->next = b;
	new->prev = b->prev;
	(b->prev)->next = new;
	b->prev = new;
	a->num_members++;
	return 1;
}

int My402ListInsertAfter ( My402List *a, void *obj, My402ListElem *b )
{
	My402ListElem *new = ( My402ListElem* )malloc(sizeof(My402ListElem));
	if ( new == NULL )
	{
		return 0;
	}
	new->obj = obj;
	new->next = b->next;
	new->prev = b;
	(b->next)->prev = new;
	b->next = new;
	a->num_members++;
	return 1;
}

My402ListElem* My402ListFirst( My402List *a )
{
	if( a->num_members != 0 )
		return (a->anchor).next;
	else 
		return NULL;
}

My402ListElem* My402ListLast( My402List *a )
{
	if( a->num_members != 0 )
		return (a->anchor).prev;
	else 
		return NULL;
}

My402ListElem* My402ListNext( My402List *a, My402ListElem *b )
{
	if( My402ListLast(a) == b )
		return NULL;
	else
		return b->next;
}

My402ListElem* My402ListPrev( My402List *a, My402ListElem *b )
{
	if( My402ListFirst(a) == b )
		return NULL;
	else
		return b->prev;
}

My402ListElem* My402ListFind(My402List *a, void *obj)
{
	My402ListElem *cur = NULL;
	for ( cur=My402ListFirst(a); cur != NULL; cur=My402ListNext(a, cur))
	{
		if ( cur->obj == obj )
		{
			return cur;
		}
	}
	return NULL; 
}

int My402ListInit( My402List *a )
{
	if( a == NULL )return 0;
	(a->anchor).next = NULL;
	(a->anchor).prev = NULL;
	(a->anchor).obj = NULL;
	a->num_members = 0;
	return 1;
}


