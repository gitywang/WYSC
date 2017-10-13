#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define handle_error(msg)	do{perror(msg);exit(EXIT_FAILURE);}while(0)
#define MAXSIZE				1024
#define MAX_COMPARE_LINE	100

struct list
{
	char *addr_line;
	int count_char;
	char *addr_tree;
	struct list *prev;
	struct list *next;
};

struct tree
{
	char text[MAXSIZE];
	int height;
	struct tree *left;
	struct tree *right;
};


struct list *init_list()
{
	struct list *head;

	head = (struct list *)malloc(sizeof(struct list));
	if(head == NULL)
		handle_error("init_list()->malloc");
	
	head->addr_line = NULL;
	head->count_char = 0;
	head->addr_tree = NULL;
	head->prev = head;
	head->next = head;

	return head;
}

int height(struct tree *root)
{
	if(root == NULL)
		return -1;
	else 
		return root->height;
}

int max(int a, int b)
{
	if(a >= b)
		return a;
	return b;
}

struct tree *singlerotatewithleft(struct tree *p2)
{
	struct tree *p1 = p2->left;
	p2->left = p1->right;
	p1->right = p2;

	p2->height = max(height(p2->left), height(p2->right)) + 1;
	p1->height = max(height(p1->left), height(p1->right)) + 1;

	return p1;
}

struct tree *singlerotatewithright(struct tree *p2)
{
	struct tree *p1 = p2->right;
	p2->right = p1->left;
	p1->left = p2;

	p2->height = max(height(p2->left), height(p2->right)) + 1;
	p1->height = max(height(p1->left), height(p1->right)) + 1;

	return p1;
}


struct tree *doublerotatewithleft(struct tree *p)
{
	p->left = singlerotatewithright(p->left);
	return singlerotatewithleft(p);
}

struct tree *doublerotatewithright(struct tree *p)
{
	p->right = singlerotatewithleft(p->right);
	return singlerotatewithright(p);
}

struct tree *insert_tree(struct tree *root, char *line_start, int count, struct list *last)
{
	int result;
	char string[count+1];
	strncpy(string, line_start, count);
	string[count] = '\0';

	//printf("string = %s\troot->text = %s\n", string, root->text);
	if(root == NULL)
	{
		root = (struct tree *)malloc(sizeof(struct tree));
		if(root == NULL)
			handle_error("insert_tree()->malloc root");

		strcpy(root->text, string);
		//printf("root->text = %s\n", root->text);
		root->height = 0;
		root->left = NULL;
		root->right = NULL;
		last->addr_tree = root;
	}
	else if((result = strcmp(string, root->text)) == 0)
	{
		last->addr_tree = root;//node's address at structure of tree
	}
	else if(result < 0)
	{
		root->left = insert_tree(root->left, line_start, count, last);
		if(height(root->left) - height(root->right) == 2)
		{
			if(strcmp(string, root->left->text) < 0)
				root = singlerotatewithleft(root);
			else
				root = doublerotatewithleft(root);
		}
	}
	else
	{
		root->right = insert_tree(root->right, line_start, count, last);
		if(height(root->right) - height(root->left) == 2)
		{
			if(strcmp(string, root->right->text) > 0)
				root = singlerotatewithright(root);
			else
				root = doublerotatewithright(root);
		}
	}

	root->height = max(height(root->left) , height(root->right)) + 1;

	return root;
}

void print_cur_line(char *addr, int count)
{
	char p[count+1];
	strncpy(p, addr, count);
	p[count] = '\0';
	
	//printf("------->> %s\n", p);;
}

void insert_list(struct list *head, char *line_start, int count)
{
	struct list *new;
	struct list *p = NULL;
	
	new = (struct list *)malloc(sizeof(struct list));
	if(new == NULL)
		handle_error("insert_list()->malloc");

	new->addr_line = line_start;
	new->count_char = count;
	
	new->next = head;
	new->prev = head->prev;
	new->prev->next = new;
	new->next->prev = new;

}


int open_argv(char argv[])
{
	int fd;

	fd = open(argv, O_RDONLY);
	if(fd == -1)
		handle_error("open_argv()->open");
	return fd;
}

void print_list(struct list *head)
{
	if(!head) return;

	struct list *p = head->next;

	while(p != head)
	{
		printf("%p\n", p->addr_line);
	//	printf("%d\n", p->count_char);
	//	printf("%p\n", p->addr_tree);
		p = p->next;
	}
}

void draw_tree(struct tree *root, int level)
{
	int i;

	if(root == NULL)
		return ;
	draw_tree(root->right, level+1);
	for(i = 0;i < level; i++)
		printf("	");
	printf("%s\n", root->text);
	draw_tree(root->left, level+1);

}



struct list *find_start_line(struct list *head, struct list *current_line, int number)
{
	int var = 1;
	struct list *prev_start = current_line;

	for(var == 1; var <= number; var++)
	{
		prev_start = prev_start->prev;
		if(prev_start == head)
		{
			//printf("prev_start == head\n");
			break;
		}
	}
	return prev_start;
}

/*
 * prev_start = find_start_line();
 * next_start = current_line;
 *
 * prev_start == head;
 *		return 0;
 * prev and next compare number counts
 *		if(same) return 1;
 *		else	 return 2;
 * 
 * if return 0 or return 2,continue next line
 *
 * */
int compare_prev_next(struct list *head, struct list *current_line, int number)
{
	int n = 1;
	int same_count = 0;


	struct list *prev_start = find_start_line(head, current_line, number);
	//printf("------prev_start = %p\n", prev_start->addr_line);
	struct list *next_start = current_line;

	if(prev_start == head)
		return 0;// prev_start line not found

	for(n = 1; n <= number; n++)
	{
		if(prev_start->addr_tree == next_start->addr_tree)
		{
			same_count++;
			prev_start = prev_start->next;
			next_start = next_start->next;
			if(next_start == head)
				break;
		}
		else
			break;
	}
	if(same_count == number)
		return 1;
	else 
		return 2;//compare compeleted, but not same
}


void delete_line_n(struct list *current_line, int number)
{
	int var = 1;
	struct list *p1 = current_line->prev;
	struct list *p2 = NULL;
	
	if(!current_line) return;
	if(number < 1)	return;
	
	for(var = 1; var <= number; var++)
	{
		p2 = p1->prev;
		p1->prev->next = p1->next;
		p1->next->prev = p1->prev;
		//fprintf(stderr, "----------->>> free %p\n", p1->addr_line);
		free(p1);
		p1 = p2;
	}

}

void compare_line_n(struct list *head)
{
	struct list *current_line = NULL;
	int number;
	int result = -1;

	for(number = 1; number <= MAX_COMPARE_LINE; number++)
	{
		current_line = head;
		while(current_line->next != head)
		{
			current_line = current_line->next;
			//printf("current_line = %p\tnumber = %d\n", current_line->addr_line, number);
			if((result = compare_prev_next(head, current_line, number)) == 1) // same
				delete_line_n(current_line , number);
			else // not same or prev start line not found
				continue;
		}
	}
}


void travel_new_list(struct list *head, char file[])
{
	char new_file[512];
	int fd;
	struct list *p = head->next;
	
	sprintf(new_file, "%s__new", file);
	fd = open(new_file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if(fd == -1)
		handle_error("travel_new_list()->open");

	while(p != head)
	{
		write(fd, p->addr_line, p->count_char);
		write(fd, "\n", 1);
		p = p->next;
	}
}

void destroy_list(struct list *head)
{
	if(!head) return;
	
	struct list *p1 = head->next;
	struct list *p2 = NULL;

	while(p1 != head)
	{
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
	free(head);
}


void destroy_tree(struct tree *root)
{
	if(root == NULL)
		return;
	destroy_tree(root->left);
	destroy_tree(root->right);
	free(root);
}

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(access(argv[1], F_OK) != 0)
	{
		fprintf(stderr, "%s not exist\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	int fd = 0;
	struct stat statres;
	char *addr;

	fd = open_argv(argv[1]);
	
	if(stat(argv[1], &statres) == -1)
		handle_error("main()->stat");

	addr = mmap(NULL, statres.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(addr == MAP_FAILED)
		handle_error("main()->mmap");

	printf("=======-------------->>> addr = %p\n", addr);
	close(fd);

	//puts(addr);
	
	int step = 0;
	char *line_start = addr;
	int count = 1;
	struct list *head = init_list();
	struct tree *root = NULL;
	
	while(1)
	{	
		addr++;
		step++;
		if(step == statres.st_size)
			break;

		if(*addr != '\n')
			count++;
		else			
		{
			insert_list(head, line_start, count);
			root = insert_tree(root, line_start, count, head->prev);
			//print_cur_line(line_start, count);
			if(*(addr+1))
			{
				line_start = addr+1;
				count = 0;
			}
		}
	}

//	print_list(head);
//	draw_tree(root, 0);

	compare_line_n(head);

	travel_new_list(head, argv[1]);

	destroy_tree(root);

	destroy_list(head);

	munmap(addr, statres.st_size);
	
	return 0;
}

