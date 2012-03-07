#include "ca_module.h"
#include "ca_module_utils.h"

static char const rcsid[] =
    "$Id: ca_module_utils.c 18830 2009-03-19 12:44:55Z mdcb $";

/*******************************************************************
 * NAME
 * ca_module_utilsextract  -  extract index from waveform
 *
 * DESCRIPTION
 *	extract value at given index from waveform
 *
 * WARNING
 *	
 * PERSONNEL:
 *  	Matthieu Bec, Software Group, ING.  mdcb@ing.iac.es
 * 	
 * HISTORY:
 *  	30/06/99 - first documented version
 *
 *******************************************************************/
void
ca_module_utilsextract(dbr_time_buff * buff, chtype type, long index,
		       dbr_plaintype * pval)
{

	if (!dbr_type_is_plain(type)) {
		printf("ca_module_utilsextract TYPE not supported %ld",
		       (long)type);
		return;
	}

	if (buff == NULL) {
		printf("no value!");
		return;
	}

	switch (type) {
	case DBR_STRING:
		strcpy(pval->s,
		       dbr_wavevalue_ptr(buff, dbf_type_to_DBR_TIME(type),
					 index));
		break;
	default:
		memcpy(pval,
		       dbr_wavevalue_ptr(buff, dbf_type_to_DBR_TIME(type),
					 index), dbr_value_size[type]);
		break;
	}
}

/*******************************************************************
 * NAME
 *  ca_module_utilsinject - inject index into waveform
 *
 * DESCRIPTION
 *	inject index into waveform
 *
 * WARNING
 *	
 * PERSONNEL:
 *  	Matthieu Bec, Software Group, ING.  mdcb@ing.iac.es
 * 	
 * HISTORY:
 *  	30/06/99 - first documented version
 *  	01/07/99 - memcpy replace switch statements
 *
 *******************************************************************/
void
ca_module_utilsinject(dbr_time_buff * buff, chtype type, long index,
		      dbr_plaintype * pval)
{

	if (!dbr_type_is_plain(type)) {
		printf("ca_module_utilsinject TYPE not supported %ld",
		       (long)type);
		return;
	}

	switch (type) {
	case DBR_STRING:
		strcpy(dbr_wavevalue_ptr
		       (buff, dbf_type_to_DBR_TIME(type), index), pval->s);
		break;
	default:
		memcpy(dbr_wavevalue_ptr
		       (buff, dbf_type_to_DBR_TIME(type), index), pval,
		       dbr_value_size[type]);
		break;
	}
}

/*******************************************************************
 * NAME
 *  plaintypeFprint - print plaintype to FILE *
 *
 * DESCRIPTION
 *	print plaintype to FILE *
 *
 * WARNING
 *	
 * PERSONNEL:
 *  	Matthieu Bec, Software Group, ING.  mdcb@ing.iac.es
 * 	
 * HISTORY:
 *  	30/06/99 - first documented version
 *
 *******************************************************************/
void plaintypeFprint(FILE * fp, chtype type, dbr_plaintype * pval)
{

	switch (type) {
	case DBR_STRING:
		fprintf(fp, "%s", pval->s);
		break;
	case DBR_SHORT:
		/* case DBR_INT: */
		fprintf(fp, "%d", pval->i);
		break;
	case DBR_FLOAT:
		fprintf(fp, "%f", pval->f);
		break;
	case DBR_ENUM:
		fprintf(fp, "%d", pval->e);
		break;
	case DBR_CHAR:
		fprintf(fp, "%c", pval->c);
		break;
	case DBR_LONG:
		fprintf(fp, "%d", pval->l);
		break;
	case DBR_DOUBLE:
		fprintf(fp, "%f", pval->d);
		break;
	default:
		fprintf(fp, "plaintypePrint TYPE not supported %ld",
			(long)type);
		break;
	}
}

/*******************************************************************
 * NAME
 *  plaintypeFprint - print plaintype to stderr
 *
 * DESCRIPTION
 *	print plaintype to stderr
 *
 * WARNING
 *	
 * PERSONNEL:
 *  	Matthieu Bec, Software Group, ING.  mdcb@ing.iac.es
 * 	
 * HISTORY:
 *  	30/06/99 - first documented version
 *
 *******************************************************************/
void plaintypePrint(chtype type, dbr_plaintype * pval)
{
	plaintypeFprint(stderr, type, pval);
}
