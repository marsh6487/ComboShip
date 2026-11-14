#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
static bool allowed;
static int mutations;
bool ComboStartupOot_PrepareContextChange(void) {return allowed;}
#include "oot_context.inc"
int main(void){
 for(int i=0;i<7;i++){
  allowed=false;mutations=0;attempts[i]();
#ifdef COMBO_BUILD
  assert(mutations==0);
#else
  assert(mutations==1);
#endif
  allowed=true;attempts[i]();
#ifdef COMBO_BUILD
  assert(mutations==1);
#else
  assert(mutations==2);
#endif
 }
 puts("PASS seven actual OoT context guards: rejected commit preserves context and retry proceeds, original guards preserved");
}
