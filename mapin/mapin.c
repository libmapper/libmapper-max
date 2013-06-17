//
// mapin.c
// a maxmsp and puredata external encapsulating the functionality of a
// libmapper input signal, allowing name and metadata to be set
// http://www.libmapper.org
// Joseph Malloch, IDMIL 2013
//
// This software was written in the Input Devices and Music Interaction
// Laboratory at McGill University in Montreal, and is copyright those
// found in the AUTHORS file.  It is licensed under the GNU Lesser Public
// General License version 2.1 or later.  Please see COPYING for details.
//

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
typedef struct _mapin
{
    t_object            ob;
    t_symbol            *sig_name;
    long                sig_length;
    char                sig_type;
    mapper_signal       sig_ptr;
    mapper_db_signal    sig_props;
    void                *outlet;
    t_symbol            *myobjname;
    t_atom              buffer[MAX_LIST];
} t_mapin;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapin_new(t_symbol *s, int argc, t_atom *argv);
static void mapin_free(t_mapin *x);

static t_max_err set_sig_ptr(t_mapin *x, t_object *attr, long argc, t_atom *argv);

static int atom_strcmp(t_atom *a, const char *string);
static const char *atom_get_string(t_atom *a);
static void atom_set_string(t_atom *a, const char *string);

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mapin_class;

// *********************************************************
// -(main)--------------------------------------------------
int main(void)
{
    t_class *c;
    c = class_new("mapin", (method)mapin_new, (method)mapin_free,
                  (long)sizeof(t_mapin), 0L, A_GIMME, 0);

    CLASS_ATTR_SYM(c, "sig_name", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_name);
    CLASS_ATTR_LONG(c, "sig_length", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_length);
    CLASS_ATTR_CHAR(c, "sig_type", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_type);
    CLASS_ATTR_OBJ(c, "sig_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_ptr);
    CLASS_ATTR_ACCESSORS(c, "sig_ptr", 0, set_sig_ptr);

    class_register(CLASS_BOX, c); /* CLASS_NOBOX */
    mapin_class = c;
    return 0;
}

static void mapin_usage()
{
    post("usage: [mapin <signal-name> <datatype> <opt: vectorlength>]");
}

// *********************************************************
// -(new)---------------------------------------------------
static void *mapin_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mapin *x = NULL;
    t_object *jp = NULL;

    long i = 0;

    if (argc < 2) {
        mapin_usage();
        return 0;
    }
    if ((argv)->a_type != A_SYM || (argv+1)->a_type != A_SYM) {
        mapin_usage();
        return 0;
    }

    if ((x = (t_mapin *)object_alloc(mapin_class))) {
        x->outlet = listout((t_object *)x);
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
static void mapin_free(t_mapin *x)
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
t_max_err set_sig_ptr(t_mapin *x, t_object *attr, long argc, t_atom *argv)
{
    x->sig_ptr = (mapper_signal)argv->a_w.w_obj;
    return 0;
}

static int check_sig_ptr(t_mapin *x)
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
