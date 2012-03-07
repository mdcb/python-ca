#if !defined(PYTHON_CA_MODULE_ADMIN_H)
#define PYTHON_CA_MODULE_ADMIN_H

#ifndef PYTHON_CA_DEBUGLEVEL
#define PYTHON_CA_DEBUGLEVEL 1
#endif

#ifndef PYTHON_CA_ENUM_AS_STRING
#define PYTHON_CA_ENUM_AS_STRING 1
#endif

#ifndef PYTHON_CA_SYNCT
#define PYTHON_CA_SYNCT 5.0
#endif

#ifdef __cplusplus
extern "C" {
#endif

	extern int ca_module_admin_debug_level;
	extern int ca_module_admin_enum_as_string;
	extern float ca_module_admin_syncT;

	int ca_module_admin_singleton_create(void);
	void ca_module_admin_singleton_register(PyObject *);

	void allyourbases_erase(char *);
	void allyourbases_insert(char *, PyObject *);
	PyObject *allyourbases_find(char *);

#ifdef __cplusplus
}
#endif
#endif
