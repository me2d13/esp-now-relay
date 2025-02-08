#pragma once

void readState();
int shouldOtaMs();
void writeState(char *lastIp, int otaMsParam);
void setupPersStateAndReadState();
char *getLastIp();