#if !defined(CROPE_H)
#define ROPE_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MAX */

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

enum rope_node_type 
{
	ROPE_LEAF = 0,
	ROPE_CONCAT = 1,
};

struct rope 
{
	struct rope_node *root;
	size_t size;
};

struct rope_node 
{
	enum rope_node_type type;
};

struct rope_concat 
{
	struct rope_node base;
	uint32_t weight;
	uint32_t depth;
	struct rope_node *left;
	struct rope_node *right;
};

struct rope_leaf 
{
	struct rope_node base;
	size_t size;
	const char *data;
};

struct rope *rope_new(const char *data, size_t size);
void rope_delete(struct rope *rope);
void rope_append(struct rope *rope, const char *data, size_t size);
struct rope* rope_split(struct rope *rope, size_t size);
void rope_dump_reversed(struct rope *rope, char* storage);
#endif /* ROPE_H */