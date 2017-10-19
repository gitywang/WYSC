#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define WORDSIZE		256
#define	handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

struct list_st
{
	char word[WORDSIZE];
	struct list_st *next;
};

struct list_st *head = NULL;

void init_list()
{
	head = (struct list_st*)malloc(sizeof(struct list_st));
	if(head == NULL)
		handle_error("malloc()->head");
	
	memset(head->word, '\0', WORDSIZE);
	head->next = head;
}

void create_list(FILE *stream)
{
	if(head == NULL) return;

	char word[WORDSIZE] = {'\0'};
	struct list_st *new = NULL;
	struct list_st *p = head;

	while(fscanf(stream, "%s", word) != EOF)
	{
		new = (struct list_st *)malloc(sizeof(struct list_st));
		if(new == NULL)
			handle_error("malloc()->new");

		strcpy(new->word, word);

		new->next = p->next;
		p->next = new;
		p = new;/* Back insert,front insert if without this line */
	}
}

void travel_list()
{
	if(head == NULL) return;

	struct list_st *p = head->next;
	printf("\n");
	while(p != head)
	{
		printf("%s\n",p->word);
		p = p->next;
	}
	printf("\n");
}


void insert_list()
{
	if(head == NULL) return;

	char insert_word[WORDSIZE] = {'\0'};;
	char insert_point[WORDSIZE] = {'\0'};
	struct list_st *p = head->next;

	printf("Please input insert point\n");
	scanf("%s", insert_point);
	printf("Please input insert word\n");
	scanf("%s", insert_word);

	while(p != head)
	{
		if(strcmp(insert_point, p->word) == 0)	
		{
			struct list_st *new = (struct list_st *)malloc(sizeof(struct list_st));
			if(new == NULL)
				handle_error("insert_list()->malloc->new");
			
			strcpy(new->word, insert_word);

			new->next = p->next;
			p->next = new;
			return;
		}
		p = p->next;
	}
	printf("Element %s not found\n\n", insert_point);
}

/* Delete node should remember deleted node's prev node */
void delete_list()
{
	if(head == NULL) return;

	char delete_word[WORDSIZE] = {'\0'};;
	struct list_st *p1 = head->next;
	struct list_st *p2 = head;

	printf("Please input delete word:\n");
	scanf("%s", delete_word);

	while(p1 != head)
	{
		if(strcmp(p1->word, delete_word) == 0)
		{
			p2->next = p1->next;
			free(p1);
			return;
		}
		p2 = p1;
		p1 = p1->next;
	}
	printf("Element %s not found\n\n", delete_word);
}

void modify_list()
{
	if(head == NULL) return;

	char modify_word[WORDSIZE] = {'\0'};
	char modify_point[WORDSIZE] = {'\0'};
	struct list_st *p = head->next;

	printf("Please input modify point\n");
	scanf("%s", modify_point);
	printf("Please input modify word:\n");
	scanf("%s", modify_word);

	while(p != head)
	{
		if(strcmp(modify_point, p->word) == 0)
		{
			strcpy(p->word, modify_word);
			printf("Modify %s succeed!\n\n", modify_word);
			return;
		}
		p = p->next;
	}
	printf("Element %s not found\n\n", modify_point);
}

void search_list()
{
	if(head == NULL) return;

	char search_word[WORDSIZE] = {'\0'};
	struct list_st *p = head->next;

	printf("Please input search word:\n");
	scanf("%s", search_word);

	while(p != head)
	{
		if(strcmp(search_word, p->word) == 0)
		{
			printf("Search %s succeed!\n\n", search_word);
			return;
		}
		p = p->next;
	}
	printf("Element %s not found\n\n", search_word);
}

/* Destroy node should remember freeded node's next node */
void destroy_list()
{
	if(head == NULL) return;
	
	struct list_st *p1 = head->next;
	struct list_st *p2 = NULL;
	
	while(p1 != head)
	{
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
	free(head);/* Last step free head */
	printf("destroy list succeed!\n");
}

int main(int argc, char *argv[])
{
	FILE *stream = NULL;

	if(argc < 2)
	{
		fprintf(stderr, "usage:./a.out <filename>\n");
		exit(EXIT_FAILURE);
	}

	stream = fopen(argv[1], "r");
	if(stream == NULL)
		handle_error("fopen()");

	/* init */
	init_list();

	/* create */
	create_list(stream);

	/* travel */
	travel_list();

	/* insert */
	insert_list();
	travel_list();

	/* delete */
	delete_list();
	travel_list();

	/* modify */
	modify_list();
	travel_list();

	/* search */
	search_list();

	/* destory */
	destroy_list();

	fclose(stream);

	return 0;
}

