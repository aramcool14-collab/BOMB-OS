#include "bomb_lang.h"

#define VAR_COUNT 8
#define NAME_SIZE 12

typedef struct { char name[NAME_SIZE]; int value; int used; } Variable;
static Variable vars[VAR_COUNT];

static int starts_with(const char *s,const char *p) {
    while(*p) { if(*s++!=*p++) return 0; }
    return 1;
}
static void send(bomb_lang_output_fn out,const char *s) { if(out) out(s); }
static int is_space(char c) { return c==' ' || c=='\t'; }
static const char *skip_space(const char *s) { while(is_space(*s))++s; return s; }
static int parse_number(const char **p,int *value) {
    int sign=1, n=0, any=0;
    const char *s=skip_space(*p);
    if(*s=='-'){sign=-1;++s;}
    while(*s>='0'&&*s<='9'){any=1;n=n*10+(*s-'0');++s;}
    if(!any)return 0;
    *value=n*sign;*p=s;return 1;
}
static int read_name(const char **p,char *name) {
    const char *s=skip_space(*p); unsigned n=0;
    while(((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||*s=='_') && n<NAME_SIZE-1) name[n++]=*s++;
    name[n]=0; *p=s; return n!=0;
}
static Variable *find_var(const char *name,int create) {
    Variable *free_slot=0;
    for(unsigned i=0;i<VAR_COUNT;i++) {
        if(vars[i].used) {
            unsigned j=0; while(vars[i].name[j]&&name[j]&&vars[i].name[j]==name[j])j++;
            if(!vars[i].name[j]&&!name[j])return &vars[i];
        } else if(!free_slot) free_slot=&vars[i];
    }
    if(create&&free_slot) {
        unsigned j=0;while(name[j]&&j<NAME_SIZE-1){free_slot->name[j]=name[j];j++;}
        free_slot->name[j]=0;free_slot->value=0;free_slot->used=1;return free_slot;
    }
    return 0;
}
static void number_text(int value,char *out) {
    char tmp[12]; unsigned n=0; unsigned v;
    if(value<0){*out++='-';v=(unsigned)(-(value+1))+1u;}else v=(unsigned)value;
    do{tmp[n++]=(char)('0'+v%10u);v/=10u;}while(v&&n<sizeof(tmp));
    while(n)*out++=tmp[--n];*out=0;
}
static void emit_value(bomb_lang_output_fn out,int value) {
    char b[16];number_text(value,b);send(out,b);
}
int bomb_lang_run(const char *line,bomb_lang_output_fn output) {
    if(!line)return 0;
    line=skip_space(line);
    if(!*line||*line=='#')return 1;

    if(starts_with(line,"say ")) {
        const char *s=skip_space(line+4);
        if(*s=='"')++s;
        send(output,s);return 1;
    }
    if(starts_with(line,"set ")) {
        const char *p=line+4;char name[NAME_SIZE];int value;
        if(!read_name(&p,name)||!parse_number(&p,&value)){send(output,"Use: set score 10");return 1;}
        Variable *v=find_var(name,1);if(!v){send(output,"No free variables left (limit: 8).");return 1;}
        v->value=value;send(output,"Variable saved.");return 1;
    }
    if(starts_with(line,"add ")||starts_with(line,"subtract ")) {
        int subtract=starts_with(line,"subtract ");const char *p=line+(subtract?9:4);
        char name[NAME_SIZE];int amount;
        if(!read_name(&p,name)||!parse_number(&p,&amount)){send(output,"Use: add score 2");return 1;}
        Variable *v=find_var(name,0);if(!v){send(output,"Make the variable first with set.");return 1;}
        v->value+=subtract?-amount:amount;send(output,"Done.");return 1;
    }
    if(starts_with(line,"print ")) {
        char name[NAME_SIZE];const char *p=line+6;
        if(!read_name(&p,name)){send(output,"Use: print score");return 1;}
        Variable *v=find_var(name,0);if(!v){send(output,"That variable does not exist.");return 1;}
        emit_value(output,v->value);return 1;
    }
    if(starts_with(line,"make app ")) { send(output,"APP:");send(output,line+9);return 1; }
    if(starts_with(line,"label ")) { send(output,"LABEL:");send(output,line+6);return 1; }
    if(starts_with(line,"button ")) { send(output,"BUTTON:");send(output,line+7);return 1; }
    if(starts_with(line,"end")) { send(output,"APP:END");return 1; }

    if(starts_with(line,"open ")) {
        if(starts_with(line+5,"calculator"))send(output,"OPEN:CALCULATOR");
        else if(starts_with(line+5,"games"))send(output,"OPEN:GAMES");
        else if(starts_with(line+5,"settings"))send(output,"OPEN:SETTINGS");
        else if(starts_with(line+5,"code"))send(output,"OPEN:CODE");
        else send(output,"Unknown app. Try calculator, games, settings, or code.");
        return 1;
    }
    if(starts_with(line,"game ")) {
        if(starts_with(line+5,"start"))send(output,"GAME:START");
        else if(starts_with(line+5,"stop"))send(output,"GAME:STOP");
        else send(output,"Try: game start or game stop.");
        return 1;
    }
    if(starts_with(line,"clear")){send(output,"SCREEN:CLEAR");return 1;}
    if(starts_with(line,"help")) {
        send(output,"Commands: say, set, add, subtract, print, make app, label, button, end, open, game, clear.");return 1;
    }
    send(output,"Unknown command. Type help.");return 0;
}
