typedef struct simplified_nbr_table {
  struct simplified_nbr_table *next;
  uint8_t theIPAddr14;
  uint8_t theIPAddr15;
  uint8_t theCh;
};

uint8_t simplified_check_table(uint8_t ipA14, uint8_t ipA15, uint8_t ch);
uint8_t simplified_getCh(uint8_t iP14, uint8_t iP15);
void simplified_nbr_table_get_channel(void);
void simplified_nbr_table_set_channel(uint8_t theIPAddr14, uint8_t theIPAddr15, uint8_t theCh);
