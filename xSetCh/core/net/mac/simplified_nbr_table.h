typedef struct simplified_nbr_table {
  struct simplified_nbr_table *next;
  uint8_t theIPAddr;
  uint8_t theCh;
};

//extern nbr_table *nt;

uint8_t simplified_check_table(uint8_t ipA, uint8_t ch);
void simplified_nbr_table_get_channel(void);
void simplified_nbr_table_set_channel(uint8_t theIPAddr, uint8_t theCh);
