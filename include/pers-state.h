#pragma once

#define NAME_LENGTH 50
#define MAX_CLIENTS 10

void setupPersStateAndReadState();

struct Client
{
    uint8_t mac[6];
    uint32_t counter;
    char name[NAME_LENGTH];
    uint8_t channel;
};

class PersState
{
public:
    PersState();
    Client clients[MAX_CLIENTS];
    int otaMs = 0;
    char lastIp[16];
    int clientCount = 0;
    void saveState();
    void loadState();
};

extern PersState state;