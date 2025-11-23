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
bool** occupe;
bool** my_minion_is_here;
int* BASES;

// my consts -----------------------
int DELTA_CAPACITY = 0;  // tolerance for miners that are almost full

// parsing functions ------------------
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

// Da queue ---------------------------
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

// ------------------------------------
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

int pos(int x, int y) {
    return x * MAP_LEN + y;
}

void coos(int index, int* xP, int* yP) {
    if (index == -1) return;
    int x = index / MAP_LEN;
    int y = index % MAP_LEN;
    if (x < 0 || x >= MAP_LEN || y < 0 || y >= MAP_LEN) return;
    *xP = x;
    *yP = y;
}

int index_at(int index, Direction dir) {
    int x = index / MAP_LEN;
    int y = index % MAP_LEN;
    switch (dir) {
        case DROITE:
            if (y == MAP_LEN - 1) return -1;
            return index + 1;
        case GAUCHE:
            if (y == 0) return -1;
            return index - 1;
        case BAS:
            if (x == MAP_LEN - 1) return -1;
            return index + MAP_LEN;
        case HAUT:
            if (x == 0) return -1;
            return index - MAP_LEN;
        default:
            return -1;
    }
}

int distance(int index_from, int index_to) {
    int x1 = index_from / MAP_LEN;
    int y1 = index_from % MAP_LEN;
    int x2 = index_to / MAP_LEN;
    int y2 = index_to % MAP_LEN;
    return abs(x1 - x2) + abs(y1 - y2);
}

// ----------------------------------

bool** locate_minions() {
    bool** tab = malloc(sizeof(bool*) * MAP_LEN);
    for (int i = 0; i < MAP_LEN; i++) {
        tab[i] = calloc(MAP_LEN, sizeof(bool));
    }

    for (int i = 0; i < MINIONS_LEN; i++) {
        minion it = MINIONS[i];
        tab[it.x][it.y] = true;
    }
    return tab;
}

bool** locate_my_minions() {
    bool** tab = malloc(sizeof(bool*) * MAP_LEN);
    for (int i = 0; i < MAP_LEN; i++) {
        tab[i] = calloc(MAP_LEN, sizeof(bool));
    }

    for (int i = 0; i < MINIONS_LEN; i++) {
        minion it = MINIONS[i];
        if (it.owner != ID) continue;
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
                if (h == -1) continue;
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

void swap_minions(int i, int j) {
    if (i < 0 || i >= MINIONS_LEN || j < 0 || j >= MINIONS_LEN) return;
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

int path_to(node** g, int index_from, int index_to, bool bypass) {
    if (index_from == index_to) return index_from;
    int n = MAP_LEN * MAP_LEN;
    bool* visite = calloc(n, sizeof(bool));
    int* parents = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        parents[i] = -1;
    }

    queue* q = create_queue();
    enqueue(q, index_from);
    visite[index_from] = true;

    bool found = false;
    while (!is_empty(q)) {
        int s = dequeue(q);
        for (int i = 0; i < 4; i++) {
            int v = g[s]->links[i];
            if (index_at(s, i) == -1) continue;
            int x, y;
            coos(index_at(s, i), &x, &y);
            if (v == -1 && (!bypass || !my_minion_is_here[x][y])) continue;
            v = index_at(s, i);
            if (!visite[v]) {
                visite[v] = true;
                parents[v] = s;
                if (v == index_to) {
                    found = true;
                    break;
                }
                if (!is_base(v)) enqueue(q, v);
            }
        }
        if (found) break;
    }
    free_queue(q);

    int result = -1;
    if (found) {
        int index_cur = index_to;
        while (parents[index_cur] != index_from && parents[index_cur] != -1) index_cur = parents[index_cur];
        if (parents[index_cur] == index_from)
            result = index_cur;
        else if (index_cur == index_from)
            result = index_to;
    }
    free(visite);
    free(parents);
    return result;
}

int best_resource_cluster_nearby(node** g, int index_from, int search_radius) {
    int n = MAP_LEN * MAP_LEN;
    int best_cluster_index = -1;
    int best_cluster_score = -100;

    int* distances = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        distances[i] = -1;
    }

    queue* q = create_queue();
    enqueue(q, index_from);
    distances[index_from] = 0;

    while (!is_empty(q)) {
        int current = dequeue(q);
        int current_dist = distances[current];

        if (current_dist > search_radius) {
            break;
        }

        tile t = g[current]->t;
        if (t.type == RESO && current_dist >= 0 && !targeted[current / MAP_LEN][current % MAP_LEN] && !is_base(current)) {
            float score = (t.amt) / ((current_dist + 1)); // - distance(index_from, pos(BaseX, BaseY)) ?? idk
            if (score > best_cluster_score) {
                best_cluster_score = score;
                best_cluster_index = current;
            }
        }

        for (int i = 0; i < 4; i++) {
            if (can_go_to(g, current, i)) {
                int v = index_at(current, i);
                if (v != -1 && distances[v] == -1) {
                    if (is_base(v)) continue;
                    distances[v] = current_dist + 1;
                    enqueue(q, v);
                }
            }
        }
    }

    free_queue(q);
    free(distances);

    return best_cluster_index;
}

int target_base(node** g, minion m) {
    int x = m.x;
    int y = m.y;
    coos(path_to(g, pos(x, y), pos(BaseX, BaseY), false), &x, &y);

    if (x == m.x && y == m.y) {
        minion* base_minions = malloc(sizeof(minion) * MINIONS_LEN);
        int len = 0;
        for (int i = 0; i < MINIONS_LEN; i++) {
            if (MINIONS[i].owner == ID && distance(pos(MINIONS[i].x, MINIONS[i].y), pos(BaseX, BaseY)) <= 5) {
                base_minions[len] = MINIONS[i];
                len++;
            }
        }
        int j = -1;
        int best_dist = 65535;
        for (int i = 0; i < len; i++) {
            minion bm = base_minions[i];
            if (bm.x == m.x && bm.y == m.y) continue;
            int d = distance(pos(bm.x, bm.y), pos(BaseX, BaseY));
            if (d < best_dist && d < distance(pos(m.x, m.y), pos(BaseX, BaseY))) {
                best_dist = d;
                j = i;
            }
        }
        if (j != -1 && best_dist != 65535) {
            coos(path_to(g, pos(m.x, m.y), pos(base_minions[j].x, base_minions[j].y), true), &x, &y);
        }  // else {
        //     coos(path_to(g, pos(x, y), pos(BaseX, BaseY), true), &x, &y);
        // }
        free(base_minions);
    }

    return pos(x, y);
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
                        coos(path_to(g, pos(x, y), best_resource_cluster_nearby(g, pos(x, y), 20), false), &x, &y);
                        if (x == m.x && y == m.y && m.carry > 0) coos(target_base(g, m), &x, &y);
                    }
                } else {
                    coos(target_base(g, m), &x, &y);
                }
            }
            fprintf(ptr, "%d %d %d %d\n", m.x, m.y, x, y);  //! Fait gaffe si tu fais un file, il faut executer cette ligne que lorsqu'on est sûr de la cible d'un minion.
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
    minion* m1 = (minion*)a;
    minion* m2 = (minion*)b;
    return m1->carry - m2->carry;
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

int calculate_minion_cost(int atk, int capacity, int hp) {
    return atk + capacity/2 + hp/2;
}

int optimal_capacity(tile** map, int baseX, int baseY) {
    int total_dist = 0;
    int resource_count = 0;
    
    for (int i = 0; i < MAP_LEN; i++) {
        for (int j = 0; j < MAP_LEN; j++) {
            if (map[i][j].type == RESO && map[i][j].amt > 0) {
                total_dist += abs(i - baseX) + abs(j - baseY);;
                resource_count++;
            }
        } // sharingan, can be modified
    }
    if (resource_count == 0) return 5;
    
    float avg_dist = (float)total_dist / resource_count;
    int capacity = (int)(avg_dist * 0.35f); // adjust the multiplier as needed
    
    if (capacity < 4) capacity = 4;
    if (capacity > 12) capacity = 20;
    
    printf("'Optimal capacity': %d\n", capacity);
    if (MY_RESOURCES < calculate_minion_cost(1, capacity, 1)) {
        capacity = (MY_RESOURCES - 2) * 2;
    }
    return capacity;
}

void create_minion(tile** g) {
    if (MY_RESOURCES > 6)
        fprintf(ptr, "CREATE 1 %d 1\n", optimal_capacity(g, BaseX, BaseY));
}

// LE CHANTIER DE TRAZE -----------------
typedef struct theoretical_action {  // used for miners :)
    int minion_pos;                  // minion pos
    int target_x;                    // x pos of target
    int target_y;                    // y pos of target
    int target_index;                // pos of target
    float score;                     // action score
    bool is_best;                    // indicates if action is the best (hmm kinda obvious said like this)
} theoretical_action;

float calculate_action_score(node** g, minion m, int resource_index) {
    if (resource_index == -1) return -1.0f;
    int dist = distance(pos(m.x, m.y), resource_index) + 1;  // +1 to avoid division by zero
    tile t = g[resource_index]->t;
    int resource_value = t.amt;
    float base_score = (float)resource_value / (float)dist;
    if (m.carry > 0) base_score *= 1.1f;                // prioritize the elderly miners >:)
    if (resource_value < m.carry) base_score *= 1.2f;   // good enough ig
    if (resource_value == m.carry) base_score *= 1.4f;  // perfect if it fills perfectly :D
    float capacity_ratio = (float)m.capacity / 10.0f;
    base_score *= capacity_ratio;

    // can still add any factor you find necessary here :p

    return base_score;
}

theoretical_action find_best_action(node** g, minion m, int minion_ind) {
    theoretical_action action;
    action.minion_pos = minion_ind;
    action.score = -1.0f;
    action.is_best = false;

    if (is_miner(m)) {
        if (m.carry < m.capacity - DELTA_CAPACITY) {
            int best_resource = best_resource_cluster_nearby(g, pos(m.x, m.y), 20);
            float resource_score = calculate_action_score(g, m, best_resource);

            if (resource_score > action.score) {
                action.target_index = best_resource;
                coos(best_resource, &action.target_x, &action.target_y);
                action.score = resource_score;
                action.is_best = true;
            }
        } else {
            int base_index = pos(BaseX, BaseY);
            float base_score = calculate_action_score(g, m, base_index);

            if (base_score > action.score) {
                action.target_index = base_index;
                coos(base_index, &action.target_x, &action.target_y);
                action.score = base_score;
                action.is_best = true;
            }
        }
    }

    return action;
}

void resolve_action_conflicts(theoretical_action* actions, int num_actions) {
    for (int i = 0; i < num_actions - 1; i++) {
        for (int j = i + 1; j < num_actions; j++) {
            if (actions[j].score > actions[i].score) {
                theoretical_action temp = actions[i];
                actions[i] = actions[j];
                actions[j] = temp;
            }
        }
    }
    for (int i = 0; i < num_actions; i++) {
        if (!actions[i].is_best) continue;
        for (int j = 0; j < i; j++) {
            if (!actions[j].is_best) continue;
            if (actions[i].target_index == actions[j].target_index) {
                actions[i].is_best = false;
                break;
            }
        }
    }
}

void tick_minions_with_score(node** g) {
    int my_minions_count = count_my_minions();

    theoretical_action* all_actions = malloc(sizeof(theoretical_action) * my_minions_count);
    int action_count = 0;

    // first time calculating best actions
    for (int i = 0; i < MINIONS_LEN; i++) {
        if (MINIONS[i].owner != ID) continue;
        theoretical_action action = find_best_action(g, MINIONS[i], i);
        if (action.is_best) {
            all_actions[action_count++] = action;
        }
    }

    resolve_action_conflicts(all_actions, action_count);

    // second time to fill in the gaps
    for (int i = 0; i < action_count; i++) {
        if (!all_actions[i].is_best) {
            int minion_idx = all_actions[i].minion_pos;
            minion m = MINIONS[minion_idx];
            int x = m.x;
            int y = m.y;
            if (is_miner(m)) {
                if (m.carry < m.capacity) {
                    if (g[pos(x, y)]->t.type != RESO || (g[pos(x, y)]->t.type == RESO && g[pos(x, y)]->t.amt == 0)) {
                        coos(path_to(g, pos(x, y), best_resource_cluster_nearby(g, pos(x, y), 20), false), &x, &y);
                        if (x == m.x && y == m.y && m.carry > 0) coos(path_to(g, pos(x, y), pos(BaseX, BaseY), false), &x, &y);
                    }
                } else {
                    coos(path_to(g, pos(x, y), pos(BaseX, BaseY), false), &x, &y);
                }
            }
        }
    }

    for (int i = 0; i < action_count; i++) {
        if (!all_actions[i].is_best) continue;

        minion m = MINIONS[all_actions[i].minion_pos];
        int next_step = path_to(g, pos(m.x, m.y), all_actions[i].target_index, false);

        int next_x = m.x, next_y = m.y;
        coos(next_step, &next_x, &next_y);

        fprintf(ptr, "%d %d %d %d\n", m.x, m.y, next_x, next_y);
        targeted[next_x][next_y] = true;

        if (m.x != next_x || m.y != next_y) {
            for (int j = 0; j < 4; j++) {
                if (index_at(pos(next_x, next_y), j) != -1)
                    g[index_at(pos(next_x, next_y), j)]->links[mirror(j)] = -1;
                if (index_at(pos(m.x, m.y), j) != -1)
                    g[index_at(pos(m.x, m.y), j)]->links[mirror(j)] = pos(m.x, m.y);
            }
        }
    }

    free(all_actions);
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
    occupe = locate_minions();
    my_minion_is_here = locate_my_minions();
    trie_minions();
    if (occupe[BaseX][BaseY]) swap_minions(get_minion_index_at(BaseX, BaseY), 0);
    node** g = convert_map_to_graph(map, occupe);
    targeted = malloc(sizeof(bool*) * MAP_LEN);
    for (int i = 0; i < MAP_LEN; i++) targeted[i] = calloc(sizeof(bool), MAP_LEN);

    if (MINIONS_LEN > 0)
        tick_minions(g);
    if (MINIONS_LEN < 200)
        create_minion(g);
    fclose(ptr);
    // best_resource_cluster_nearby(g, pos(BaseX, BaseY), 20);
    // print_t(map);
    // do your things here
    free_data(map, MAP_LEN, MINIONS, randomEvents);
    return 0;
}