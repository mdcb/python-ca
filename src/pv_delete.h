#if !defined(PV_DELETE_H)
#define PV_DELETE_H

#ifdef __cplusplus
extern "C" {
#endif

	void pv_delete(pvobject *);
	int pv_traverse(pvobject *, visitproc, void *);
	int pv_clear(pvobject *);

#ifdef __cplusplus
}
#endif
#endif
