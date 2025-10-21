#include <iostream>
#include <cstdlib>
#include "../LWE.h"
#include "../FHEW.h"
#include <cassert>
#include <string.h>
using namespace std;


void SaveSecretKey(const LWE::SecretKey* LWEsk, char* f) {
  memcpy(f, LWEsk, sizeof(LWE::SecretKey));
}

LWE::SecretKey* LoadSecretKey(char* f) {
  LWE::SecretKey* LWEsk = (LWE::SecretKey*) malloc(sizeof(LWE::SecretKey));  
  assert(memcpy(LWEsk,f,sizeof(LWE::SecretKey)));
  return LWEsk;
}





unsigned long SaveEvalKey(const FHEW::EvalKey *EK, char* f) {
	return FHEW::fwrite_ek(*EK, f);
}

FHEW::EvalKey* LoadEvalKey(char* f) {
  FHEW::EvalKey* EK;
  EK = FHEW::fread_ek(f);
  return EK;
}







void SaveCipherText(const LWE::CipherText* ct, char* filepath){
  FILE * f;
  f = fopen(filepath, "wb"); // wb -write binary
  if (f == NULL){
    cerr << "Failed to open "<< filepath <<  " in Write-Binary mode .\n";
    exit(1);
  }
  cerr << "Writing CipherText to "<< filepath <<  ".\n";
  assert(fwrite(ct, sizeof(LWE::CipherText), 1, f));
  fclose(f);
}

LWE::CipherText* LoadCipherText(char* filepath) {
  FILE * f;
  f = fopen(filepath, "rb"); // wb -write binary
  if (f == NULL) {
    cerr << "Failed to open "<< filepath <<  " in Read-Binary mode.\n";
    exit(1);
  }
  cerr << "Loading CipherText from "<< filepath <<  ".\n";
  LWE::CipherText* ct = new LWE::CipherText;
  assert(fread(ct, sizeof(LWE::CipherText), 1, f));
  fclose(f);
  return ct;
}
