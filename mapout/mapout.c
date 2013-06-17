//
// mapout.c
// a maxmsp and puredata external encapsulating the functionality of a
// libmapper output signal, allowing name and metadata to be set
// http://www.libmapper.org
// Joseph Malloch, IDMIL 2013
//
// This software was written in the Input Devices and Music Interaction
// Laboratory at McGill University in Montreal, and is copyright those
// found in the AUTHORS file.  It is licensed under the GNU Lesser Public
// General License version 2.1 or later.  Please see COPYING for details.
//

/* TODO:
 * on startup:
    scan patcher and subpatchers for mapin and mapout objects
    register them as libmapper signals
 */

// *********************************************************
// -(Includes)----------------------------------------------

#include "ext.h"            // standard Max include, always required
#include "ext_obex.h"       // required for new style Max object
#include "jpatcher_api.h"
#include <mapper/mapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <lo/lo.h>
#ifndef WIN32
  #include <arpa/inet.h>
#endif

#include <unistd.h>

#define MAX_LIST 256

// *********************************************************
// -(object struct)-----------------------------------------
typedef struct _mapout
{
    t_object            ob;
    t_symbol            *sig_name;
    long                sig_length;
    char                sig_type;
    mapper_signal       sig_ptr;
    mapper_db_signal    sig_props;
    t_symbol            *myobjname;
    mapper_timetag_t    timetag;
} t_mapout;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapout_new(t_symbol *s, int argc, t_atom *argv);
static void mapout_free(t_mapout *x);

static t_max_err set_sig_ptr(t_mapout *x, t_object *attr, long argc, t_atom *argv);

static void mapout_int(t_mapout *x, long i);
static void mapout_float(t_mapout *x, double f);
static void mapout_list(t_mapout *x, t_symbol *s, int argc, t_atom *argv);

static int atom_strcmp(t_atom *a, const char *string);
static const char *atom_get_string(t_atom *a);
static void atom_set_string(t_atom *a, const char *string);

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mapout_class;

// *********************************************************
// -(main)--------------------------------------------------
int main(void)
{
    t_class *c;
    c = class_new("mapout", (method)mapout_new, (method)mapout_free,
                  (long)sizeof(t_mapout), 0L, A_GIMME, 0);

    class_addmethod(c, (method)mapout_int, "int", A_LONG, 0);
    class_addmethod(c, (method)mapout_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)mapout_list, "list", A_GIMME, 0);

    CLASS_ATTR_SYM(c, "sig_name", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_name);
    CLASS_ATTR_LONG(c, "sig_length", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_length);
    CLASS_ATTR_CHAR(c, "sig_type", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_type);
    CLASS_ATTR_OBJ(c, "sig_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_ptr);
    CLASS_ATTR_ACCESSORS(c, "sig_ptr", 0, set_sig_ptr);

    class_register(CLASS_BOX, c); /* CLASS_NOBOX */
    mapout_class = c;
    return 0;
}

static void mapout_usage()
{
    post("usage: [mapout <signal-name> <datatype> <optional: vectorlength>]");
}

// *********************************************************
// -(new)---------------------------------------------------
static void *mapout_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mapout *x = NULL;
    t_object *jp = NULL;

    long i = 0;

    if (argc < 2) {
        mapout_usage();
        return 0;
    }
    if ((argv)->a_type != A_SYM || (argv+1)->a_type != A_SYM) {
        mapout_usage();
        return 0;
    }

    if ((x = (t_mapout *)object_alloc(mapout_class))) {
        x->sig_name = gensym(atom_getsym(argv)->s_name);
        char *temp = atom_getsym(argv+1)->s_name;
        x->sig_type = temp[0];
        x->sig_ptr = 0;
        x->sig_props = 0;
        
        if (argc >= 3 && (argv+2)->a_type == A_LONG) {
            x->sig_length = atom_getlong(argv+2);
            i = 3;
        }
        else {
            x->sig_length = 1;
            i = 2;
        }

        jp = (t_object *)gensym("#P")->s_thing;
        if (jp) {
            t_hashtab *ht;

            // look in the jpatcher's obex for an object called "mapperhash"
			object_obex_lookup(jp, gensym("mapperhash"), (t_object **)&ht);
			if (!ht) {
				// it's not there? create it.
				ht = hashtab_new(0);
				// objects stored in the obex will be freed when the obex's owner is freed
				// in this case, when the patcher object is freed. so we don't need to
				// manage the memory associated with the "mapperhash".
				object_obex_store(jp, gensym("mapperhash"), (t_object *)ht);
			}
			// cache the registered name so we can remove self from hashtab
			x = object_register(CLASS_BOX, x->myobjname = symbol_unique(), x);
			// store self in the hashtab. IMPORTANT: set the OBJ_FLAG_REF flag so that the
			// hashtab knows not to free us when it is freed.
			hashtab_storeflags(ht, x->myobjname, (t_object *)x, OBJ_FLAG_REF);
        }

        if (!x->sig_ptr) {
            post("error: mapout did not get sig_ptr");
        }
        else {
            // add other declared properties
            for (; i < argc; i++) {
                if (i > argc - 2) // need 2 arguments for key and value
                    break;
                if ((atom_strcmp(argv+i, "@name") == 0) ||
                    (atom_strcmp(argv+i, "@type") == 0) ||
                    (atom_strcmp(argv+i, "@length") == 0)){
                    i++;
                    continue;
                }
                else if (atom_get_string(argv+i)[0] == '@') {
                    switch ((argv+i+1)->a_type) {
                        case A_SYM: {
                            const char *value = atom_get_string(argv+i+1);
                            msig_set_property(x->sig_ptr, atom_get_string(argv+i)+1,
                                              's', (lo_arg *)value);
                            i++;
                            break;
                        }
                        case A_FLOAT:
                        {
                            float value = atom_getfloat(argv+i+1);
                            msig_set_property(x->sig_ptr, atom_get_string(argv+i)+1,
                                              'f', (lo_arg *)&value);
                            i++;
                            break;
                        }
                        case A_LONG:
                        {
                            int value = atom_getlong(argv+i+1);
                            msig_set_property(x->sig_ptr, atom_get_string(argv+i)+1,
                                              'i', (lo_arg *)&value);
                            i++;
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
            
    }
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
static void mapout_free(t_mapout *x)
{
    t_object *jp;

    // get the object's patcher
	object_obex_lookup(x, gensym("#P"), &jp);
	if (jp) {
		t_hashtab *ht;

		// find the mapperhash
		object_obex_lookup(jp, gensym("mapperhash"), (t_object **)&ht);
		if (ht) {
			hashtab_chuckkey(ht, x->myobjname); // remove self from hashtab
		}
	}
}

// *********************************************************
// -(set the signal pointer)--------------------------------
t_max_err set_sig_ptr(t_mapout *x, t_object *attr, long argc, t_atom *argv)
{
    x->sig_ptr = (mapper_signal)argv->a_w.w_obj;
    return 0;
}

static int check_sig_ptr(t_mapout *x)
{
    if (!x || !x->sig_ptr) {
        return 1;
    }
    else if (!x->sig_props) {
        x->sig_props = msig_properties(x->sig_ptr);
    }
    return 0;
}

// *********************************************************
// -(int input)---------------------------------------------
static void mapout_int(t_mapout *x, long i)
{
    if (check_sig_ptr(x))
        return;
    if (x->sig_props->length != 1)
        return;
    if (x->sig_props->type == 'i')
        msig_update(x->sig_ptr, &i, 1, MAPPER_NOW);
    else if (x->sig_props->type == 'f') {
        float f = (float)i;
        msig_update(x->sig_ptr, &f, 1, MAPPER_NOW);
    }
}

// *********************************************************
// -(float input)-------------------------------------------
static void mapout_float(t_mapout *x, double d)
{
    if (check_sig_ptr(x))
        return;
    if (x->sig_props->length != 1)
        return;
    if (x->sig_props->type == 'f') {
        float f = (float)d;
        msig_update(x->sig_ptr, &f, 1, MAPPER_NOW);
    }
    else if (x->sig_props->type == 'i') {
        int i = (int)d;
        msig_update(x->sig_ptr, &i, 1, MAPPER_NOW);
    }
}

// *********************************************************
// -(list input)--------------------------------------------
static void mapout_list(t_mapout *x, t_symbol *s, int argc, t_atom *argv)
{
    if (check_sig_ptr(x))
        return;

    int i = 0, j = 0, id = -1;
    if (argc) {
        if (argc == 2 && (argv + 1)->a_type == A_SYM) {
            if ((argv)->a_type != A_LONG)
                return;
            id = (int)atom_getlong(argv);
            if (strcmp(atom_getsym(argv+1)->s_name, "release") == 0)
                msig_release_instance(x->sig_ptr, id, x->timetag);
        }
        else if (argc == x->sig_props->length + 1) {
            // Special case: signal value may be preceded by instance number
            if ((argv)->a_type == A_LONG) {
                id = (int)atom_getlong(argv);
                j = 1;
            }
            else {
                post("Instance ID is not int!");
                return;
            }
        }
        // TODO: handle multi-count updates
        else if (argc != x->sig_props->length)
            return;
        
        if (x->sig_props->type == 'i') {
            int payload[x->sig_props->length];
            for (i = 0; i < argc; i++) {
                if ((argv + i + j)->a_type == A_FLOAT)
                    payload[i] = (int)atom_getfloat(argv + i + j);
                else if ((argv + i + j)->a_type == A_LONG)
                    payload[i] = (int)atom_getlong(argv + i + j);
            }
            //update signal
            if (id == -1) {
                msig_update(x->sig_ptr, payload, 1, x->timetag);
            }
            else {
                msig_update_instance(x->sig_ptr, id, payload, 1, x->timetag);
            }
        }
        else if (x->sig_props->type == 'f') {
            float payload[x->sig_props->length];
            for (i = 0; i < argc; i++) {
                if ((argv + i + j)->a_type == A_FLOAT)
                    payload[i] = atom_getfloat(argv + i + j);
                else if ((argv + i + j)->a_type == A_LONG)
                    payload[i] = (float)atom_getlong(argv + i + j);
            }
            //update signal
            if (id == -1) {
                msig_update(x->sig_ptr, payload, 1, x->timetag);
            }
            else {
                msig_update_instance(x->sig_ptr, id, payload, 1, x->timetag);
            }
        }
    }
}

// *********************************************************
// some helper functions

static int atom_strcmp(t_atom *a, const char *string)
{
    if (a->a_type != A_SYM || !string)
        return 1;
    return strcmp(atom_getsym(a)->s_name, string);
}

static const char *atom_get_string(t_atom *a)
{
    return atom_getsym(a)->s_name;
}

static void atom_set_string(t_atom *a, const char *string)
{
    atom_setsym(a, gensym((char *)string));
}
