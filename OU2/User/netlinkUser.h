//
// Created by hasse on 2/25/19.
//

#ifndef OU2_NETLINKUSER_H
#define OU2_NETLINKUSER_H

#include "../Resources/PDU_user.h"



size_t strnlen(const char *s, size_t maxlen);

void *test_rhashtable(data arg);
void delete_rhashtable(char* key);
void get_rhashtable(char* key);
void init_rhashtable();
void insert_rhashtable(char* key);



#endif //OU2_NETLINKUSER_H
