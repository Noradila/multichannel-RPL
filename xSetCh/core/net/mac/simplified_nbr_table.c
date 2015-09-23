#include "contiki-conf.h"
#include <string.h>
#include <stdlib.h>
#include "net/mac/simplified_nbr_table.h"
#include "lib/list.h"
#include "lib/memb.h"

LIST(simplified_nbr_table_list_table);
MEMB(simplified_nbr_table_list_mem, struct simplified_nbr_table, 10);
/*---------------------------------------------------------------------------*/
uint8_t simplified_check_table(uint8_t ipA14, uint8_t ipA15, uint8_t ch) {
  struct simplified_nbr_table *nt;

  uint8_t ok = 0;
  for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    if((ipA14 == (nt->theIPAddr14)) && (ipA15 == (nt->theIPAddr15))) {
      if(ch != (nt->theCh)) {
	nt->theCh = ch;
        ok = 1;
        return ok;
      }
      else {
	ok = 1;
	return ok;
      }	
    }
  }
  return ok;
}
/*---------------------------------------------------------------------------*/
uint8_t simplified_getCh(uint8_t iP14, uint8_t iP15) {
  struct simplified_nbr_table *nt;

  for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    if((iP14 == (nt->theIPAddr14)) && (iP15 == (nt->theIPAddr15))) {
      return (nt->theCh);
    }
  }
  return;
}
/*---------------------------------------------------------------------------*/
void simplified_nbr_table_get_channel(void) {
  struct simplified_nbr_table *nt;

  printf("----------------------------\n");
  for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    printf("2 THEIPADDR %d %d THECH %d\n\n", nt->theIPAddr14, nt->theIPAddr15, nt->theCh);
  }
}
/*---------------------------------------------------------------------------*/
void simplified_nbr_table_set_channel(uint8_t theIPAddr14, uint8_t theIPAddr15, uint8_t theCh) {
  struct simplified_nbr_table *nt;
  
  /* checking is done at simplified_check_table(..) 
     simplified_nbr_table_set_channel(..) is only called after checking with 
     simplified_check_table(..) */

  nt = memb_alloc(&simplified_nbr_table_list_mem);
  if(nt != NULL) {
    nt->theIPAddr14 = theIPAddr14;
    nt->theIPAddr15 = theIPAddr15;
    nt->theCh = theCh;
    list_add(simplified_nbr_table_list_table, nt);
  }

}



