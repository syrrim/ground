#include "nodes.h"

static char * blank = "";

int push(NodeList * nodelist, Node * node){
    if(nodelist->size == 0){
        nodelist->size = 8;
        nodelist->start = malloc(nodelist->size * sizeof(Node *));
    }else if(nodelist->size <= nodelist->len){
        nodelist->size <<= 1;
        nodelist->start = realloc(nodelist->start, nodelist->size);
    }
    if(!nodelist->start)
        return 1;
    ((Node **)nodelist->start)[nodelist->len] = node;
    nodelist->len ++;
    return 0;
}

#define SWAP(list, a, b)do{\
        __typeof__(*list) _hold = list[a];\
        list[a] = list[b];\
        list[b] = _hold;\
    }while(0);

#define REV(list, s, e)do{\
    for(int _ind = 0; _ind < (e - s)/2; _ind++)\
            SWAP(list, _ind + s, e - _ind - 1);\
    }while(0);

//stolen from https://stackoverflow.com/a/4457397/4455114
void rot(NodeList * nodelist, int num){
    Node ** list = nodelist->start;
    int len = nodelist->len;
    REV(list, 0, len);
    REV(list, 0, num);
    REV(list, num, len);
}

int insert(Buf * buf, NodeList *nodelist, int pos){
    rot(nodelist, pos);

    EAT(buf, " \n\r")

    while(true){
        if(peek(buf) == '<'){
            ADVANCE(buf)
            if(peek(buf) == '/'){
                while(peek(buf) != '>')ADVANCE(buf);
                ADVANCE(buf);
                break;
            }
            char * name = get_until(buf, " \n\r>=\"");
            Attrs attrs = malloc(sizeof(Attrs));
            map_init(attrs);
            EAT(buf, " \n\r")
            while(peek(buf) != '>'){
                char * key, * value;
                if(peek(buf) == '"'){
                    ADVANCE(buf)
                    key = get_until(buf, "\"");
                    ADVANCE(buf)
                }else{
                    key = get_until(buf, " \n\r>=");
                }
                if(!key)return 1;
                EAT(buf, " \n\r")
                if(peek(buf) == '='){
                    ADVANCE(buf)
                    EAT(buf, " \n\r")
                    if(*buf->pos == '"'){
                        ADVANCE(buf)
                        value = get_until(buf, "\"");
                        ADVANCE(buf)
                    }else{
                        value = get_until(buf, " \n\r>=");
                    }
                    if(!value)return 1;
                }else{
                    value = blank;
                }
                map_set(attrs, key, value);
            }
            ADVANCE(buf)
            Node * node = malloc(sizeof(Node));
            node->data = (DataNode){.type=DATA_NODE, .name=name, .attrs=attrs};
            if(insert(buf, &node->data.nodelist, 0))
                return 1;
            push(nodelist, node);
        }else{
            char * text = get_until(buf, "<\0");
            Node * node = malloc(sizeof(Node));
            node->text = (struct TextNode){TEXT_NODE, text};
            push(nodelist, node);
            if(peek(buf) != '<'){
                break;
            }
        }
    }
    rot(nodelist, nodelist->len - pos);
    return 0;
}

void close_node(Node * node){
    switch(node->any.type){
        case DATA_NODE:
            for(int i = 0; i < node->data.nodelist.len; i++)
                close_node(node->data.nodelist.start[i]);
            map_deinit(node->data.attrs);
            if(node->data.name)free(node->data.name);
            break;
        case TEXT_NODE:
            if(node->text.text)free(node->text.text);
            break;
    }
    free(node);
}

int cpylen(char * dest, char * src){
    int ind;
    while(src[ind]){
        *(dest++) = src[ind++];
    }
    return ind;
}

/*
void serialize(Node * node){
    while(node){
        printf("type:%d\n", node->any.type);
        printf("text:%p\n", node->text.text);
        switch(node->any.type){
            case DATA_NODE:
                printf("<%s", node->data.name);
                Attrs attrs = node->data.attrs;
                map_iter_t it = map_iter(attrs);
                const char * key;
                while((key = map_next(attrs, &it))){
                    printf(" \"%s\"=\"%s\"", key, *map_get(attrs, key));
                }
                printf(">\n");
                serialize(node->data.child);
                printf("</%s>\n", node->data.name);

                break;
            case TEXT_NODE:
                if(node->text.text)
                    printf("%s\n", node->text.text);
                break;
        }
        node = node->any.next;
    }
}
*/


/*
int main(int argc, char ** argv){
    char *data = "<hello \"state d\"=\"good enough\">Non-code<treat></treat></hello>goodbye";
    Buf buf = (Buf){-1, data, data, data + strlen(data)};
    Node p;
    Node * node = parse(&buf, &p);
    serialize(node);
}*/
