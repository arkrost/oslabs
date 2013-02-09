#include "rope.h"

static void node_delete(struct rope_node *node)
{
	if (node->type == ROPE_CONCAT) {
		struct rope_concat *concat = (struct rope_concat *) node;
		if (concat->left) node_delete(concat->left);
		if (concat->right) node_delete(concat->right);
	}
	free(node);
}

static inline size_t node_size(struct rope_node *node)
{
	size_t size = 0;
	while (node && node->type != ROPE_LEAF) {
		struct rope_concat *concat = (struct rope_concat *) node;
		size += concat->weight;
		node = concat->right;
	}
	if (node) {
		struct rope_leaf *leaf = (struct rope_leaf *) node;
		size += leaf->size;
	}
	return size;
}

static inline size_t node_depth(struct rope_node *node)
{
	if (node->type != ROPE_LEAF) {
		struct rope_concat *concat = (struct rope_concat *) node;
		return concat->depth;
	} else {
		return 0;
	}
}

static struct rope_node *node_leaf(const char *data, size_t size)
{
	struct rope_leaf *leaf = malloc(sizeof(struct rope_leaf));
	memset(leaf, 0, sizeof(struct rope_leaf));
	leaf->base.type = ROPE_LEAF;
	leaf->size = size;
	leaf->data = data;
	return (struct rope_node *) leaf;
}

static struct rope_node *node_concat(struct rope_node *left, struct rope_node *right)
{
	if (!left || !right) {
		if (left)
			return left;
		else
			return right;
	}
	struct rope_concat *concat = malloc(sizeof(struct rope_concat));
	memset(concat, 0, sizeof(struct rope_concat));
	concat->base.type = ROPE_CONCAT;
	concat->depth = MAX(node_depth(left), node_depth(right)) + 1;
	concat->weight = node_size(left);
	concat->left = left;
	concat->right = right;
	return (struct rope_node *) concat;
}

static struct rope_node * node_split(struct rope_node *tree, size_t tree_size, size_t size)
{
	if (size >= tree_size) return NULL;

	struct rope_node *curr = tree;
	struct rope_node *tail = NULL;
	size_t curr_size = tree_size;
	size_t curr_trim = curr_size - size;
	while (curr_trim > 0 && curr->type != ROPE_LEAF) {
		struct rope_concat *concat = (struct rope_concat *)curr;
		if (curr_size - concat->weight <= curr_trim) {
			tail = node_concat(concat->right, tail);
			curr_trim = curr_trim  + concat->weight - curr_size;
			concat->right = NULL;
			concat->depth = node_depth(concat->left) + 1;
			curr_size = concat->weight;
			concat->weight -= curr_trim;
			curr = concat->left;
		} else {
			curr_size = curr_size - concat->weight;
			curr = concat->right;
		}
	}
	if (curr_trim) {
		struct rope_leaf *leaf = (struct rope_leaf *) curr;
		leaf->size -= curr_trim;
		struct rope_node *r = node_leaf(leaf->data + leaf->size, curr_trim);
		tail = node_concat(r, tail);
	}
	return tail;
}

static char* node_dump_reversed(struct rope_node *node, char* buf) 
{
	if (node->type == ROPE_CONCAT) {
		struct rope_concat *concat = (struct rope_concat *) node;
		buf = node_dump_reversed(concat->right, buf);
		return node_dump_reversed(concat->left, buf);
	} else {
		struct rope_leaf *leaf = (struct rope_leaf *) node;
		int i;
		for(i = leaf->size - 1; i >=0; --i, ++buf) *buf = leaf->data[i];
		return buf;
	}	
}


/*---------------------------------------------------------------------------*/
struct rope * rope_new(const char *data, size_t size)
{
	struct rope *rope = malloc(sizeof(struct rope));
	rope->root = node_leaf(data, size);
	rope->size = size;
	return rope;
}

void rope_delete(struct rope *rope)
{
	if (rope->root) node_delete(rope->root);
	free(rope);
}

void rope_append(struct rope *rope, const char *data, size_t size)
{
	struct rope_node *tail = node_leaf(data, size);
	rope->root = node_concat(rope->root, tail);
	rope->size += size;
}

struct rope* rope_split(struct rope *rope, size_t size)
{
	size = MIN(size, rope->size);
	struct rope_node *tail = node_split(rope->root,	rope->size,	size);
	struct rope* res = malloc(sizeof(struct rope));
	res->root = tail;
	res->size = rope->size - size;
	rope->size = size;
	return res;
}

void rope_dump_reversed(struct rope *rope, char* storage) 
{
	node_dump_reversed(rope->root, storage);
}