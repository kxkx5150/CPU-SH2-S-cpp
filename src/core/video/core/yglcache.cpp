

#include "ygl.h"
#include "yui.h"
#include "vidshared.h"

static u32 YglgetHash(u64 addr)
{
    return ((addr >> 4) & HASHSIZE);
}

static YglCacheHash *YglgetNewCash(YglTextureManager *tm)
{

    YglCacheHash *rtn;

    if (tm->CashLink_index >= HASHSIZE * 2) {
        printf("not enough cash");
        return NULL;
    }
    rtn = &tm->CashLink[tm->CashLink_index];
    tm->CashLink_index++;
    return rtn;
}

int YglIsCached(YglTextureManager *tm, u64 addr, YglCache *c)
{

    u32 hashkey;
    hashkey = YglgetHash(addr);

    if (tm->HashTable[hashkey] == NULL) {
        return 0;
    } else {
        YglCacheHash *at = tm->HashTable[hashkey];
        while (at != NULL) {
            if (at->addr == addr) {
                c->x = at->x;
                c->y = at->y;
                return 1;
            }
            at = at->next;
        }
        return 0;
    }

    return 1;
}

void YglCacheAdd(YglTextureManager *tm, u64 addr, YglCache *c)
{

    u32           hashkey;
    YglCacheHash *add;
    hashkey = YglgetHash(addr);

    if (tm->HashTable[hashkey] == NULL) {
        add                    = YglgetNewCash(tm);
        add->addr              = addr;
        add->x                 = c->x;
        add->y                 = c->y;
        add->next              = NULL;
        tm->HashTable[hashkey] = add;
    } else {
        YglCacheHash *at = tm->HashTable[hashkey];
        while (at != NULL) {
            if (at->addr == addr) {
                at->addr = addr;
                at->x    = c->x;
                at->y    = c->y;
                return;
            }
            at = at->next;
        }

        add                    = YglgetNewCash(tm);
        add->addr              = addr;
        add->x                 = c->x;
        add->y                 = c->y;
        add->next              = tm->HashTable[hashkey];
        tm->HashTable[hashkey] = add;
    }
}

void YglCacheReset(YglTextureManager *tm)
{
    memset(tm->HashTable, 0, sizeof(tm->HashTable));
    tm->CashLink_index = 0;
}
