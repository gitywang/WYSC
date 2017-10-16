#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define	WORDSIZE		256
#define handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

struct tree
{
	char word[WORDSIZE];
	int count;
	int height;
	struct tree *left;
	struct tree *right;
};

int max(int a, int b)
{
	if(a > b)
		return a;
	return b;
}

int height(struct tree *root)
{
	if(root == NULL)
		return -1;
	return root->height;
}

struct tree *single_rotate_with_left(struct tree *p2)
{
	struct tree *p1 = NULL;

	p1 = p2->left;
	p2->left = p1->right;
	p1->right = p2;

	p2->height = max(height(p2->left), height(p2->right)) + 1;
	p1->height = max(height(p1->left), height(p1->right)) + 1;

	return p1;
}

struct tree *single_rotate_with_right(struct tree *p2)
{
	struct tree *p1 = NULL;

	p1 = p2->right;
	p2->right = p1->left;
	p1->left = p2;

	p2->height = max(height(p2->left), height(p2->right)) + 1;
	p1->height = max(height(p1->left), height(p1->right)) + 1;

	return p1;
}

struct tree *double_rotate_with_left(struct tree *p)
{
	p->left = single_rotate_with_right(p->left);
	return single_rotate_with_left(p);
}

struct tree *double_rotate_with_right(struct tree *p)
{
	p->right = single_rotate_with_left(p->right);
	return single_rotate_with_right(p);
}

struct tree *insert_tree(struct tree *root, const char *word)
{
	int result;

	if(root == NULL)
	{
		root = (struct tree *)malloc(sizeof(struct tree));
		if(root == NULL)
			handle_error("insert_tree()->malloc");
		strcpy(root->word, word);
		root->count = 1;
		root->height = 0;
		root->left = NULL;
		root->right = NULL;
	}
	else if((result = strcmp(word, root->word)) == 0)
		root->count++;
	else if(result < 0)
	{
		root->left = insert_tree(root->left, word);
		if(height(root->left) - height(root->right) == 2)
		{
			if(strcmp(word, root->left->word) < 0)
				root = single_rotate_with_left(root);
			else
				root = double_rotate_with_left(root);
		}
	}
	else
	{
		root->right = insert_tree(root->right, word);
		if(height(root->right) - height(root->left) == 2)
		{
			if(strcmp(word, root->right->word) > 0)
				root = single_rotate_with_right(root);
			else
				root = double_rotate_with_right(root);
		}
	}

	root->height = max(height(root->left), height(root->right)) + 1;
	
	return root;
}

void travel_tree(struct tree *root)
{
	if(root != NULL)
	{
		travel_tree(root->left);
		printf("%s\t%d\n", root->word, root->count);
		travel_tree(root->right);
	}
}

void draw_tree(struct tree *root, int level)
{
	int i;
	if(root == NULL)
		return;
	draw_tree(root->right, level+1);
	for(i = 0; i < level; i++)
		printf("    ");
	printf("%s %d\n",root->word, root->count);
	draw_tree(root->left, level+1);
}

void destroy_tree(struct tree *root)
{
	if(root == NULL)
		return ;
	destroy_tree(root->left);
	destroy_tree(root->right);
	free(root);
}

int main(int argc, char *argv[])
{
	FILE *stream = NULL;
	char word[WORDSIZE];
	struct tree *root = NULL;

	if(argc < 2)
	{
		fprintf(stderr, "usgae: ./a.out <filename>\n");
		exit(EXIT_FAILURE);
	}

	stream = fopen(argv[1], "r");
	if(stream == NULL)
		handle_error("fopen()");

	while(fscanf(stream, "%s", word) != EOF)
		root = insert_tree(root, word);

	travel_tree(root);

	draw_tree(root, 0);

	destroy_tree(root);

	fclose(stream);

	return 0;
}
