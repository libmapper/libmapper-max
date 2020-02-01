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
    t_object            *dev_obj;
    mapper_signal       sig_ptr;
    mapper_timetag_t    *tt_ptr;
    long                is_instance;
    mapper_id           instance_id;
    void                *outlet;
    t_symbol            *myobjname;
    t_object            *patcher;
    t_hashtab           *ht;
    t_atomarray         *args;
    long                connect_state;
    int                 length;
    char                type;
} t_mapout;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapout_new(t_symbol *s, int argc, t_atom *argv);
static void mapout_free(t_mapout *x);

static void add_to_hashtab(t_mapout *x, t_hashtab *ht);
static void remove_from_hashtab(t_mapout *x);
static t_max_err set_sig_ptr(t_mapout *x, t_object *attr, long argc, t_atom *argv);
static t_max_err set_dev_obj(t_mapout *x, t_object *attr, long argc, t_atom *argv);
static t_max_err set_tt_ptr(t_mapout *x, t_object *attr, long argc, t_atom *argv);

static void mapout_loadbang(t_mapout *x);
static void mapout_int(t_mapout *x, long i);
static void mapout_float(t_mapout *x, double f);
static void mapout_list(t_mapout *x, t_symbol *s, int argc, t_atom *argv);
static void mapout_query(t_mapout *x);
static void mapout_release(t_mapout *x);
static void mapout_anything(t_mapout *x, t_symbol *s, int argc, t_atom *argv);

t_max_err mapout_instance_get(t_mapout *x, t_object *attr, long *argc, t_atom **argv);
t_max_err mapout_instance_set(t_mapout *x, t_object *attr, long argc, t_atom *argv);

static int atom_strcmp(t_atom *a, const char *string);
static const char *atom_get_string(t_atom *a);
static void atom_set_string(t_atom *a, const char *string);
static int atom_coerce_int(t_atom *a);
static float atom_coerce_float(t_atom *a);

t_symbol *maybe_start_queue_sym;

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mapout_class;

// *********************************************************
// -(main)--------------------------------------------------
int main(void)
{
    t_class *c;
    c = class_new("map.out", (method)mapout_new, (method)mapout_free,
                  (long)sizeof(t_mapout), 0L, A_GIMME, 0);

    class_addmethod(c, (method)mapout_loadbang, "loadbang", 0);
    class_addmethod(c, (method)mapout_int, "int", A_LONG, 0);
    class_addmethod(c, (method)mapout_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)mapout_list, "list", A_GIMME, 0);
    class_addmethod(c, (method)mapout_query, "query", 0);
    class_addmethod(c, (method)mapout_release, "release", 0);
    class_addmethod(c, (method)mapout_anything, "anything", A_GIMME, 0);
    class_addmethod(c, (method)add_to_hashtab, "add_to_hashtab", A_CANT, 0);
    class_addmethod(c, (method)remove_from_hashtab, "remove_from_hashtab", A_CANT, 0);

    CLASS_ATTR_SYM(c, "sig_name", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_name);
    CLASS_ATTR_LONG(c, "sig_length", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_length);
    CLASS_ATTR_CHAR(c, "sig_type", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_type);
    CLASS_ATTR_OBJ(c, "dev_obj", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, dev_obj);
    CLASS_ATTR_ACCESSORS(c, "dev_obj", 0, set_dev_obj);
    CLASS_ATTR_OBJ(c, "sig_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, sig_ptr);
    CLASS_ATTR_ACCESSORS(c, "sig_ptr", 0, set_sig_ptr);
    CLASS_ATTR_OBJ(c, "tt_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapout, tt_ptr);
    CLASS_ATTR_ACCESSORS(c, "tt_ptr", 0, set_tt_ptr);

    CLASS_ATTR_LONG(c, "instance", 0, t_mapout, instance_id);
    CLASS_ATTR_ACCESSORS(c, "instance", mapout_instance_get, mapout_instance_set);

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
        x->outlet = listout((t_object *)x);

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
        x->args = atomarray_new(argc-i, argv+i);

        // cache the registered name so we can remove self from hashtab later
        x = object_register(CLASS_BOX, x->myobjname = symbol_unique(), x);

        x->patcher = (t_object *)gensym("#P")->s_thing;
        mapout_loadbang(x);
    }
    maybe_start_queue_sym = gensym("maybe_start_queue");
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
static void mapout_free(t_mapout *x)
{
    remove_from_hashtab(x);
    if (x->args)
        object_free(x->args);
}

void mapout_loadbang(t_mapout *x)
{
    t_hashtab *ht;

    if (!x->patcher)
        return;

    t_object *patcher = x->patcher;
    while (patcher) {
        object_obex_lookup(patcher, gensym("mapperhash"), (t_object **)&ht);
        if (ht) {
            add_to_hashtab(x, ht);
            break;
        }
        patcher = jpatcher_get_parentpatcher(patcher);
    }
}

void add_to_hashtab(t_mapout *x, t_hashtab *ht)
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

void remove_from_hashtab(t_mapout *x)
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
void parse_extra_properties(t_mapout *x, int argc, t_atom *argv)
{
    int i, j, k, length, heterogeneous_types;
    const char *prop;
    char type;

    // try to parse atom array as list of properties in form @key [value]
    for (i = 0; i < argc;) {
        if (i > argc - 2) // need at least 2 arguments for key and value
            break;
        else if ((argv + i)->a_type != A_SYM) {
            ++i;
            continue;
        }
        prop = atom_get_string(argv + i);

        // ignore some properties
        if (   (prop[0] != '@')
            || (strcmp(prop, "@name") == 0)
            || (strcmp(prop, "@type") == 0)
            || (strcmp(prop, "@length") == 0)) {
            ++i;
            continue;
        }

        // ignore leading '@'
        ++prop;

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
                if (   (type == A_LONG && _type == A_FLOAT)
                    || (type == A_FLOAT && _type == A_LONG)) {
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
            object_post((t_object*)x, "value missing for property %s", prop);
            continue;
        }
        if (heterogeneous_types == 2) {
            object_post((t_object*)x, "only numeric types may be mixed in property values!");
            i += length;
            continue;
        }

        if (strcmp(prop, "instance") == 0) {
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
            /* Remove the default signal instance (0) if it exists. Since the user
             * may have properly added an instance 0, we will check for user_data. */
            void *data = mapper_signal_instance_user_data(x->sig_ptr, 0);
            if (!data)
                mapper_signal_remove_instance(x->sig_ptr, 0);

            x->is_instance = 1;
            mapper_signal_reserve_instances(x->sig_ptr, 1, &x->instance_id, (void **)&x);
        }
        else if (   strcmp(prop, "minimum") == 0 || strcmp(prop, "min") == 0
                 || strcmp(prop, "maximum") == 0 || strcmp(prop, "max") == 0) {
            // check number of arguments
            int is_min = (prop[1] == 'i');
            if (type != A_LONG && type != A_FLOAT) {
                i += length;
                continue;
            }
            switch (x->sig_type) {
                case 'i': {
                    int val[x->sig_length];
                    for (j = 0; j < x->sig_length; j++) {
                        for (k = 0; k < length; k++)
                            val[j] = atom_coerce_int(argv + i + k);
                    }
                    if (is_min)
                        mapper_signal_set_minimum(x->sig_ptr, val);
                    else
                        mapper_signal_set_maximum(x->sig_ptr, val);
                    break;
                }
                case 'f': {
                    float val[x->sig_length];
                    for (j = 0; j < x->sig_length; j++) {
                        for (k = 0; k < length; k++)
                            val[j] = atom_coerce_float(argv + i + k);
                    }
                    if (is_min)
                        mapper_signal_set_minimum(x->sig_ptr, val);
                    else
                        mapper_signal_set_maximum(x->sig_ptr, val);
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
                        mapper_signal_set_property(x->sig_ptr, prop,
                                                   1, 's', value, 1);
                    }
                    else {
                        const char *value[length];
                        for (j = 0; j < length; j++)
                            value[j] = atom_get_string(argv + i + j);
                        mapper_signal_set_property(x->sig_ptr, prop, length,
                                                   's', &value, 1);
                    }
                    break;
                }
                case A_FLOAT: {
                    float value[length];
                    for (j = 0; j < length; j++)
                        value[j] = atom_coerce_float(argv + i + j);
                    mapper_signal_set_property(x->sig_ptr, prop, length,
                                               'f', value, 1);
                    break;
                }
                case A_LONG: {
                    int value[length];
                    for (j = 0; j < length; j++)
                        value[j] = atom_coerce_int(argv + i + j);
                    mapper_signal_set_property(x->sig_ptr, prop, length,
                                               'i', value, 1);
                    break;
                }
                default:
                    break;
            }
        }
        i += length;
    }
    mapper_signal_push(x->sig_ptr);
}

// *********************************************************
// -(set the device pointer)--------------------------------
t_max_err set_dev_obj(t_mapout *x, t_object *attr, long argc, t_atom *argv)
{
    x->dev_obj = (t_object *)argv->a_w.w_obj;
    return 0;
}

// *********************************************************
// -(set the signal pointer)--------------------------------
t_max_err set_sig_ptr(t_mapout *x, t_object *attr, long argc, t_atom *argv)
{
    x->sig_ptr = (mapper_signal)argv->a_w.w_obj;
    if (x->sig_ptr) {
        long num_atoms;
        t_atom *atoms;
        atomarray_getatoms(x->args, &num_atoms, &atoms);
        parse_extra_properties(x, num_atoms, atoms);
    }
    return 0;
}

// *********************************************************
// -(set the device pointer)--------------------------------
t_max_err set_tt_ptr(t_mapout *x, t_object *attr, long argc, t_atom *argv)
{
    x->tt_ptr = (mapper_timetag_t *)argv->a_w.w_obj;
    return 0;
}

// *********************************************************
// -(check if device and signal pointers have been set)-----
static int check_ptrs(t_mapout *x)
{
    if (!x || !x->dev_obj || !x->sig_ptr) {
        return 1;
    }
    else if (!x->length) {
        x->length = mapper_signal_length(x->sig_ptr);
        x->type = mapper_signal_type(x->sig_ptr);
    }
    return 0;
}

// *********************************************************
// -(int input)---------------------------------------------
static void mapout_int(t_mapout *x, long l)
{
    int i;
    float f;
    void *value = 0;

    if (check_ptrs(x))
        return;

    if (x->length != 1)
        return;
    if (x->type == 'i') {
        i = (int)l;
        value = &i;
    }
    else if (x->type == 'f') {
        f = (float)l;
        value = &f;
    }
    critical_enter(0);
    object_method(x->dev_obj, maybe_start_queue_sym);
    if (x->is_instance)
        mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                      value, 1, *x->tt_ptr);
    else
        mapper_signal_update(x->sig_ptr, value, 1, *x->tt_ptr);
    critical_exit(0);
}

// *********************************************************
// -(float input)-------------------------------------------
static void mapout_float(t_mapout *x, double d)
{
    int i;
    float f;
    void *value = 0;

    if (check_ptrs(x))
        return;

    if (x->length != 1)
        return;
    if (x->type == 'f') {
        f = (float)d;
        value = &f;
    }
    else if (x->type == 'i') {
        i = (int)d;
        value = &i;
    }
    critical_enter(0);
    object_method(x->dev_obj, maybe_start_queue_sym);
    if (x->is_instance)
        mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                      value, 1, *x->tt_ptr);
    else
        mapper_signal_update(x->sig_ptr, value, 1, *x->tt_ptr);
    critical_exit(0);
}

// *********************************************************
// -(list input)--------------------------------------------
static void mapout_list(t_mapout *x, t_symbol *s, int argc, t_atom *argv)
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
        critical_enter(0);
        object_method(x->dev_obj, maybe_start_queue_sym);
        if (x->is_instance) {
            mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                          value, count, *x->tt_ptr);
        }
        else {
            mapper_signal_update(x->sig_ptr, value, count, *x->tt_ptr);
        }
        critical_exit(0);
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
        critical_enter(0);
        //update signal
        object_method(x->dev_obj, maybe_start_queue_sym);
        if (x->is_instance) {
            mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                          value, count, *x->tt_ptr);
        }
        else {
            mapper_signal_update(x->sig_ptr, value, count, *x->tt_ptr);
        }
        critical_exit(0);
    }
}

// *********************************************************
// -(anything)----------------------------------------------
static void mapout_anything(t_mapout *x, t_symbol *s, int argc, t_atom *argv)
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
// -(query remote endpoints)--------------------------------
static void mapout_query(t_mapout *x)
{
    if (check_ptrs(x))
        return;

    /* TODO: we should cache query timetag and object pointer so that
     *  we can deliver responses only to the object being queried. */

    mapper_signal_query_remotes(x->sig_ptr, MAPPER_NOW);
}

// *********************************************************
// -(release instance)--------------------------------------
static void mapout_release(t_mapout *x)
{
    if (check_ptrs(x) || !x->is_instance)
        return;

    critical_enter(0);
    object_method(x->dev_obj, maybe_start_queue_sym);
    mapper_signal_instance_release(x->sig_ptr, x->instance_id, *x->tt_ptr);
    critical_exit(0);
}

// *********************************************************
// -(get instance id)---------------------------------------
t_max_err mapout_instance_get(t_mapout *x, t_object *attr, long *argc, t_atom **argv)
{
    char alloc;
    atom_alloc(argc, argv, &alloc); // allocate return atom
    atom_setlong(*argv, (long)x->instance_id);
    return 0;
}

// *********************************************************
// -(set instance id)---------------------------------------
t_max_err mapout_instance_set(t_mapout *x, t_object *attr, long argc, t_atom *argv)
{
    x->instance_id = atom_coerce_int(argv);
    x->is_instance = 1;
    mapper_signal_reserve_instances(x->sig_ptr, 1, &x->instance_id, (void **)&x);
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
