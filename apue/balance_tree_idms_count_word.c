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
		printf("%s\t%d\t%d\n", root->word, root->count, root->height);
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
	printf("%s %d %d\n",root->word, root->count, root->height);
	draw_tree(root->left, level+1);
}

void destroy_tree(struct tree *root)
{
	if(root == NULL)
		return;
	destroy_tree(root->left);
	destroy_tree(root->right);
	free(root);
}

struct tree *findmin(struct tree *root)
{
	if(root == NULL)
		return NULL;
	else if(root->left == NULL)
		return root;
	else
		return root->left;
}

struct tree *get_parent(struct tree *root, const char *word)
{
	if((root == NULL) || (strcmp(word, root->word) == 0))
		return NULL;
	if((root->left && (strcmp(word, root->left->word) == 0)) || (root->right && (strcmp(root->right->word, word) == 0)))
		return root;
	else if((root->left) && (strcmp(word, root->left->word) < 0))
		return get_parent(root->left, word);
	else
		return get_parent(root->right, word);
}



struct tree *delete_tree(struct tree *root, const char *word)
{
	int result;
	if(word == NULL)
		return NULL;
	
	if(root == NULL)
	{
		printf("Element not found!\n");
		return NULL;
	}
	
	if((result = strcmp(word, root->word)) == 0)/* Found element to be deleted */
	{
		struct tree *min = findmin(root->right);/* Find smallest element on deleted element's right subtree */
		if(min != NULL)/* Deleted element's right subtree exist */
		{
			strcpy(root->word, min->word);/* Smallest element's data copy to deleted element's data */
			if(min != root->right)/* Smallest element is not deleted element's right subtree */
			{
				struct tree *parent = get_parent(root->right, min->word);
				parent->left = min->right;
			}
			else/* Smallest element is deleted element's right subtree */
			{
				root->right = min->right;
			}
		}
		else/* Deleted element's right subtree not exist */
		{
			min = root;
			root = root->left;
		}
		free(min);
	
	}
	else if(result < 0)
	{
		root->left = delete_tree(root->left, word);/* Go left */
		if(height(root->right) - height(root->left) == 2)
		{
			if(root->right)
			{
			//	if(height(root->right->left) > height(root->right->right))
			//		root = double_rotate_with_left(root);
			//	else
			//		root = single_rotate_with_left(root);
				if(height(root->right->right) > height(root->right->left))
					root = single_rotate_with_left(root);
				else
					root = double_rotate_with_left(root);
			}
		}
	}
	else
	{
		root->right = delete_tree(root->right, word);/* Go right */
		if(height(root->left) - height(root->right) == 2)
		{
			if(root->left)
			{
			//	if(height(root->left->right) > height(root->left->left))
			//		root = double_rotate_with_right(root);
			//	else
			//		root = single_rotate_with_right(root);
			
				if(height(root->left->left) < height(root->left->right))
					root = single_rotate_with_right(root);
				else
					root = double_rotate_with_right(root);
			}
		}
	}

	if(root != NULL)
		root->height = max(height(root->left), height(root->right)) + 1;

	return root;
}


void search_tree(struct tree *root, const char *word)
{
	int result;
	
	if(word == NULL)
		return;
	if(root == NULL)
	{
		printf("Element not found!\n");
		return;
	}
	
	if((result = strcmp(word, root->word)) == 0)
		printf("search %s succeed!\n%s\t%d\n", word, root->word, root->count);
	else if(result < 0)
		search_tree(root->left, word);
	else
		search_tree(root->right, word);

}

int main(int argc, char *argv[])
{
	FILE *stream = NULL;
	char word[WORDSIZE];
	char insert_word[WORDSIZE] = {'\0'};
	char search_word[WORDSIZE] = {'\0'};
	char delete_word[WORDSIZE] = {'\0'};
	struct tree *root = NULL;

	if(argc < 2)
	{
		fprintf(stderr, "usgae: ./a.out <filename>\n");
		exit(EXIT_FAILURE);
	}

	stream = fopen(argv[1], "r");
	if(stream == NULL)
		handle_error("fopen()");
	
	/* create */
	while(fscanf(stream, "%s", word) != EOF)
		root = insert_tree(root, word);

	/* travel */
	travel_tree(root);

	/* draw */
	draw_tree(root, 0);

	/* insert */
	printf("Please input insert_word:\n");
	scanf("%s", insert_word);
	root = insert_tree(root, insert_word);
	
	draw_tree(root, 0);

	/* delete */
	printf("Please input delete_word:\n");
	scanf("%s", delete_word);
	root = delete_tree(root, delete_word);
	
	draw_tree(root, 0);
	travel_tree(root);


	/* search */
	printf("Please input search_word:\n");
	scanf("%s", search_word);
	search_tree(root, search_word);
	
	/* destory */
	destroy_tree(root);

	fclose(stream);

	return 0;
}
