#include "contiki-conf.h"
#include <string.h>
#include <stdlib.h>
#include "net/mac/simplified_nbr_table.h"
#include "lib/list.h"
#include "lib/memb.h"

LIST(simplified_nbr_table_list_table);
MEMB(simplified_nbr_table_list_mem, struct simplified_nbr_table, 10);
//MEMB(simplified_nbr_table_list_mem, struct simplified_nbr_table, 10);


uint8_t simplified_check_table(uint8_t ipA, uint8_t ch) {
  struct simplified_nbr_table *nt;

  uint8_t ok = 0;
  for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    if(ipA == (nt->theIPAddr)) {
      //printf("SAME IPADDR\n\n");
      if(ch != (nt->theCh)) {
	//printf("CH IS GOING TO CHANGED FROM %d\n\n", nt->theCh);
	nt->theCh = ch;
	//printf("CH IS CHANGED TO %d\n\n", nt->theCh);
        ok = 1;
        return ok;
      }
      else {
	//printf("NO CHANGES!!!\n\n");
	ok = 1;
	return ok;
      }	
    }
  }
  return ok;
}

/*---------------------------------------------------------------------------*/
uint8_t simplified_getCh(uint8_t iP) {
  struct simplified_nbr_table *nt;

  for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    if(iP == (nt->theIPAddr)) {
      //printf("IP SAME, CH IS %d\n\n", nt->theCh);
      return (nt->theCh);
    }
  }
  return;
}
/*---------------------------------------------------------------------------*/
void simplified_nbr_table_get_channel(void) {
//nbr_table *xx;
//  printf("IN SIMPLIFIED NBR TABLE GET CHANNEL %d\n\n\n", xx->theIPAddr);

  struct simplified_nbr_table *nt;

printf("----------------------------\n");
  for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    printf("2 THEIPADDR %d THECH %d\n\n", nt->theIPAddr, nt->theCh);
  }
printf("//////////////////////////////////\n");

}
/*---------------------------------------------------------------------------*/
void simplified_nbr_table_set_channel(uint8_t theIPAddr, uint8_t theCh) {
  struct simplified_nbr_table *nt;
  
  /*for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    printf("THEIPADDR %d THECH %d\n\n", nt->theIPAddr, nt->theCh);
  }*/


  /*for(nt = list_head(simplified_nbr_table_list_table); nt != NULL; nt = nt->next) {
    if(theIPAddr == (nt->theIPAddr) && theCh != (nt->theCh)) {
printf("IPADDR %d == %d NT->IPADDR\n\n", theIPAddr, nt->theIPAddr);
      //if(theCh != (nt->theCh)) {
printf("THECH %d == %d NT->THECH\n\n", theCh, nt->theCh);
        nt->theCh = theCh;
	//break;
	return;
     // }
    }
  }*/

  nt = memb_alloc(&simplified_nbr_table_list_mem);
  if(nt != NULL) {
printf("added IPAddr %d theCh %d\n\n", theIPAddr, theCh);
    nt->theIPAddr = theIPAddr;
    nt->theCh = theCh;
    list_add(simplified_nbr_table_list_table, nt);
  }

}



