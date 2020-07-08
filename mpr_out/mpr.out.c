//
// mpr.out.c
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

// *********************************************************
// -(Includes)----------------------------------------------

#include "ext.h"            // standard Max include, always required
#include "ext_obex.h"       // required for new style Max object
#include "jpatcher_api.h"
#include <mpr/mpr.h>
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
typedef struct _mpr_out
{
    t_object            ob;
    t_symbol            *sig_name;
    long                sig_length;
    char                sig_type;
    t_object            *dev_obj;
    mpr_sig             sig_ptr;
    long                is_instance;
    mpr_id              instance_id;
    void                *outlet1;
    void                *outlet2;
    t_symbol            *myobjname;
    t_object            *patcher;
    t_hashtab           *ht;
    long                num_args;
    t_atom              *args;
    long                connect_state;
    int                 length;
    char                type;
} t_mpr_out;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mpr_out_new(t_symbol *s, int argc, t_atom *argv);
static void mpr_out_free(t_mpr_out *x);

static void add_to_hashtab(t_mpr_out *x, t_hashtab *ht);
static void remove_from_hashtab(t_mpr_out *x);
static t_max_err set_sig_ptr(t_mpr_out *x, t_object *attr, long argc, t_atom *argv);
static t_max_err set_dev_obj(t_mpr_out *x, t_object *attr, long argc, t_atom *argv);

static void mpr_out_loadbang(t_mpr_out *x);
static void mpr_out_int(t_mpr_out *x, long i);
static void mpr_out_float(t_mpr_out *x, double f);
static void mpr_out_list(t_mpr_out *x, t_symbol *s, int argc, t_atom *argv);
static void mpr_out_release(t_mpr_out *x);

static int atom_strcmp(t_atom *a, const char *string);
static const char *atom_get_string(t_atom *a);
static void atom_set_string(t_atom *a, const char *string);
static int atom_coerce_int(t_atom *a);
static float atom_coerce_float(t_atom *a);

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mpr_out_class;

// *********************************************************
// -(main)--------------------------------------------------
int main(void)
{
    t_class *c;
    c = class_new("mpr.out", (method)mpr_out_new, (method)mpr_out_free,
                  (long)sizeof(t_mpr_out), 0L, A_GIMME, 0);

    class_addmethod(c, (method)mpr_out_loadbang, "loadbang", 0);
    class_addmethod(c, (method)mpr_out_int, "int", A_LONG, 0);
    class_addmethod(c, (method)mpr_out_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)mpr_out_list, "list", A_GIMME, 0);
    class_addmethod(c, (method)mpr_out_release, "release", 0);
    class_addmethod(c, (method)add_to_hashtab, "add_to_hashtab", A_CANT, 0);
    class_addmethod(c, (method)remove_from_hashtab, "remove_from_hashtab", A_CANT, 0);

    CLASS_ATTR_SYM(c, "sig_name", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_name);
    CLASS_ATTR_LONG(c, "sig_length", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_length);
    CLASS_ATTR_CHAR(c, "sig_type", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_type);
    CLASS_ATTR_OBJ(c, "dev_obj", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, dev_obj);
    CLASS_ATTR_ACCESSORS(c, "dev_obj", 0, set_dev_obj);
    CLASS_ATTR_OBJ(c, "sig_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_ptr);
    CLASS_ATTR_ACCESSORS(c, "sig_ptr", 0, set_sig_ptr);

    class_register(CLASS_BOX, c); /* CLASS_NOBOX */
    mpr_out_class = c;
    return 0;
}

static void mpr_out_usage()
{
    post("usage: [mpr.out <signal-name> <datatype> <optional: vectorlength>]");
}

// *********************************************************
// -(new)---------------------------------------------------
static void *mpr_out_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mpr_out *x = NULL;

    long i = 0;

    if (argc < 2) {
        mpr_out_usage();
        return 0;
    }
    if ((argv)->a_type != A_SYM || (argv+1)->a_type != A_SYM) {
        mpr_out_usage();
        return 0;
    }

    if ((x = (t_mpr_out *)object_alloc(mpr_out_class))) {
        x->outlet1 = listout((t_object *)x);
        x->outlet2 = listout((t_object *)x);

        x->sig_name = atom_getsym(argv);

        char *temp = atom_getsym(argv+1)->s_name;
        x->sig_type = temp[0];
        if (x->sig_type != 'i' && x->sig_type != 'f')
            return 0;

        x->sig_ptr = 0;
        x->length = 0;
        x->instance_id = 0;
        x->is_instance = 0;
        x->connect_state = 0;

        if (argc >= 3 && (argv+2)->a_type == A_LONG) {
            x->sig_length = atom_getlong(argv+2);
            if (x->sig_length > 100) {
                post("vector lengths > 100 not currently supported.");
                return 0;
            }
            i = 3;
        }
        else {
            x->sig_length = 1;
            i = 2;
        }

        // we need to cache any arguments to add later
        x->args = 0;
        x->num_args = argc - i;
        if (x->num_args) {
            long alloced = 0;
            char result = 0;
            atom_alloc_array(x->num_args, &alloced, &x->args, &result);
            if (!result || !alloced)
                return 0;
            sysmem_copyptr(argv+i, x->args, x->num_args * sizeof(t_atom));
        }

        // cache the registered name so we can remove self from hashtab later
        x = object_register(CLASS_BOX, x->myobjname = symbol_unique(), x);

        x->patcher = (t_object *)gensym("#P")->s_thing;
        mpr_out_loadbang(x);
    }
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
static void mpr_out_free(t_mpr_out *x)
{
    remove_from_hashtab(x);
    if (x->args)
        sysmem_freeptr(x->args);
}

void mpr_out_loadbang(t_mpr_out *x)
{
    t_hashtab *ht;

    if (!x->patcher)
        return;

    t_object *patcher = x->patcher;
    while (patcher) {
        object_obex_lookup(patcher, gensym("mprhash"), (t_object **)&ht);
        if (ht) {
            add_to_hashtab(x, ht);
            break;
        }
        patcher = jpatcher_get_parentpatcher(patcher);
    }
}

void add_to_hashtab(t_mpr_out *x, t_hashtab *ht)
{
    if (x->connect_state) {
        // already registered
        return;
    }

    // store self in the hashtab. IMPORTANT: set the OBJ_FLAG_REF flag so that the
    // hashtab knows not to free us when it is freed.
    hashtab_storeflags(ht, x->myobjname, (t_object *)x, OBJ_FLAG_REF);
    x->ht = ht;
    x->connect_state = 1;
}

void remove_from_hashtab(t_mpr_out *x)
{
    if (x->ht) {
        hashtab_chuckkey(x->ht, x->myobjname);
        x->ht = NULL;
    }
    x->dev_obj = 0;
    x->sig_ptr = 0;
    x->length = 0;
    x->connect_state = 0;
}

// *********************************************************
// -(parse props from object arguments)---------------------
void parse_extra_properties(t_mpr_out *x)
{
    int i;
    // add other declared properties
    for (i = 0; i < x->num_args; i++) {
        if (i > x->num_args - 2) // need 2 arguments for key and value
            break;
        else if ((x->args+i)->a_type != A_SYM) {
            i++;
            continue;
        }
        else if ((atom_strcmp(x->args+i, "@name") == 0) ||
            (atom_strcmp(x->args+i, "@type") == 0) ||
            (atom_strcmp(x->args+i, "@length") == 0)){
            i++;
            continue;
        }
        else if (atom_strcmp(x->args+i, "@instance") == 0) {
            if ((x->args+i+1)->a_type == A_SYM &&
                atom_strcmp(x->args+i+1, "polyindex") == 0) {
                /* Check if object is embedded in a poly~ object - if so,
                 * retrieve the index and use as instance id. */
                t_object *patcher = NULL;
                t_max_err err = object_obex_lookup(x, gensym("#P"), &patcher);
                if (err == MAX_ERR_NONE) {
                    t_object *assoc = NULL;
                    object_method(patcher, gensym("getassoc"), &assoc);
                    if (assoc) {
                        method m = zgetfn(assoc, gensym("getindex"));
                        if (m) {
                            x->instance_id = (long)(*m)(assoc, patcher);
                        }
                        else
                            continue;
                    }
                    else
                        continue;
                }
                else
                    continue;
            }
            else if ((x->args+i+1)->a_type == A_LONG) {
                x->instance_id = atom_getlong(x->args+i+1);
            }
            else {
                continue;
            }
            /* Remove the default signal instance (0) if it exists. Since the user
             * may have properly added an instance 0, we will check for user_data. */
            void *data = mpr_sig_get_inst_data(x->sig_ptr, 0);
            if (!data)
                mpr_sig_remove_inst(x->sig_ptr, 0);

            x->is_instance = 1;
            i++;
            mpr_sig_reserve_inst(x->sig_ptr, 1, &x->instance_id, (void **)&x);
        }
        else if (atom_strcmp(x->args+i, "@minimum") == 0 ||
                 atom_strcmp(x->args+i, "@min") == 0) {
            // check number of arguments
            int length = 1, scalar_min = 0;
            char type;
            while (length + i < x->num_args) {
                type = (x->args+i+length)->a_type;
                if (type == A_SYM && atom_get_string(x->args+i+length)[0] == '@') {
                    // reached next property name
                    break;
                }
                if (type != A_LONG && type != A_FLOAT) {
                    length = 0;
                    break;
                }
                length++;
            }
            length--;
            if (length != x->sig_length) {
                // we will allow using scalars to set min of entire vector
                if (length == 1)
                    scalar_min = 1;
                else {
                    post("'%s' property for signal %s requires %i arguments! (got %i)",
                         atom_get_string(x->args+i), x->sig_name->s_name,
                         x->sig_length, length);
                    continue;
                }
            }

            ++i;
            int j;
            switch (x->sig_type) {
                case 'i': {
                    int val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_int(x->args + i);
                    }
                    if (scalar_min) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_MIN, NULL, x->sig_length,
                                     MPR_INT32, val, 1);
                    i--;
                    break;
                }
                case 'f':
                case 'd': {
                    float val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_float(x->args + i);
                    }
                    if (scalar_min) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_MIN, NULL, x->sig_length,
                                     MPR_FLT, val, 1);
                    i--;
                    break;
                }
                default:
                    break;
            }
        }
        else if (atom_strcmp(x->args+i, "@maximum") == 0 ||
                 atom_strcmp(x->args+i, "@max") == 0) {
            // check number of arguments
            int length = 1, scalar_max = 0;
            char type;
            while (length + i < x->num_args) {
                type = (x->args+i+length)->a_type;
                if (type == A_SYM && atom_get_string(x->args+i+length)[0] == '@') {
                    // reached next property name
                    break;
                }
                if (type != A_LONG && type != A_FLOAT) {
                    length = 0;
                    break;
                }
                length++;
            }
            length--;
            if (length != x->sig_length) {
                // we will allow using scalars to set max of entire vector
                if (length == 1)
                    scalar_max = 1;
                else {
                    post("'%s' property for signal %s requires %i arguments! (got %i)",
                         atom_get_string(x->args+i), x->sig_name->s_name,
                         x->sig_length, length);
                    continue;
                }
            }

            ++i;
            int j;
            switch (x->sig_type) {
                case 'i': {
                    int val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_int(x->args + i);
                    }
                    if (scalar_max) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_MAX, NULL, x->sig_length,
                                     MPR_INT32, val, 1);
                    i--;
                    break;
                }
                case 'f':
                case 'd': {
                    float val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_float(x->args + i);
                    }
                    if (scalar_max) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_MAX, NULL, x->sig_length,
                                     MPR_FLT, val, 1);
                    i--;
                    break;
                }
                default:
                    break;
            }
        }
        else if (atom_get_string(x->args+i)[0] == '@') {
            // TODO: enable vector property values
            switch ((x->args+i+1)->a_type) {
                case A_SYM: {
                    const char *value = atom_get_string(x->args+i+1);
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_UNKNOWN,
                                     atom_get_string(x->args+i)+1, 1, MPR_STR,
                                     value, 1);
                    i++;
                    break;
                }
                case A_FLOAT:
                {
                    float value = atom_getfloat(x->args+i+1);
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_UNKNOWN,
                                     atom_get_string(x->args+i)+1, 1, MPR_FLT,
                                     &value, 1);
                    i++;
                    break;
                }
                case A_LONG:
                {
                    int value = atom_getlong(x->args+i+1);
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_UNKNOWN,
                                     atom_get_string(x->args+i)+1, 1, MPR_INT32,
                                     &value, 1);
                    i++;
                    break;
                }
                default:
                    break;
            }
        }
    }
}

// *********************************************************
// -(set the device pointer)--------------------------------
t_max_err set_dev_obj(t_mpr_out *x, t_object *attr, long argc, t_atom *argv)
{
    x->dev_obj = (t_object *)argv->a_w.w_obj;
    return 0;
}

// *********************************************************
// -(set the signal pointer)--------------------------------
t_max_err set_sig_ptr(t_mpr_out *x, t_object *attr, long argc, t_atom *argv)
{
    x->sig_ptr = (mpr_sig)argv->a_w.w_obj;
    if (x->sig_ptr)
        parse_extra_properties(x);
    return 0;
}

// *********************************************************
// -(check if device and signal pointers have been set)-----
static int check_ptrs(t_mpr_out *x)
{
    if (!x || !x->dev_obj || !x->sig_ptr) {
        return 1;
    }
    else if (!x->length) {
        x->length = mpr_obj_get_prop_as_int32(x->sig_ptr, MPR_PROP_LEN, NULL);
        x->type = mpr_obj_get_prop_as_int32(x->sig_ptr, MPR_PROP_TYPE, NULL);
    }
    return 0;
}

// *********************************************************
// -(int input)---------------------------------------------
static void mpr_out_int(t_mpr_out *x, long l)
{
    if (check_ptrs(x))
        return;

    mpr_sig_set_value(x->sig_ptr, x->instance_id, 1, MPR_INT32, &l);
}

// *********************************************************
// -(float input)-------------------------------------------
static void mpr_out_float(t_mpr_out *x, double d)
{
    if (check_ptrs(x))
        return;

    mpr_sig_set_value(x->sig_ptr, x->instance_id, 1, MPR_DBL, &d);
}

// *********************************************************
// -(list input)--------------------------------------------
static void mpr_out_list(t_mpr_out *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, count;
    void *value = 0;

    if (check_ptrs(x) || !argc)
        return;

    if (argc < x->length || (argc % x->length) != 0) {
        object_post((t_object *)x, "Illegal list length (expected factor of %i)",
                    x->length);
        return;
    }
    count = argc / x->length;

    if (x->type == 'i') {
        int payload[argc];
        value = &payload;
        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_FLOAT)
                payload[i] = (int)atom_getfloat(argv+i);
            else if ((argv+i)->a_type == A_LONG)
                payload[i] = (int)atom_getlong(argv+i);
            else {
                object_post((t_object *)x, "Illegal data type in list!");
                return;
            }
        }
        //update signal
        mpr_sig_set_value(x->sig_ptr, x->instance_id, argc, MPR_INT32, value);
    }
    else if (x->type == 'f') {
        float payload[argc];
        value = &payload;
        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_FLOAT)
                payload[i] = atom_getfloat(argv+i);
            else if ((argv+i)->a_type == A_LONG)
                payload[i] = (float)atom_getlong(argv+i);
            else {
                object_post((t_object *)x, "Illegal data type in list!");
                return;
            }
        }
        //update signal
        mpr_sig_set_value(x->sig_ptr, x->instance_id, argc, MPR_FLT, value);
    }
}

// *********************************************************
// -(release instance)--------------------------------------
static void mpr_out_release(t_mpr_out *x)
{
    if (check_ptrs(x) || !x->is_instance)
        return;

    mpr_sig_release_inst(x->sig_ptr, x->instance_id);
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

static int atom_coerce_int(t_atom *a)
{
    if (a->a_type == A_LONG)
        return (int)atom_getlong(a);
    else if (a->a_type == A_FLOAT)
        return (int)atom_getfloat(a);
    else
        return 0;
}

static float atom_coerce_float(t_atom *a)
{
    if (a->a_type == A_LONG)
        return (float)atom_getlong(a);
    else if (a->a_type == A_FLOAT)
        return atom_getfloat(a);
    else
        return 0.f;
}
