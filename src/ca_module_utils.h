#if !defined(WAVEFORMCA_H)
#define WAVEFORMCA_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * struct for time waveform
 *******************************************************************/
typedef struct dbr_time_buff
{
  dbr_short_t status; /* status of value */
  dbr_short_t severity; /* severity of alarm */
  TS_STAMP stamp; /* time stamp */
  void * value; /* pointer to current value */
} dbr_time_buff;

#define dbr_wavevalue_ptr(PDBR, DBR_TYPE, index) \
((void *)(((char *)PDBR) + dbr_value_offset[DBR_TYPE] + index*dbr_value_size[DBR_TYPE] ))

/*******************************************************************
 * union for all plain type
 *******************************************************************/
typedef union dbr_plaintype
{
  dbr_string_t s;
  dbr_enum_t e;
  dbr_char_t c;
  dbr_short_t i;
  dbr_long_t l;
  dbr_float_t f;
  dbr_double_t d;
} dbr_plaintype;

/*******************************************************************
 * prototype
 *******************************************************************/
void ca_module_utilsextract(dbr_time_buff *, chtype, long,
                            dbr_plaintype *);
void ca_module_utilsinject(dbr_time_buff *, chtype, long,
                           dbr_plaintype *);
void * ca_module_utilsbuffpointer(dbr_time_buff *, chtype);
void plaintypePrint(chtype, dbr_plaintype *);
void plaintypeFprint(FILE *, chtype, dbr_plaintype *);

#ifdef __cplusplus
}
#endif
#endif
