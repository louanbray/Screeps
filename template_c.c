#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

typedef enum tileType {WALL,RESO,DASH,PROT,BONK,GOLD} tileType;

typedef struct tile {
    tileType type;  // the type of the tile
    int amt;        // amount of resources on the tile (0 <= amt <= 10)
    int meta;       // metadata regarding special tiles (>= 0)
} tile;

typedef struct minion {
    int owner;      // id refering to the owner of the minion
    int x;          // coords
    int y;          // coords
    int carry;      // the amount of resources the minion is currently holding
    int hp;         // hp
    int capacity;   // max amount of resources the minion can carry
    int atk;        // attack
} minion;

typedef struct randomEvent {
    int id;         // id of the random event
    int execTurn;   // the turn at which the random event will trigger
} randomEvent;

// parsing functions
int read_int(FILE* ptr) {
    int buffer=0;
    int sign=1;
    char c = fgetc(ptr);
    while(c != EOF && !((c>=48 && c<=57) || c=='-')) {  // align to closest integer
        c = fgetc(ptr);
    }
    while(c != EOF && ((c>=48 && c<=57) || c=='-')) {
        if(c == '-') {
            sign=-1;
        } else {
            buffer = 10*buffer+(c-48);  /* this is a common technique to read integers, you should remember it */
        }
        c = fgetc(ptr);
    }
    //fprintf(stderr, "- %d -\n",buffer);
    return (c!=EOF)?(buffer*sign):(-7272727); // completely random EOF return
}

tileType read_tile_type(FILE* ptr) {
    switch (fgetc(ptr)) {
        case 'W': return WALL;
        case 'R': return RESO;
        case 'D': return DASH;
        case 'S': return PROT;
        case 'F': return BONK;
        case 'M': return GOLD;
        default: return 0;
    }
}

void read_data(tile*** map, int* mapLenP, minion** minions, int* minionLenP, randomEvent** randEvs, int* randEvLenP, int* idP, int* rscP, int* bXP, int* bYP, int* curTurnP, int* maxTurnP) {
    FILE* ptr = fopen("mapData.txt", "r");
    *mapLenP=read_int(ptr);
    (*map)=malloc(sizeof(tile*)*(*mapLenP));
    //printf(">> %d\n", *mapLenP);
    for(int i=0; i<(*mapLenP);i++) {
        (*map)[i]=malloc(sizeof(tile)*(*mapLenP));
        for(int j=0; j<(*mapLenP);j++) {
            (*map)[i][j].type=read_tile_type(ptr);
            (*map)[i][j].amt=read_int(ptr);
            (*map)[i][j].meta=read_int(ptr);
            //printf("%d,%d,%d ",(*map)[i][j].type, (*map)[i][j].amt, (*map)[i][j].meta);
        }
        //printf("\n");
    }
    //printf(">> %d\n", *mapLenP);
    *minionLenP=read_int(ptr);
    (*minions)=malloc(sizeof(minion)*(*minionLenP));
    for(int m=0; m<*minionLenP;m++) {
        (*minions)[m].owner=read_int(ptr);
        (*minions)[m].x=read_int(ptr);
        (*minions)[m].y=read_int(ptr);
        (*minions)[m].carry=read_int(ptr);
        (*minions)[m].hp=read_int(ptr);
        (*minions)[m].capacity=read_int(ptr);
        (*minions)[m].atk=read_int(ptr);
    }
    //printf(">> %d\n", *minionLenP);
    *idP=read_int(ptr);
    *rscP=read_int(ptr);
    *bXP=read_int(ptr);
    *bYP=read_int(ptr);
    *curTurnP=read_int(ptr);
    *maxTurnP=read_int(ptr);
    *randEvLenP=0;
    (*randEvs)=malloc(sizeof(randomEvent)*((((*maxTurnP)+150)/150)));
    //printf("%d %d %d %d %d %d\n",*idP,*rscP,*bXP,*bYP,*curTurnP,*maxTurnP);
    int retVal=read_int(ptr);
    while(retVal != -7272727) {
        (*randEvs)[*randEvLenP].id=retVal;
        (*randEvs)[*randEvLenP].execTurn=read_int(ptr);
        *randEvLenP += 1;
        retVal=read_int(ptr);
    }
    fclose(ptr);
}

void free_data(tile** map, int mapLen, minion* minions, randomEvent* randEvents) {
    for(int i=0; i<mapLen;i++) {
        free(map[i]);
    }
    free(map);
    free(minions);
    free(randEvents);
}

// ---------- //

int main() {
    tile** map;
    int mapLen;
    minion* minions;
    int minionsLen;
    randomEvent* randomEvents;
    int randomEventLen;
    int myID,myResources,baseX,baseY,curTurn,maxTurns;
    read_data(&map,&mapLen,&minions,&minionsLen,&randomEvents,&randomEventLen,&myID,&myResources,&baseX,&baseY,&curTurn,&maxTurns);

    // do your things here

    free_data(map,mapLen,minions,randomEvents);
    return 0;
}