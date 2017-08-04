#include "nodes.h"

#define BUFSIZE 1024

int fill(Buf * buf){
    int n = read(buf->fd, buf->start, BUFSIZE);
    if(n<=0){
        return n;
    }
    buf->pos = buf->start;
    buf->end = buf->start + n;
    return 0;
}

char peek(Buf * buf){
    if(buf->pos >= buf->end)
        if(fill(buf))
            return '\0';
    return *buf->pos;
}

int advance(Buf * buf){
    buf->pos++;
    return 0;
}

bool is_in(char ch, char * str){
    for(int i = 0; str[i]; i++)
        if(ch == str[i])
            return true;
    return false;
};

char * get_until(Buf * buf, char * match){
    int size = 8;
    char * text = malloc(sizeof(char) * size);
    int ind = 0;
    while(true){
        if(is_in(peek(buf), match))break;
        text[ind++] = peek(buf);
        if(ind >= size){
            size <<= 1;
            text = realloc(text, size);
        }
        if(advance(buf))break;
    }
    text[ind] = '\0';
    return text;
}
Query parse_query(Buf * buf){
    Query query;
    int pair_ind = 0;
    while(true){
        char * word, * val;
        EAT(buf, " ");
        if(peek(buf) == '"'){
            ADVANCE(buf);
            key = get_until(buf, "\"");
            ADVANCE(buf);
        }else{
            key = get_until(buf, " =\r\n");
        }
        if(!key)return 1;
        EAT(buf, " ");
        if(peek(buf) == '='){
            if(peek(buf) == '"'){
                val = get_until(buf, "\"");
            }else{
                val = get_until(buf, " ");
            }
        if(!val)return 1;
        query.pairs[pair_ind].key = key;
        query.pairs[pair_ind].val = val;
        ind ++;
    }
    query.pairs[pair_ind] = '\0';
}

Buf * from_fd(int fd){
    char * block = malloc(BUFSIZE);
    Buf * buf = malloc(sizeof(Buf));
    memcpy(buf, &(Buf){fd, block, NULL, NULL}, sizeof(buf));
    if(fill(buf))return NULL;
    return buf;
}
void close_buf(Buf * buf){
    free(buf->start);
    free(buf);
}

enum Commands {
    NEXTSIBLING, 
    PREVSIBLING, 
    CHILD, 
    PARENT, 
    PNAME, 
    PTEXT, 
    PATTR, 
    INEXT, 
    IPREV, 
    DELETE, 
    FIND,
    MARK,
    GOTO,
};

static map_t(enum Commands) commands;
void init_parse(){
    map_init(&commands);
#define SET(str, val) map_set(&commands, str, val);
    SET("nextsibling",	NEXTSIBLING);
    SET("ns",		NEXTSIBLING);
    SET("prevsibling",	PREVSIBLING);
    SET("ps",		PREVSIBLING);
    SET("child",	CHILD);
    SET("c",		CHILD);
    SET("parent",	PARENT);
    SET("p",		PARENT);
    SET("printname",	PNAME);
    SET("pname",	PNAME);
    SET("pn",		PNAME);
    SET("printtext",	PTEXT);
    SET("ptext",	PTEXT);
    SET("pt",		PTEXT);
    SET("printattr",	PATTR);
    SET("pattr",	PATTR);
    SET("pa",		PATTR);
    SET("insertnext",	INEXT);
    SET("inext",	INEXT);
    SET("in",		INEXT);
    SET("insertprev",	IPREV);
    SET("iprev",	IPREV);
    SET("ip",		IPREV);
    SET("delete",	DELETE);
    SET("d",		DELETE);
    SET("find",		FIND);
    SET("f",		FIND);
    SET("mark",		MARK);
    SET("m",		MARK);
    SET("goto",		GOTO);
    SET("g",		GOTO);
#undef SET
}

bool matches(Node * node, Query * query){
    return strcmp(*map_get(node->data.attrs, query->key), query->val) == 0;
}

int exe(int fd, NodeList * root){
#define WSTR(str)for(char * _str = str; *_str; _str++)write(fd, _str, 1);
#define WSTRLN(str)WSTR(str);write(fd, "\n", 1);
    Buf * buf = from_fd(fd);
    Stack stack = (Stack){root, 0, {0}};
    char * pairs[64];
    char * names[64];

    map_t(Stack) marks;
    map_init(marks);

#define NODELIST ({ \
        NodeList * list = root; \
        for(int _i = 0; _i < stack.depth; _i++){\
            list = list->start[stack.inds[_i]]->nodelist;\
        }\
        list;\
    })
#define NODE (NODELIST->start[stack.inds[stack.depth]])
    while(true){
        char * word = get_until(buf, " \n\r");
        printf("%s\n", word);
        enum Commands * comm = map_get(&commands, word);
        if(advance(buf))return 1;
        if(!comm)return 1;
        switch(*comm){
            case NEXTSIBLING:
                if(NODELIST->len > inds[depth]){
                    inds[depth] ++;
                }
                break;
            case PREVSIBLING:
                if(inds[depth] > 0){
                    inds[depth] --;
                }
                break;
            case CHILD:
                if(NODE->any.type == DATA_NODE){
                    depth ++;
                    lists[depth] = &NODE->data.nodelist;
                    inds[depth] = 0;
                }
                break;
            case PARENT:
                if(depth > 0){
                    depth --;
                }
                break;
            case PNAME:
                if(NODE->any.type == DATA_NODE)
                    WSTRLN(NODE->data.name)
                break;
            case PTEXT:
                printf("printtext, %d, %d\n", inds[depth], NODE->any.type);
                if(NODE->any.type == TEXT_NODE){
                    WSTRLN(NODE->text.text)
                }
                break;
            case PATTR:
                if(NODE->any.type == DATA_NODE){
                    EAT(buf, " ");
                    char * key;
                    if(peek(buf) == '"'){
                        ADVANCE(buf)
                        key = get_until(buf, "\"");
                        ADVANCE(buf)
                    }else{
                        key = get_until(buf, " \n\r");
                    }
                    char ** val = map_get(NODE->data.attrs, key);
                    if(val){
                        WSTRLN(*val);
                    }else{
                        WSTRLN("");
                    }

                }
                break;
            case INEXT:
                printf("inserting\n");
                insert(buf, NODELIST, inds[depth]+1);
                break;
            case IPREV:
                insert(buf, NODELIST, inds[depth]);
            case DELETE:
                for(int i = inds[depth]; i < NODELIST->len-1; i++){
                    NODELIST->start[i] = NODELIST->start[i+1];
                }
                NODELIST->len --;
                break;
            case FIND:

                break;
            case MARK:
                char * word = get_until(" \r\n");
                map_set(&marks, word, stack);
                free(word);
                break;
            case GOTO:
                char * word = get_until(" \r\n");
                Stack * new = map_get(&marks, word);
                if(new){
                    stack = *new;
                }
                free(word);
                break;
        }
        EAT(buf, " \n\r");
    }
#undef NODE
#undef NODELIST
#undef WSTRLN
#undef WSTR
}

