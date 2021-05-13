//
// mpr.out.c
// a maxmsp and puredata external encapsulating the functionality of a
// libmapper output signal, allowing name and metadata to be set
// http://www.libmapper.org
// Joseph Malloch, 2013-2020
//
// This software was written in the Graphics and Experiential Media (GEM) Lab at Dalhousie
// University in Halifax and the Input Devices and Music Interaction Laboratory (IDMIL) at McGill
// University in Montreal, and is copyright those found in the AUTHORS file.  It is licensed under
// the GNU Lesser Public General License version 2.1 or later.  Please see COPYING for details.
//

// *********************************************************
// -(Includes)----------------------------------------------
#ifdef WIN32
#define _WINSOCKAPI_ //for winsock1/2 conflicts
#endif

#include "ext.h"            // standard Max include, always required
#include "ext_obex.h"       // required for new style Max object
#include "ext_critical.h"
#include "jpatcher_api.h"
#include <mapper/mapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
  #include <arpa/inet.h>
  #include <unistd.h>
#endif


#define MAX_LIST 256

// *********************************************************
// -(object struct)-----------------------------------------
typedef struct _mpr_out
{
    t_object            ob;
    void                *outlet;
    t_symbol            *sig_name;
    long                sig_length;
    char                sig_type;
    mpr_dev             dev_obj;
    mpr_sig             sig_ptr;
    long                is_instanced;
    mpr_id              instance_id;
    t_symbol            *myobjname;
    t_object            *patcher;
    t_hashtab           *ht;
    t_atomarray         *args;
    long                connect_state;
    int                 length;
    char                type;
} t_mpr_out;

typedef struct _mpr_ptrs
{
    int                 num_objs;
    t_object            **objs;
} t_mpr_ptrs;

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
static void mpr_out_anything(t_mpr_out *x, t_symbol *s, int argc, t_atom *argv);

t_max_err mpr_out_instance_get(t_mpr_out *x, t_object *attr, long *argc, t_atom **argv);
t_max_err mpr_out_instance_set(t_mpr_out *x, t_object *attr, long argc, t_atom *argv);

static int atom_strcmp(t_atom *a, const char *string);
static const char *atom_get_string(t_atom *a);
static void atom_set_string(t_atom *a, const char *string);
static int atom_coerce_int(t_atom *a);
static float atom_coerce_float(t_atom *a);

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mpr_out_class;

// *********************************************************

#ifdef WIN32
void ext_main(void *r)
{
    main();
}
#endif

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
    class_addmethod(c, (method)mpr_out_anything, "anything", A_GIMME, 0);
    class_addmethod(c, (method)add_to_hashtab, "add_to_hashtab", A_CANT, 0);
    class_addmethod(c, (method)remove_from_hashtab, "remove_from_hashtab", A_CANT, 0);

    CLASS_ATTR_SYM(c, "sig_name", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_name);
    CLASS_ATTR_LONG(c, "sig_length", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_length);
    CLASS_ATTR_CHAR(c, "sig_type", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_type);
    CLASS_ATTR_OBJ(c, "dev_obj", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, dev_obj);
    CLASS_ATTR_ACCESSORS(c, "dev_obj", 0, set_dev_obj);
    CLASS_ATTR_OBJ(c, "sig_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mpr_out, sig_ptr);
    CLASS_ATTR_ACCESSORS(c, "sig_ptr", 0, set_sig_ptr);

    CLASS_ATTR_ATOM_LONG(c, "instance", 0, t_mpr_out, instance_id);
    CLASS_ATTR_ACCESSORS(c, "instance", mpr_out_instance_get, mpr_out_instance_set);

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
        x->outlet = listout((t_object *)x);

        x->sig_name = gensym(atom_getsym(argv)->s_name);

        char *temp = atom_getsym(argv+1)->s_name;
        x->sig_type = temp[0];
        if (x->sig_type != 'i' && x->sig_type != 'f')
            return 0;

        x->sig_ptr = 0;
        x->length = 0;
        x->instance_id = 0;
        x->is_instanced = 0;
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
        x->args = atomarray_new(argc-i, argv+i);

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
    if (x->is_instanced) {
        // need to remove self from instance user_data
        t_mpr_ptrs *ptrs = mpr_sig_get_inst_data(x->sig_ptr, x->instance_id);
        if (ptrs && ptrs->num_objs > 0) {
            int i, found = 0;
            for (i = 0; i < ptrs->num_objs; i++) {
                if (found) {
                    ptrs->objs[i-1] = ptrs->objs[i];
                }
                else if (ptrs->objs[i] == (void*)x) {
                    found = 1;
                }
            }
            if (found) {
                --ptrs->num_objs;
                if (ptrs->num_objs <= 0) {
                    // also free struct
                    free(ptrs);
                    mpr_sig_set_inst_data(x->sig_ptr, x->instance_id, NULL);
                }
                else {
                    ptrs->objs = realloc(ptrs->objs, (ptrs->num_objs) * sizeof(t_object *));
                }
            }
        }
    }
    remove_from_hashtab(x);
    if (x->args)
        object_free(x->args);
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

    // store self in the hashtab. IMPORTANT: set the OBJ_FLAG_REF flag so the
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
void parse_extra_properties(t_mpr_out *x, int argc, t_atom *argv)
{
    int i, j, k, length, heterogeneous_types;
    const char *prop_name;
    char type;

    // try to parse atom array as list of properties in form @key [value]
    for (i = 0; i < argc;) {
        if (i > argc - 2) // need at least 2 arguments for key and value
            break;
        else if ((argv + i)->a_type != A_SYM) {
            ++i;
            continue;
        }
        prop_name = atom_get_string(argv + i);

        // ignore some properties
        if (   (prop_name[0] != '@')
            || (strcmp(prop_name, "@name") == 0)
            || (strcmp(prop_name, "@type") == 0)
            || (strcmp(prop_name, "@length") == 0)) {
            ++i;
            continue;
        }

        // ignore leading '@'
        ++prop_name;

        // advance to first value atom
        ++i;

        // find length and type of property value
        length = 0;
        type = 0;
        heterogeneous_types = 0;
        while (i + length < argc) {
            char _type = (argv + i + length)->a_type;
            if (_type == A_SYM && atom_get_string(argv + i + length)[0] == '@') {
                // reached next property name
                break;
            }
            if (!type)
                type = _type;
            else if (type != _type) {
                if ((type == A_LONG && _type == A_FLOAT) || (type == A_FLOAT && _type == A_LONG)) {
                    // we will allow mixed number types
                    type = A_FLOAT;
                    heterogeneous_types = 1;
                }
                else
                    heterogeneous_types = 2;
            }
            ++length;
        }

        if (length <= 0) {
            object_post((t_object*)x, "value missing for property %s", prop_name);
            continue;
        }
        if (heterogeneous_types == 2) {
            object_post((t_object*)x, "only numeric types may be mixed in property values!");
            i += length;
            continue;
        }

        if (strcmp(prop_name, "instance") == 0) {
            if ((argv + i)->a_type == A_SYM &&
                atom_strcmp(argv + i, "polyindex") == 0) {
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
                    }
                }
            }
            else if ((argv + i)->a_type == A_LONG) {
                x->instance_id = atom_getlong(argv + i);
            }
            else {
                object_post((t_object*)x, "instance value must be an integer or 'polyindex'");
                i += length;
                continue;
            }
            if (!x->is_instanced) {
                /* Set use_inst property to True. */
                int one = 1;
                mpr_obj_set_prop(x->sig_ptr, MPR_PROP_USE_INST, NULL, 1, MPR_BOOL, &one, 1);
                /* Remove the default signal instance (0). */
                mpr_sig_remove_inst(x->sig_ptr, 0);
                x->is_instanced = 1;
            }
            t_mpr_ptrs *ptrs = mpr_sig_get_inst_data(x->sig_ptr, x->instance_id);
            if (!ptrs) {
                ptrs = (t_mpr_ptrs *)malloc(sizeof(struct _mpr_ptrs));
                ptrs->objs = (t_object **)malloc(sizeof(t_object *));
                ptrs->num_objs = 1;
                ptrs->objs[0] = (void*)x;
                mpr_sig_reserve_inst(x->sig_ptr, 1, &x->instance_id, (void **)&ptrs);
            }
            else {
                ptrs->objs = realloc(ptrs->objs, (ptrs->num_objs+1) * sizeof(t_object *));
                ptrs->objs[ptrs->num_objs] = (void*)x;
                ptrs->num_objs++;
            }
        }
        else if (   strcmp(prop_name, "minimum") == 0 || strcmp(prop_name, "min") == 0
                 || strcmp(prop_name, "maximum") == 0 || strcmp(prop_name, "max") == 0) {
            // check number of arguments
            mpr_prop extremum = (prop_name[1] == 'i') ? MPR_PROP_MIN : MPR_PROP_MAX;
            if (type != A_LONG && type != A_FLOAT) {
                i += length;
                continue;
            }
            switch (x->sig_type) {
                case 'i': {
                    int *val = malloc(x->sig_length * sizeof(int));
                    for (j = 0, k = 0; j < x->sig_length; j++, k++) {
                        if (k >= length)
                            k = 0;
                        val[j] = atom_coerce_int(argv + i + k);
                    }
                    mpr_obj_set_prop(x->sig_ptr, extremum, NULL, x->sig_length, MPR_INT32, val, 1);
                    free(val);
                    break;
                }
                case 'f': {
                    float *val = malloc(x->sig_length * sizeof(float));
                    for (j = 0, k = 0; j < x->sig_length; j++, k++) {
                        if (k >= length)
                            k = 0;
                        val[j] = atom_coerce_float(argv + i + k);
                    }
                    mpr_obj_set_prop(x->sig_ptr, extremum, NULL, x->sig_length, MPR_FLT, val, 1);
                    free(val);
                    break;
                }
                default:
                    break;
            }
        }
        else {
            switch (type) {
                case A_SYM: {
                    if (length == 1) {
                        const char *value = atom_get_string(argv + i);
                        mpr_obj_set_prop(x->sig_ptr, MPR_PROP_UNKNOWN, prop_name, 1, MPR_STR, value, 1);
                    }
                    else {
                        char *value = malloc(length);
                        for (j = 0; j < length; j++)
                            value[j] = atom_get_string(argv + i + j);
                        mpr_obj_set_prop(x->sig_ptr, MPR_PROP_UNKNOWN, prop_name, length, MPR_STR, &value, 1);
                        free(value);
                    }
                    break;
                }
                case A_FLOAT: {
                    float *value = malloc(length * sizeof(float));
                    for (j = 0; j < length; j++)
                        value[j] = atom_coerce_float(argv + i + j);
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_UNKNOWN, prop_name, length, MPR_FLT, value, 1);
                    free(value);
                    break;
                }
                case A_LONG: {
                    int *value = malloc(length * sizeof(int));
                    for (j = 0; j < length; j++)
                        value[j] = atom_coerce_int(argv + i + j);
                    mpr_obj_set_prop(x->sig_ptr, MPR_PROP_UNKNOWN, prop_name, length, MPR_INT32, value, 1);
                    free(value);
                    break;
                }
                default:
                    break;
            }
        }
        i += length;
    }
    critical_enter(0);
    mpr_obj_push(x->sig_ptr);
    critical_exit(0);
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
    if (x->sig_ptr) {
        long num_atoms;
        t_atom *atoms;
        atomarray_getatoms(x->args, &num_atoms, &atoms);
        parse_extra_properties(x, num_atoms, atoms);
    }
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

    critical_enter(0);
    mpr_sig_set_value(x->sig_ptr, x->instance_id, 1, MPR_INT32, &l);
    critical_exit(0);
}

// *********************************************************
// -(float input)-------------------------------------------
static void mpr_out_float(t_mpr_out *x, double d)
{
    if (check_ptrs(x))
        return;

    critical_enter(0);
    mpr_sig_set_value(x->sig_ptr, x->instance_id, 1, MPR_DBL, &d);
    critical_exit(0);
}

// *********************************************************
// -(list input)--------------------------------------------
static void mpr_out_list(t_mpr_out *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    void *value = 0;

    if (check_ptrs(x) || !argc)
        return;

    if (argc < x->length || (argc % x->length) != 0) {
        object_post((t_object *)x, "Illegal list length (expected factor of %i)",
                    x->length);
        return;
    }

    if (x->type == 'i') {
        int *payload = malloc(argc * sizeof(int));
        value = &payload;
        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_FLOAT)
                payload[i] = (int)atom_getfloat(argv+i);
            else if ((argv+i)->a_type == A_LONG)
                payload[i] = (int)atom_getlong(argv+i);
            else {
                object_post((t_object *)x, "Illegal data type in list!");
                free(payload);
                return;
            }
        }
        //update signal
        critical_enter(0);
        mpr_sig_set_value(x->sig_ptr, x->instance_id, argc, MPR_INT32, value);
        critical_exit(0);
        free(payload);
    }
    else if (x->type == 'f') {
        float *payload = malloc(argc * sizeof(float));
        value = &payload;
        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_FLOAT)
                payload[i] = atom_getfloat(argv+i);
            else if ((argv+i)->a_type == A_LONG)
                payload[i] = (float)atom_getlong(argv+i);
            else {
                object_post((t_object *)x, "Illegal data type in list!");
                free(payload);
                return;
            }
        }
        //update signal
        critical_enter(0);
        mpr_sig_set_value(x->sig_ptr, x->instance_id, argc, MPR_FLT, value);
        critical_exit(0);
        free(payload);
    }
}

// *********************************************************
// -(anything)----------------------------------------------
static void mpr_out_anything(t_mpr_out *x, t_symbol *s, int argc, t_atom *argv)
{
    if (check_ptrs(x)) {
        // we need to cache any arguments to add later
        atomarray_appendatoms(x->args, argc, argv);
    }
    else {
        // we can call parse_extra_properties() immediately
        parse_extra_properties(x, argc, argv);
    }
}

// *********************************************************
// -(release instance)--------------------------------------
static void mpr_out_release(t_mpr_out *x)
{
    if (check_ptrs(x) || !x->is_instanced)
        return;

    critical_enter(0);
    mpr_sig_release_inst(x->sig_ptr, x->instance_id);
    critical_exit(0);
}

// *********************************************************
// -(get instance id)---------------------------------------
t_max_err mpr_out_instance_get(t_mpr_out *x, t_object *attr, long *argc, t_atom **argv)
{
    char alloc;
    atom_alloc(argc, argv, &alloc); // allocate return atom
    atom_setlong(*argv, (long)x->instance_id);
    return 0;
}

// *********************************************************
// -(set instance id)---------------------------------------
t_max_err mpr_out_instance_set(t_mpr_out *x, t_object *attr, long argc, t_atom *argv)
{
    x->instance_id = atom_coerce_int(argv);
    if (!x->is_instanced) {
        /* Set use_inst property to True. */
        int one = 1;
        mpr_obj_set_prop(x->sig_ptr, MPR_PROP_USE_INST, NULL, 1, MPR_BOOL, &one, 1);
        /* Remove the default signal instance (0). */
        mpr_sig_remove_inst(x->sig_ptr, 0);
        x->is_instanced = 1;
    }
    t_mpr_ptrs *ptrs = mpr_sig_get_inst_data(x->sig_ptr, x->instance_id);
    if (!ptrs) {
        ptrs = (t_mpr_ptrs *)malloc(sizeof(struct _mpr_ptrs));
        ptrs->objs = (t_object **)malloc(sizeof(t_object *));
        ptrs->num_objs = 1;
        ptrs->objs[0] = (void*)x;
        mpr_sig_reserve_inst(x->sig_ptr, 1, &x->instance_id, (void **)&ptrs);
    }
    else {
        ptrs->objs = realloc(ptrs->objs, (ptrs->num_objs+1) * sizeof(t_object *));
        ptrs->objs[ptrs->num_objs] = (void*)x;
        ptrs->num_objs++;
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
