#include <stdio.h>
#include "main.h"
#include "keyValStore.h"
#include "sub.h"

int main(void) {
    char result[256];

    // GET key1, key noch nicht vorhanden
    if (get("key1", result) == 0)
        printf("GET:key1:%s\n", result);
    else
        printf("GET:key1:key_nonexistent\n");

    // PUT key1 value1
    put("key1", "value1");
    printf("PUT:key1:value1\n");
    // PUT key2 value2
    put("key2", "value2");
    printf("PUT:key2:value2\n");
    // PUT key1 value3 (überschreibt)
    put("key1", "value3");
    printf("PUT:key1:value3\n");

    // DEL key2
    if (del("key2") == 0)
        printf("DEL:key2:key_deleted\n");
    else
        printf("DEL:key2:key_nonexistent\n");
    // DEL key, key schon gelöscht
    if (del("key2") == 0)
        printf("DEL:key2:key_deleted\n");
    else
        printf("DEL:key2:key_nonexistent\n");

    // GET key1, sollte value3 sein
    if (get("key1", result) == 0)
        printf("GET:key1:%s\n", result);
    else
        printf("GET:key1:key_nonexistent\n");

    printf("QUIT\n");

    return 0;
}
