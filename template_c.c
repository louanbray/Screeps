#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum tileType {
    WALL,
    RESO,
    DASH,
    PROT,
    BONK,
    GOLD
} tileType;

typedef struct tile {
    tileType type;  // the type of the tile
    int amt;        // amount of resources on the tile (0 <= amt <= 10)
    int meta;       // metadata regarding special tiles (>= 0)
} tile;

typedef struct minion {
    int owner;     // id refering to the owner of the minion
    int x;         // coords
    int y;         // coords
    int carry;     // the amount of resources the minion is currently holding
    int hp;        // hp
    int capacity;  // max amount of resources the minion can carry
    int atk;       // attack
} minion;

typedef struct randomEvent {
    int id;        // id of the random event
    int execTurn;  // the turn at which the random event will trigger
} randomEvent;

int MAP_LEN = -1;
int ID = -1;
int WALLS = true;
int BaseX = -1;
int BaseY = -1;
int MY_RESOURCES;
int MY_MINIONS = 0;
minion* MINIONS;
int MINIONS_LEN;
FILE* ptr;
bool** targeted;
int* BASES;

// parsing functions
int read_int(FILE* ptr) {
    int buffer = 0;
    int sign = 1;
    char c = fgetc(ptr);
    while (c != EOF && !((c >= 48 && c <= 57) || c == '-')) {  // align to closest integer
        c = fgetc(ptr);
    }
    while (c != EOF && ((c >= 48 && c <= 57) || c == '-')) {
        if (c == '-') {
            sign = -1;
        } else {
            buffer = 10 * buffer + (c - 48); /* this is a common technique to read integers, you should remember it */
        }
        c = fgetc(ptr);
    }
    // fprintf(stderr, "- %d -\n",buffer);
    return (c != EOF) ? (buffer * sign) : (-7272727);  // completely random EOF return
}

tileType read_tile_type(FILE* ptr) {
    char c = fgetc(ptr);
    if (c == '\r' || c == '\n') c = fgetc(ptr);
    switch (c) {
        case 'W':
            return WALL;
        case 'R':
            return RESO;
        case 'D':
            return DASH;
        case 'S':
            return PROT;
        case 'F':
            return BONK;
        case 'M':
            return GOLD;
        default:
            return 0;
    }
}

void read_data(tile*** map, int* mapLenP, minion** minions, int* minionLenP, randomEvent** randEvs, int* randEvLenP, int* idP, int* rscP, int* bXP, int* bYP, int* curTurnP, int* maxTurnP) {
    FILE* ptr = fopen("mapData.txt", "r");
    *mapLenP = read_int(ptr);
    (*map) = malloc(sizeof(tile*) * (*mapLenP));
    // printf(">> %d\n", *mapLenP);
    for (int i = 0; i < (*mapLenP); i++) {
        (*map)[i] = malloc(sizeof(tile) * (*mapLenP));
        for (int j = 0; j < (*mapLenP); j++) {
            (*map)[i][j].type = read_tile_type(ptr);
            (*map)[i][j].amt = read_int(ptr);
            (*map)[i][j].meta = read_int(ptr);
            // printf("%d,%d,%d ",(*map)[i][j].type, (*map)[i][j].amt, (*map)[i][j].meta);
        }
        // printf("\n");
    }
    // printf(">> %d\n", *mapLenP);
    *minionLenP = read_int(ptr);
    (*minions) = malloc(sizeof(minion) * (*minionLenP));
    for (int m = 0; m < *minionLenP; m++) {
        (*minions)[m].owner = read_int(ptr);
        (*minions)[m].x = read_int(ptr);
        (*minions)[m].y = read_int(ptr);
        (*minions)[m].carry = read_int(ptr);
        (*minions)[m].hp = read_int(ptr);
        (*minions)[m].capacity = read_int(ptr);
        (*minions)[m].atk = read_int(ptr);
    }
    // printf(">> %d\n", *minionLenP);
    *idP = read_int(ptr);
    *rscP = read_int(ptr);
    *bXP = read_int(ptr);
    *bYP = read_int(ptr);
    *curTurnP = read_int(ptr);
    *maxTurnP = read_int(ptr);
    *randEvLenP = 0;
    (*randEvs) = malloc(sizeof(randomEvent) * ((((*maxTurnP) + 150) / 150)));
    // printf("%d %d %d %d %d %d\n",*idP,*rscP,*bXP,*bYP,*curTurnP,*maxTurnP);
    int retVal = read_int(ptr);
    while (retVal != -7272727) {
        (*randEvs)[*randEvLenP].id = retVal;
        (*randEvs)[*randEvLenP].execTurn = read_int(ptr);
        *randEvLenP += 1;
        retVal = read_int(ptr);
    }
    fclose(ptr);
}

void free_data(tile** map, int mapLen, minion* minions, randomEvent* randEvents) {
    for (int i = 0; i < mapLen; i++) {
        free(map[i]);
    }
    free(map);
    free(minions);
    free(randEvents);
}

// ------------
typedef struct queue_elt {
    struct queue_elt* next;
    int val;
} queue_elt;

typedef struct queue {
    queue_elt* start;
    queue_elt* end;
} queue;

queue* create_queue() {
    queue* q = malloc(sizeof(queue));
    q->start = NULL;
    q->end = NULL;
    return q;
}

void enqueue(queue* q, int val) {
    queue_elt* elt = malloc(sizeof(queue_elt));
    elt->val = val;
    elt->next = NULL;
    queue_elt* temp = q->end;
    if (q->end != NULL)
        q->end->next = elt;
    else
        q->start = elt;
    q->end = elt;
}

int dequeue(queue* q) {
    if (q->start == NULL) return -1;
    queue_elt* elt = q->start;
    int v = elt->val;

    q->start = elt->next;
    if (q->start == NULL)
        q->end = NULL;

    free(elt);
    return v;
}

bool is_empty(queue* q) {
    return q->start == NULL;
}

void free_queue(queue* q) {
    queue_elt* elt = q->start;
    while (elt != NULL) {
        queue_elt* tmp = elt;
        elt = elt->next;
        free(tmp);
    }
    free(q);
}

// ---------- //
typedef struct node {
    tile t;
    int* links;
} node;

typedef enum Direction {
    DROITE,
    GAUCHE,
    BAS,
    HAUT
} Direction;

int pos(int x, int y) {
    return x * MAP_LEN + y;
}

void coos(int index, int* xP, int* yP) {
    int x = index / MAP_LEN;
    int y = index % MAP_LEN;
    if (x < 0 || x >= MAP_LEN || y < 0 || y >= MAP_LEN) return;
    *xP = x;
    *yP = y;
}

int index_at(int index, Direction dir) {
    int x = index / MAP_LEN;
    int y = index % MAP_LEN;

    // Vérifier les bords pour les déplacements horizontaux
    if (dir == DROITE && y == MAP_LEN - 1) return -1;
    if (dir == GAUCHE && y == 0) return -1;
    // Vérifier les bords pour les déplacements verticaux
    if (dir == BAS && x == MAP_LEN - 1) return -1;
    if (dir == HAUT && x == 0) return -1;

    return index + ((dir % 4 == 0) - (dir % 4 == 1)) + ((dir % 4 == 2) - (dir % 4 == 3)) * MAP_LEN;
}

int distance(int index_from, int index_to) {
    int x1 = index_from / MAP_LEN;
    int y1 = index_from % MAP_LEN;
    int x2 = index_to / MAP_LEN;
    int y2 = index_to % MAP_LEN;
    return abs(x1 - x2) + abs(y1 - y2);
}

Direction mirror(Direction dir) {
    switch (dir) {
        case DROITE:
            return GAUCHE;
        case GAUCHE:
            return DROITE;
        case BAS:
            return HAUT;
        case HAUT:
            return BAS;
        default:
            return -1;  // Valeur invalide
    }
}

bool** locate_minions() {
    bool** tab = malloc(sizeof(bool*) * MAP_LEN);
    for (int i = 0; i < MAP_LEN; i++) {
        bool* tmp = calloc(sizeof(bool), MAP_LEN);
        tab[i] = tmp;
    }

    for (int i = 0; i < MINIONS_LEN; i++) {
        minion it = MINIONS[i];
        tab[it.x][it.y] = true;
    }
    return tab;
}

node** convert_map_to_graph(tile** map, bool** occupe) {
    int n = MAP_LEN * MAP_LEN;
    node** g = malloc(sizeof(node*) * n);
    for (int i = 0; i < MAP_LEN; i++) {
        for (int j = 0; j < MAP_LEN; j++) {
            node* nd = malloc(sizeof(node));
            nd->t = map[i][j];
            nd->links = malloc(sizeof(int) * 4);
            for (int k = 0; k < 4; k++) {
                int h = index_at(pos(i, j), k);
                nd->links[k] = -1;
                if (h < 0 || h >= n) continue;
                int hx = h / MAP_LEN;
                int hy = h % MAP_LEN;
                if (map[hx][hy].type != WALL && !occupe[hx][hy]) nd->links[k] = h;
            }
            g[pos(i, j)] = nd;
        }
    }
    return g;
}

bool can_go_to(node** g, int index_from, Direction dir) {
    return (g[index_from]->links[dir] != -1);
}

void print_t(tile** map) {
    for (int i = 0; i < MAP_LEN; i++) {
        for (int j = 0; j < MAP_LEN; j++) {
            printf("%d | ", map[i][j].type);
        }
        printf("\n");
    }
}

void print_access(node** g, int x, int y) {
    printf("Accès de %d (x:%d, y:%d) : [%d,%d,%d,%d]", pos(x, y), x, y, g[pos(x, y)]->links[0], g[pos(x, y)]->links[1], g[pos(x, y)]->links[2], g[pos(x, y)]->links[3]);
}

// -------- Utils ----------
bool is_miner(minion m) {
    return m.capacity >= 5;
}

void swap_minion(int i, int j) {
    if (i < 0 || i >= MINIONS_LEN || j < 0 || j > MINIONS_LEN) return;
    minion tmp = MINIONS[i];
    MINIONS[i] = MINIONS[j];
    MINIONS[j] = tmp;
}

int get_minion_index_at(int x, int y) {
    int d = -1;
    for (int i = 0; i < MINIONS_LEN; i++) {
        if (MINIONS[i].x == x && MINIONS[i].y == y)
            d = i;
    }
    return d;
}

int* locate_bases(int BaseX, int BaseY) {
    int* bases = malloc(sizeof(int) * 4);
    bases[0] = pos(BaseX, BaseY);
    bases[1] = pos(MAP_LEN - 1 - BaseX, BaseY);
    bases[2] = pos(BaseX, MAP_LEN - 1 - BaseY);
    bases[3] = pos(MAP_LEN - 1 - BaseX, MAP_LEN - 1 - BaseY);

    return bases;
}

bool is_base(int index) {
    int x = index / MAP_LEN;
    int y = index % MAP_LEN;
    for (int i = 0; i < 4; i++) {
        if (index == BASES[i]) {
            return true;
        }
    }
    return false;
}

int path_to(node** g, int index_from, int index_to) {
    int n = MAP_LEN * MAP_LEN;
    bool* visite = calloc(sizeof(bool), n);
    int* parents = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        parents[i] = -1;
    }

    queue* q = create_queue();
    enqueue(q, index_from);
    visite[index_from] = true;
    int k = 0;
    while (!is_empty(q)) {
        k++;
        int s = dequeue(q);
        for (int i = 0; i < 4; i++) {
            if (can_go_to(g, s, i)) {
                int v = index_at(s, i);
                if (!visite[v]) {
                    visite[v] = true;
                    parents[v] = s;
                    if (!is_base(v)) enqueue(q, v);
                }
            }
        }
    }

    free_queue(q);
    int index_cur = -1;
    if (visite[index_to]) {
        index_cur = index_to;
        while (parents[index_cur] != index_from && parents[index_cur] != -1) {
            index_cur = parents[index_cur];
        }
    }

    free(visite);
    free(parents);

    return index_cur;
}

int best_resource_cluster_nearby(node** g, int index_from, int search_radius) {
    int n = MAP_LEN * MAP_LEN;
    int best_cluster_index = -1;
    int best_cluster_amount = -1;

    bool* visited = calloc(sizeof(bool), n);
    int* distances = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        distances[i] = -1;
    }

    queue* q = create_queue();
    enqueue(q, index_from);
    visited[index_from] = true;
    distances[index_from] = 0;

    while (!is_empty(q)) {
        int current = dequeue(q);
        int current_dist = distances[current];

        if (current_dist > search_radius) {
            break;
        }

        tile t = g[current]->t;
        if (t.type == RESO && current_dist >= 0 && !targeted[current / MAP_LEN][current % MAP_LEN] && !is_base(current)) {
            int score = t.amt * 100 / ((current_dist + 1) * 2);
            if (score > best_cluster_amount) {
                best_cluster_amount = score;
                best_cluster_index = current;
            }
        }

        for (int i = 0; i < 4; i++) {
            if (can_go_to(g, current, i)) {
                int v = index_at(current, i);
                if (!visited[v]) {
                    if (is_base(v)) continue;
                    visited[v] = true;
                    distances[v] = current_dist + 1;
                    enqueue(q, v);
                }
            }
        }
    }

    free_queue(q);
    free(visited);
    free(distances);

    // printf("Scores des ressources autour de la position (id:%d):\n", ID);
    // if (!ID)
    //     for (int i = 0; i < MAP_LEN; i++) {
    //         for (int j = 0; j < MAP_LEN; j++) {
    //             if (distances[pos(i, j)] <= 0)
    //                 printf("X | ");
    //             else
    //                 printf("%d | ", g[pos(i, j)]->t.amt * 100 / (distances[pos(i, j)] * 2));
    //         }
    //         printf("\n");
    //     }
    return best_cluster_index;
}

void tick_minions(node** g) {
    for (int i = 0; i < MINIONS_LEN; i++) {
        if (MINIONS[i].owner == ID) {
            minion m = MINIONS[i];
            int x = m.x;
            int y = m.y;
            if (is_miner(m)) {
                if (m.carry < m.capacity) {
                    if (g[pos(x, y)]->t.type != RESO || (g[pos(x, y)]->t.type == RESO && g[pos(x, y)]->t.amt == 0)) {
                        coos(path_to(g, pos(x, y), best_resource_cluster_nearby(g, pos(x, y), 20)), &x, &y);
                        if (x == m.x && y == m.y && m.carry > 0) {
                            coos(path_to(g, pos(x, y), pos(BaseX, BaseY)), &x, &y);
                        }
                    }
                } else {
                    coos(path_to(g, pos(x, y), pos(BaseX, BaseY)), &x, &y);
                }
            }
            fprintf(ptr, "%d %d %d %d\n", m.x, m.y, x, y);
            targeted[x][y] = true;
            if (m.x != x || m.y != y)
                for (int i = 0; i < 4; i++) {
                    if (index_at(pos(x, y), i) != -1)
                        g[index_at(pos(x, y), i)]->links[mirror(i)] = -1;
                    if (index_at(pos(m.x, m.y), i) != -1)
                        g[index_at(pos(m.x, m.y), i)]->links[mirror(i)] = pos(m.x, m.y);
                }
        }
    }
}

// -------------------------

int comp(const void* a, const void* b) {
    return (rand() % 65536) - 32768;
    minion* m1 = (minion*)a;
    minion* m2 = (minion*)b;
    return distance(pos(BaseX, BaseY), pos(m1->x, m1->y)) - distance(pos(BaseX, BaseY), pos(m2->x, m2->y));
}

void trie_minions() {
    qsort(MINIONS, MINIONS_LEN, sizeof(minion), comp);
}

int count_my_minions() {
    int c = 0;
    for (int i = 0; i < MINIONS_LEN; i++) {
        if (MINIONS[i].owner == ID)
            c++;
    }
    return c;
}

void create_minion() {
    if (MY_RESOURCES < 8)
        fprintf(ptr, "CREATE 1 5 1\n");
    else if (MY_RESOURCES >= 13)
        fprintf(ptr, "CREATE 2 10 1\n");
}

int main() {
    ptr = fopen("answer.txt", "w");
    tile** map;
    randomEvent* randomEvents;
    int randomEventLen;
    int myID, curTurn, maxTurns;
    read_data(&map, &MAP_LEN, &MINIONS, &MINIONS_LEN, &randomEvents, &randomEventLen, &ID, &MY_RESOURCES, &BaseX, &BaseY, &curTurn, &maxTurns);
    MY_MINIONS = count_my_minions();
    BASES = locate_bases(BaseX, BaseY);
    bool** occupe = locate_minions();
    // trie_minions();
    if (occupe[BaseX][BaseY]) swap_minion(get_minion_index_at(BaseX, BaseY), 0);
    node** g = convert_map_to_graph(map, occupe);
    targeted = malloc(sizeof(bool*) * MAP_LEN);
    for (int i = 0; i < MAP_LEN; i++) targeted[i] = calloc(sizeof(bool), MAP_LEN);

    if (MINIONS_LEN > 0)
        tick_minions(g);
    if (MINIONS_LEN < 44)
        create_minion();
    fclose(ptr);
    // best_resource_cluster_nearby(g, pos(BaseX, BaseY), 20);
    // print_t(map);
    // do your things here
    free_data(map, MAP_LEN, MINIONS, randomEvents);
    return 0;
}