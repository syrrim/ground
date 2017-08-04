#ifndef NODE_PARSE_H
#define NODE_PARSE_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <map.h>

typedef struct Buf {
    int fd;
    char * start;
    char * pos;
    char * end;
} Buf;

typedef map_str_t * Attrs;

typedef union Node Node;
typedef struct Nodelist {
    int size;
    int len;
    Node ** start;
} NodeList;

enum NodeType {DATA_NODE, TEXT_NODE};

#define NODESTART \
    enum NodeType type; \

typedef struct AnyNode {
    NODESTART
} AnyNode;

typedef struct DataNode {
    NODESTART
    char * name;
    Attrs attrs;
    NodeList nodelist;
} DataNode;

typedef struct TextNode {
    NODESTART
    char * text;
} TextNode;
#undef NODESTART


union Node{
    struct AnyNode any;
    DataNode data; 
    TextNode text;
};


typedef struct {
    char * names[32];
    struct {char * key; char * val;} attrs[32];
} Query;

typedef struct {
    NodeList * root;
    int depth;
    int inds[64];
} Stack;

bool matches(Node * node, Query * query);

char peek(Buf * buf);
int advance(Buf * buf);

#define ADVANCE(buf) if(advance(buf)){return 1;}

#define EAT(buf, str) while(is_in(peek(buf), str)){ADVANCE(buf)} 
#define EAT_UNTIL(buf, str) while(!is_in(peek(buf), str)){ADVANCE(buf)} 

bool is_in(char ch, char * str);

char * get_until(Buf * buf, char * match);

Query parse_query(Buf * buf);

void init_parse();
Buf * from_fd(int fd);
int exe(int fd, NodeList * nodelist);

int push(NodeList * nodelist, Node * node);

void rot(NodeList * nodelist, int num);

int insert(Buf * buf, NodeList * nodelist, int pos);
#endif
