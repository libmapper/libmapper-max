//
// mapper.c
// a maxmsp external encapsulating the functionality of libmapper
// http://www.idmil.org/software/mappingtools
// Joseph Malloch, IDMIL 2010
// LGPL
//

#define MAXMSP

// *********************************************************
// -(Includes)----------------------------------------------

#ifdef MAXMSP
    #include "ext.h"			// standard Max include, always required
    #include "ext_obex.h"		// required for new style Max object
    #include "ext_dictionary.h"
    #include "jpatcher_api.h"
#else
    #include "m_pd.h"
#endif
#include "../libmapper/src/mapper_internal.h"
#include "../libmapper/include/mapper/mapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lo/lo.h"

#include <unistd.h>
#include <arpa/inet.h>

#define INTERVAL 1
#define MAX_PATH_CHARS 2048
#define MAX_FILENAME_CHARS 512

// *********************************************************
// -(object struct)-----------------------------------------
typedef struct _mapper 
{
	t_object ob;
    void *outlet1;
    void *outlet2;
    void *outlet3;
    void *clock;          // pointer to clock object
	char *name;
    mapper_device device;
    mapper_signal signal;
    int ready;
    int learn_mode;
#ifdef MAXMSP
    char *definition;
    t_dictionary *d;
#endif
} t_mapper;

t_symbol *ps_list;
int port = 9000;

// *********************************************************
// -(function prototypes)-----------------------------------
void *mapper_new(t_symbol *s, int argc, t_atom *argv);
void mapper_free(t_mapper *x);
void mapper_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
void mapper_add_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
void mapper_remove_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
void mapper_poll(t_mapper *x);
void mapper_float_handler(mapper_signal msig, mapper_signal_value_t *v);
void mapper_int_handler(mapper_signal msig, mapper_signal_value_t *v);
void mapper_print_properties(t_mapper *x);
void mapper_read_definition(t_mapper *x);
void mapper_register_signals(t_mapper *x);
void mapper_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
int mapper_setup_device(t_mapper *x);
#ifdef MAXMSP
    void mapper_assist(t_mapper *x, void *b, long m, long a, char *s);
#endif

// *********************************************************
// -(global class pointer variable)-------------------------
void *mapper_class;

// *********************************************************
// -(main)--------------------------------------------------
#ifdef MAXMSP
    int main(void)
    {	
        t_class *c;
        c = class_new("mapper", (method)mapper_new, (method)mapper_free, 
                      (long)sizeof(t_mapper), 0L, A_GIMME, 0);
        class_addmethod(c, (method)mapper_assist,         "assist",   A_CANT,     0);
        class_addmethod(c, (method)mapper_add_signal,     "add",      A_GIMME,    0);
        class_addmethod(c, (method)mapper_remove_signal,  "remove",   A_GIMME,    0);
        class_addmethod(c, (method)mapper_anything,       "anything", A_GIMME,    0);
        class_addmethod(c, (method)mapper_learn,          "learn",    A_LONG,     0);
        class_register(CLASS_BOX, c); /* CLASS_NOBOX */
        mapper_class = c;
        ps_list = gensym("list");
        return 0;
    }
#else
    int mapper_setup(void)
    {
        t_class *c;
        c = class_new(gensym("mapper"), (t_newmethod)mapper_new, (t_method)mapper_free, 
                      (long)sizeof(t_mapper), 0L, A_GIMME, 0);
        class_addmethod(c,  (t_method)mapper_add_signal,    gensym("add"),      A_GIMME, 0);
        class_addmethod(c,  (t_method)mapper_remove_signal, gensym("remove"),   A_GIMME, 0);
        class_addanything(c, (t_method)mapper_anything);
        class_addmethod(c, (t_method)mapper_learn, gensym("learn"), A_FLOAT, 0);
        mapper_class = c;
        ps_list = gensym("list");
        return 0;
    }
#endif

// *********************************************************
// -(new)---------------------------------------------------
void *mapper_new(t_symbol *s, int argc, t_atom *argv)
{
	t_mapper *x = NULL;
    long i;
    char *alias = NULL;
    
#ifdef MAXMSP
    if (x = object_alloc(mapper_class)) {
        x->outlet3 = outlet_new((t_object *)x,0);
        x->outlet2 = outlet_new((t_object *)x,0);
        x->outlet1 = listout((t_object *)x);
        
        x->name = strdup("maxmsp");
        
        for (i = 0; i < argc; i++) {
            if ((argv + i)->a_type == A_SYM) {
                if(strcmp(atom_getsym(argv+i)->s_name, "@alias") == 0) {
                    if ((argv + i + 1)->a_type == A_SYM) {
                        alias = strdup(atom_getsym(argv+i+1)->s_name);
                        i++;
                    }
                }
                else if ((strcmp(atom_getsym(argv+i)->s_name, "@def") == 0) || 
                         (strcmp(atom_getsym(argv+i)->s_name, "@definition") == 0)) {
                    if ((argv + i + 1)->a_type == A_SYM) {
                        x->definition = strdup(atom_getsym(argv+i+1)->s_name);
                        mapper_read_definition(x);
                        i++;
                    }
                }
            }
        }
#else
    if (x = (t_mapper *) pd_new(mapper_class) ) {
        x->outlet3 = outlet_new(&x->ob, 0);
        x->outlet2 = outlet_new(&x->ob, 0);
        x->outlet1 = outlet_new(&x->ob, 0);
        
        x->name = strdup("puredata");
        
        for (i = 0; i < argc; i++) {
            if ((argv + i)->a_type == A_SYMBOL) {
                if(strcmp((argv+i)->a_w.w_symbol->s_name, "@alias") == 0) {
                    if ((argv + i + 1)->a_type == A_SYMBOL) {
                        alias = strdup((argv+i+1)->a_w.w_symbol->s_name);
                        i++;
                    }
                }
                else if ((strcmp((argv+i)->a_w.w_symbol->s_name, "@def") == 0) || 
                         (strcmp((argv+i)->a_w.w_symbol->s_name, "@definition") == 0)) {
                    if ((argv + i + 1)->a_type == A_SYMBOL) {
                        //x->definition = strdup((argv+i+1)->a_w.w_symbol->s_name);
                        //mapper_read_definition(x);
                        i++;
                    }
                }
            }
        }
#endif
        
        if (alias) {
            free(x->name);
            x->name = strdup(alias);
        }
        if (*x->name == '/')
            *x->name++;
        
        if (mapper_setup_device(x)) {
            post("Error initializing device.\n");
        }
        else {
            x->ready = 0;
            x->learn_mode = 0;
#ifdef MAXMSP
            x->clock = clock_new(x, (method)mapper_poll);	// Create the timing clock
#else
            x->clock = clock_new(x, (t_method)mapper_poll);
#endif
            clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
            mapper_register_signals(x);
        }
    }
	return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
void mapper_free(t_mapper *x)
{
    clock_unset(x->clock);	// Remove clock routine from the scheduler
    clock_free(x->clock);		// Frees memeory used by clock
    
#ifdef MAXMSP
    object_free(x->d);          // Frees memory used by dictionary
#endif
    
    if (x->device) {
        if (x->device->routers) {
            post("Removing router...");
            mdev_remove_router(x->device, x->device->routers);
        }
        post("Freeing device %s...", mapper_admin_name(x->device->admin));
        mdev_free(x->device);
    }
}

// *********************************************************
// -(print properties)--------------------------------------
void mapper_print_properties(t_mapper *x)
{
    t_atom myList[2];
	char *message;
    
    if (x->ready) {        
        //output name
        message = strdup(mapper_admin_name(x->device->admin));
#ifdef MAXMSP
        atom_setsym(myList, gensym("name"));
        atom_setsym(myList + 1, gensym(message));
#else
        SETSYMBOL(myList, gensym("name"));
        SETSYMBOL(myList + 1, gensym(message));
#endif
        outlet_list(x->outlet3, ps_list, 2, myList);
        
        //output IP
        message = strdup(inet_ntoa(x->device->admin->interface_ip));
#ifdef MAXMSP
        atom_setsym(myList, gensym("IP"));
        atom_setsym(myList + 1, gensym(message));
#else
        SETSYMBOL(myList, gensym("IP"));
        SETSYMBOL(myList + 1, gensym(message));
#endif
        outlet_list(x->outlet3, ps_list, 2, myList);
        
        //output port
#ifdef MAXMSP
        atom_setsym(myList, gensym("port"));
        atom_setlong(myList + 1, x->device->admin->port.value);
#else
        SETSYMBOL(myList, gensym("port"));
        SETFLOAT(myList + 1, (float)x->device->admin->port.value);
#endif
        outlet_list(x->outlet3, ps_list, 2, myList);
        
        //output numInputs
#ifdef MAXMSP
        atom_setsym(myList, gensym("numInputs"));
        atom_setlong(myList + 1, mdev_num_inputs(x->device));
#else
        SETSYMBOL(myList, gensym("numInputs"));
        SETFLOAT(myList + 1, (float)mdev_num_inputs(x->device));
#endif
        outlet_list(x->outlet3, ps_list, 2, myList);
        
        //output numOutputs
#ifdef MAXMSP
        atom_setsym(myList, gensym("numOutputs"));
        atom_setlong(myList + 1, mdev_num_outputs(x->device));
#else
        SETSYMBOL(myList, gensym("numOutputs"));
        SETFLOAT(myList + 1, (float)mdev_num_outputs(x->device));
#endif
        outlet_list(x->outlet3, ps_list, 2, myList);
    }
}

// *********************************************************
// -(inlet/outlet assist - maxmsp only)---------------------
#ifdef MAXMSP
void mapper_assist(t_mapper *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { // inlet
		sprintf(s, "OSC input");
	} 
	else {	// outlet
        if (a == 0) {
            sprintf(s, "Mapped OSC data");
        }
        else if (a == 1) {
            sprintf(s, "State queries");
        }
        else {
            sprintf(s, "Device information");
        }
	}
}
#endif

// *********************************************************
// -(add signal)--------------------------------------------
void mapper_add_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
	t_atom myList[2];
    //need to read attribs: type, units, min/minimum, max/maximum
    //char *type;
    char *sig_name, *sig_units = 0, sig_type = 0;
    int sig_min_int, sig_max_int, sig_length = 1;
    int *sig_min_int_ptr = 0, *sig_max_int_ptr = 0;
    float sig_min_float, sig_max_float;
    float *sig_min_float_ptr = 0, *sig_max_float_ptr = 0;
    long i;
    
    if (argc < 4)
		return;

#ifdef MAXMSP
    if ((argv->a_type == A_SYM) && ((argv+1)->a_type == A_SYM)) {
        //get signal name
        sig_name = strdup(atom_getsym(argv+1)->s_name);
        
        //parse signal properties
        for (i = 2; i < argc; i++) {
            if ((argv + i)->a_type == A_SYM) {
                if(strcmp(atom_getsym(argv+i)->s_name, "@units") == 0) {
                    if ((argv + i + 1)->a_type == A_SYM) {
                        sig_units = strdup(atom_getsym(argv+i+1)->s_name);
                        i++;
                    }
                }
                else if (strcmp(atom_getsym(argv+i)->s_name, "@type") == 0) {
                    if ((argv + i + 1)->a_type == A_SYM) {
                        char *temp = atom_getsym(argv + i + 1)->s_name;
                        if ((strcmp(temp, "int") == 0) || (strcmp(temp, "i") == 0))
                            sig_type = 'i';
                        else if ((strcmp(temp, "float") == 0) || (strcmp(temp, "f") == 0))
                            sig_type = 'f';
                        else {
                            post("Skipping registration of signal %s (unknown type).\n", sig_name);
                            return;
                        }
                        i++;
                    }
                }
                else if ((strcmp(atom_getsym(argv+i)->s_name, "@min") == 0) || (strcmp(atom_getsym(argv+i)->s_name, "@minimum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_min_float = atom_getfloat(argv + i + 1);
                        sig_min_float_ptr = &sig_min_float;
                        sig_min_int = (int)sig_min_float;
                        sig_min_int_ptr = &sig_min_int;
                        i++;
                    }
                    else if ((argv + i + 1)->a_type == A_LONG) {
                        sig_min_int = (int)atom_getlong(argv + i + 1);
                        sig_min_int_ptr = &sig_min_int;
                        sig_min_float = (float)sig_min_int;
                        sig_min_float_ptr = &sig_min_float;
                        i++;
                    }
                }
                else if ((strcmp(atom_getsym(argv+i)->s_name, "@max") == 0) || (strcmp(atom_getsym(argv+i)->s_name, "@maximum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_max_float = atom_getfloat(argv + i + 1);
                        sig_max_float_ptr = &sig_max_float;
                        sig_max_int = (int)sig_max_float;
                        sig_max_int_ptr = &sig_max_int;
                        i++;
                    }
                    else if ((argv + i + 1)->a_type == A_LONG) {
                        sig_max_int = (int)atom_getlong(argv + i + 1);
                        sig_max_int_ptr = &sig_max_int;
                        sig_max_float = (float)sig_max_int;
                        sig_max_float_ptr = &sig_max_float;
                        i++;
                    }
                }
                else if (strcmp(atom_getsym(argv+i)->s_name, "@length") == 0) {
                    if ((argv + i + 1)->a_type == A_LONG) {
                        sig_length = (int)atom_getlong(argv + i + 1);
                        i++;
                    }
                }
            }
        }
        if (sig_type) {
            if (strcmp(atom_getsym(argv)->s_name, "input") == 0) {
                mdev_add_input(x->device, atom_getsym(argv + 1)->s_name, sig_length, sig_type, sig_units,
                               sig_type == 'i' ? MSIGVALP(sig_min_int_ptr) : MSIGVALP(sig_max_float_ptr), 
                               sig_type == 'i' ? MSIGVALP(sig_max_int_ptr) : MSIGVALP(sig_max_float_ptr), 
                               mapper_int_handler, x);
                
                //output numInputs
                atom_setsym(myList, gensym("numInputs"));
                atom_setlong(myList + 1, mdev_num_inputs(x->device));
                outlet_list(x->outlet3, ps_list, 2, myList);
            } 
            else if (strcmp(atom_getsym(argv)->s_name, "output") == 0) {
                mdev_add_output(x->device, sig_length, atom_getsym(argv + 1)->s_name, sig_units, sig_type, 
                                sig_type == 'i' ? MSIGVALP(sig_min_int_ptr) : MSIGVALP(sig_max_float_ptr), 
                                sig_type == 'i' ? MSIGVALP(sig_max_int_ptr) : MSIGVALP(sig_max_float_ptr));
                
                //output numOutputs
                atom_setsym(myList, gensym("numOutputs"));
                atom_setlong(myList + 1, mdev_num_outputs(x->device));
                outlet_list(x->outlet3, ps_list, 2, myList);
            }
        }
        else {
            post("Skipping registration of signal %s (undeclared type).\n", sig_name);
        }
	}
#else
    if ((argv->a_type == A_SYMBOL) && ((argv+1)->a_type == A_SYMBOL)) {
        //get signal name
        sig_name = strdup((argv+1)->a_w.w_symbol->s_name);
        
        //parse signal properties
        for (i = 2; i < argc; i++) {
            if ((argv + i)->a_type == A_SYMBOL) {
                if(strcmp((argv+i)->a_w.w_symbol->s_name, "@units") == 0) {
                    if ((argv + i + 1)->a_type == A_SYMBOL) {
                        sig_units = strdup((argv+i+1)->a_w.w_symbol->s_name);
                        i++;
                    }
                }
                else if (strcmp((argv+i)->a_w.w_symbol->s_name, "@type") == 0) {
                    if ((argv + i + 1)->a_type == A_SYMBOL) {
                        sig_type = strdup((argv+i+1)->a_w.w_symbol->s_name);
                        i++;
                    }
                }
                else if ((strcmp((argv+i)->a_w.w_symbol->s_name, "@min") == 0) || 
                         (strcmp((argv+i)->a_w.w_symbol->s_name, "@minimum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_min_float = atom_getfloat(argv + i + 1);
                        sig_min_float_ptr = &sig_min_float;
                        sig_min_int = (int)sig_min_float;
                        sig_min_int_ptr = &sig_min_int;
                        i++;
                    }
                }
                else if ((strcmp((argv+i)->a_w.w_symbol->s_name, "@max") == 0) || 
                         (strcmp((argv+i)->a_w.w_symbol->s_name, "@maximum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_max_float = atom_getfloat(argv + i + 1);
                        sig_max_float_ptr = &sig_max_float;
                        sig_max_int = (int)sig_max_float;
                        sig_max_int_ptr = &sig_max_int;
                        i++;
                    }
                }
                else if (strcmp((argv+i)->a_w.w_symbol->s_name, "@length") == 0) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_length = (int)atom_getfloat(argv + i + 1);
                        i++;
                    }
                }
            }
        }
        if (sig_type && *sig_type) {
            if (strcmp((argv)->a_w.w_symbol->s_name, "input") == 0) {
                if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0)) {
                    /*x->signal = msig_int(sig_length, (argv + 1)->a_w.w_symbol->s_name, 
                     sig_units, sig_min_int_ptr, sig_max_int_ptr, 
                     0, int_handler, x);
                     mdev_register_input(x->device, x->signal);*/
                }
                else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0)) {
                    /*x->signal = msig_float(sig_length, (argv + 1)->a_w.w_symbol->s_name, 
                     sig_units, sig_min_float_ptr, sig_max_float_ptr, 
                     0, float_handler, x);
                     mdev_register_input(x->device, x->signal);*/
                }
                else {
                    post("Skipping registration of signal %s (unknown type).", sig_name);
                }
                
                //output numInputs
                SETSYMBOL(myList, gensym("numInputs"));
                SETFLOAT(myList + 1, mdev_num_inputs(x->device));
                outlet_anything(x->outlet3, ps_list, 2, myList);
            } 
            else if (strcmp((argv)->a_w.w_symbol->s_name, "output") == 0) {
                if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0)) {
                    /*x->signal = msig_int(sig_length, (argv + 1)->a_w.w_symbol->s_name, 
                     sig_units, sig_min_int_ptr, sig_max_int_ptr, 
                     0, 0, 0);
                     mdev_register_output(x->device, x->signal);*/
                }
                else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0)) {
                    /*x->signal = msig_float(sig_length, (argv + 1)->a_w.w_symbol->s_name, 
                     sig_units, sig_min_float_ptr, sig_max_float_ptr, 
                     0, 0, 0);
                     mdev_register_output(x->device, x->signal);*/
                }
                else {
                    post("Skipping registration of signal %s (unknown type).", sig_name);
                }
                
                //output numOutputs
                SETSYMBOL(myList, gensym("numOutputs"));
                SETFLOAT(myList + 1, mdev_num_outputs(x->device));
                outlet_anything(x->outlet3, ps_list, 2, myList);
            }
        }
        else {
            post("Skipping registration of signal %s (undeclared type).", sig_name);
        }
	}
#endif
}

// *********************************************************
// -(remove signal)-----------------------------------------
void mapper_remove_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
	// not yet supported by libmapper
}

// *********************************************************
// -(anything)----------------------------------------------
void mapper_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
	if (argc) {
        //find signal
        mapper_signal msig;
        if (mdev_find_output_by_name(x->device, s->s_name, &msig) == -1) {
            if (x->learn_mode) {
                // register as new signal
                if (argv->a_type == A_FLOAT) {
                    msig = mdev_add_output(x->device, argc, s->s_name, 0, 'f', 0, 0, 0, mapper_float_handler, x);
                }
#ifdef MAXMSP
                else if (argv->a_type == A_LONG) {
                    msig = mdev_add_output(x->device, argc, s->s_name, 0, 'i', 0, 0, 0, mapper_int_handler, x);
                }
#endif
            }
            else {
                return;
            }

        }
        if (msig->props.type == 'i') {
            int payload[msig->props.length];
            for (i = 0; i < argc; i++) {
                if ((argv + i)->a_type == A_FLOAT)
                    payload[i] = (int)atom_getfloat(argv + i);
#ifdef MAXMSP
                else if ((argv + i)->a_type == A_LONG)
                    payload[i] = (int)atom_getlong(argv + i);
#endif
                
            }
            // zero-pad if necessary???
            for (; i < msig->props.length; i++) {
                payload[i] = 0;
            }
            //update signal
            msig_update(msig, MSIGVALP(payload));
        }
        else if (msig->props.type == 'f') {
            float payload[msig->props.length];
            for (i = 0; i < argc; i++) {
                if ((argv + i)->a_type == A_FLOAT)
                    payload[i] = atom_getfloat(argv + i);
#ifdef MAXMSP
                else if ((argv + i)->a_type == A_LONG)
                    payload[i] = (float)atom_getlong(argv + i);
#endif
                
            }
            // zero-pad if necessary???
            for (; i < msig->props.length; i++) {
                payload[i] = 0.;
            }
            //update signal
            msig_update(msig, MSIGVALP(payload));
        }
        else {
            return;
        }
    }
}

// *********************************************************
// -(int handler)-------------------------------------------
void mapper_int_handler(mapper_signal msig, mapper_signal_value_t *v)
{
    t_mapper *x = msig->user_data;
	char *path = strdup(msig->props.name);
    int i, length = msig->props.length;
	
    t_atom myList[length];
#ifdef MAXMSP
    atom_setsym(myList, gensym(path));
    for (i = 0; i < length; i++) {
        atom_setlong(myList + i + 1, (*v).i32);
    }
#else
    SETSYMBOL(myList, gensym(path));
    for (i = 0; i < length; i++) {
        SETFLOAT(myList + i + 1, (float)(*v).i32);
    }
#endif
    outlet_list(x->outlet1, ps_list, 2, myList);
}

// *********************************************************
// -(float handler)-----------------------------------------
void mapper_float_handler(mapper_signal msig, mapper_signal_value_t *v)
{
    t_mapper *x = msig->user_data;
	char *path = strdup(msig->props.name);
    int i, length = msig->props.length;
	
    t_atom myList[length];
#ifdef MAXMSP
    atom_setsym(myList, gensym(path));
    for (i = 0; i < length; i++) {
        atom_setfloat(myList + i + 1, (*v).f);
    }
#else
    SETSYMBOL(myList, gensym(path));
    for (i = 0; i < length; i++) {
        SETFLOAT(myList + 1, (*v).f);
    }
#endif
    outlet_list(x->outlet1, ps_list, 2, myList);
}

// *********************************************************
// -(set up new device)-------------------------------------
int mapper_setup_device(t_mapper *x)
{
    post("using name: %s", x->name);
    
    x->device = mdev_new(x->name, port, 0);

    if (!x->device)
        return 1;
    else
        mapper_print_properties(x);
    
    return 0;
}

// *********************************************************
// -(read device definition - maxmsp only)------------------
#ifdef MAXMSP
void mapper_read_definition (t_mapper *x)
{
    if (x->d) {
        object_free(x->d);
    }
    t_object *info;
    t_symbol *sym_device = gensym("device");
    t_symbol *sym_name = gensym("name");
    const char *my_name = 0;
    short path;
    long filetype = 'JSON', outtype;
    
    //add ".json" to end of string if missing (or pick new filetype!)
    
    if (locatefile_extended(x->definition, &path, &outtype, &filetype, 1) == 0) {
        post("located file %s\n", x->definition);
        if (dictionary_read(x->definition, path, &(x->d)) == 0) {
            //check that first key is "device"
            if (dictionary_entryisdictionary(x->d, sym_device)) {
                //recover name from dictionary
                dictionary_getdictionary(x->d, sym_device, &info);
                dictionary_getstring((t_dictionary *)info, sym_name, &my_name);
                if (my_name) {
                    free(x->name);
                    x->name = strdup(my_name);
                }
            }
        }
        else {
            post("Could not parse file %s", x->definition);
        }
    }
    else {
        post("Could not locate file %s", x->definition);
    }
}
#endif

// *********************************************************
// -(register signals from dictionary - maxmsp only)--------
#ifdef MAXMSP
void mapper_register_signals(t_mapper *x) {
    t_atom *signals;
    long num_signals, i;
    t_object *device, *inputs, *outputs, *temp;
    t_symbol *sym_device = gensym("device");
    t_symbol *sym_inputs = gensym("inputs");
    t_symbol *sym_outputs = gensym("outputs");
    t_symbol *sym_name = gensym("name");
    t_symbol *sym_type = gensym("type");
    t_symbol *sym_units = gensym("units");
    t_symbol *sym_minimum = gensym("minimum");
    t_symbol *sym_maximum = gensym("maximum");
    t_symbol *sym_length = gensym("length");
    
    const char *sig_name, *sig_units, *sig_type;
    char sig_type_char = 0;
    double sig_min_double, sig_max_double;
    float sig_min_float, sig_max_float, *sig_min_float_ptr = 0, *sig_max_float_ptr = 0;
    long sig_min_long, sig_max_long, sig_length;
    int sig_min_int, sig_max_int, *sig_min_int_ptr = 0, *sig_max_int_ptr = 0;
    
    if (x->d) {
        // Get pointer to dictionary "device"
        if (dictionary_getdictionary(x->d, sym_device, &device) == MAX_ERR_NONE) {
            // Get pointer to atom array "inputs"
            if (dictionary_getatomarray((t_dictionary *)device, sym_inputs, &inputs) == MAX_ERR_NONE) {
                atomarray_getatoms((t_atomarray *)inputs, &num_signals, &signals);
                // iterate through array of atoms
                for (i=0; i<num_signals; i++) {
                    // initialize variables
                    if (sig_units) {
                        free(&sig_units);
                    }
                    if (sig_type) {
                        free(&sig_type);
                    }
                    sig_min_int_ptr = sig_max_int_ptr = 0;
                    sig_min_float_ptr = sig_max_float_ptr = 0;
                    sig_length = 1;
                    
                    // each atom object points to a dictionary, need to recover atoms by key
                    temp = atom_getobj(&(signals[i]));
                    if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) == MAX_ERR_NONE) {
                        dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units);
                        dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type);
                        dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length);
                        if (dictionary_getfloat((t_dictionary *)temp, sym_minimum, &sig_min_double) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_double;
                            sig_min_float_ptr = &sig_min_float;
                            sig_min_int = (int)sig_min_double;
                            sig_min_int_ptr = &sig_min_int;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_minimum, &sig_min_long) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_long;
                            sig_min_float_ptr = &sig_min_float;
                            sig_min_int = (int)sig_min_long;
                            sig_min_int_ptr = &sig_min_int;
                        }
                        if (dictionary_getfloat((t_dictionary *)temp, sym_maximum, &sig_max_double) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_double;
                            sig_max_float_ptr = &sig_max_float;
                            sig_max_int = (int)sig_max_double;
                            sig_max_int_ptr = &sig_max_int;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_maximum, &sig_max_long) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_long;
                            sig_max_float_ptr = &sig_max_float;
                            sig_min_int = (int)sig_max_long;
                            sig_max_int_ptr = &sig_max_int;
                        }
                        if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0))
                            sig_type_char = 'i';
                        else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0))
                            sig_type_char = 'f';
                        else {
                            post("Skipping registration of signal %s (unknown type).\n", sig_name);
                            continue;
                        }

                        mdev_add_input(x->device, sig_name, (int)sig_length, sig_type_char, sig_units,
                                       sig_type_char == 'i' ? MSIGVALP(sig_min_int_ptr) : MSIGVALP(sig_min_float_ptr), 
                                       sig_type_char == 'i' ? MSIGVALP(sig_max_int_ptr) : MSIGVALP(sig_max_float_ptr), 
                                       0, mapper_int_handler, x);
                    }
                }
            }
            // Get pointer to atom array "outputs"
            if (dictionary_getatomarray((t_dictionary *)device, sym_outputs, &outputs) == MAX_ERR_NONE) {
                atomarray_getatoms((t_atomarray *)outputs, &num_signals, &signals);
                // iterate through array of atoms
                for (i=0; i<num_signals; i++) {
                    // initialize variables
                    if (sig_units) {
                        free(&sig_units);
                    }
                    if (sig_type) {
                        free(&sig_type);
                    }
                    sig_min_int_ptr = sig_max_int_ptr = 0;
                    sig_min_float_ptr = sig_max_float_ptr = 0;
                    sig_length = 1;
                    
                    // each atom object points to a dictionary, need to recover atoms by key
                    temp = atom_getobj(&(signals[i]));
                    if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) == MAX_ERR_NONE) {
                        dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units);
                        dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type);
                        dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length);
                        if (dictionary_getfloat((t_dictionary *)temp, sym_minimum, &sig_min_double) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_double;
                            sig_min_float_ptr = &sig_min_float;
                            sig_min_int = (int)sig_min_double;
                            sig_min_int_ptr = &sig_min_int;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_minimum, &sig_min_long) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_long;
                            sig_min_float_ptr = &sig_min_float;
                            sig_min_int = (int)sig_min_long;
                            sig_min_int_ptr = &sig_min_int;
                        }
                        if (dictionary_getfloat((t_dictionary *)temp, sym_maximum, &sig_max_double) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_double;
                            sig_max_float_ptr = &sig_max_float;
                            sig_max_int = (int)sig_max_double;
                            sig_max_int_ptr = &sig_max_int;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_maximum, &sig_max_long) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_long;
                            sig_max_float_ptr = &sig_max_float;
                            sig_max_int = (int)sig_max_long;
                            sig_max_int_ptr = &sig_max_int;
                        }
                        if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0))
                            sig_type_char = 'i';
                        else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0))
                            sig_type_char = 'f';
                        else {
                            post("Skipping registration of signal %s (unknown type).\n", sig_name);
                            continue;
                        }

                        mdev_add_output(x->device, (int)sig_length, sig_name, sig_units, sig_type_char, 
                                        sig_type_char == 'i' ? MSIGVALP(sig_min_int_ptr) : MSIGVALP(sig_min_float_ptr), 
                                        sig_type_char == 'f' ? MSIGVALP(sig_max_int_ptr) : MSIGVALP(sig_max_float_ptr), 
                                        0, 0, 0);
                    }
                }
            }
        }
    }
}
#endif

// *********************************************************
// -(poll libmapper)----------------------------------------
void mapper_poll(t_mapper *x)
{    
    mdev_poll(x->device, 0);
    if (!x->ready) {
        if (mdev_ready(x->device)) {
            //mapper_db_dump(db);
			x->ready = 1;
			mapper_print_properties(x);
        }
    }
	clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
}
    
// *********************************************************
// -(toggle learning mode)----------------------------------
void mapper_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc == 1) {
        if (argv->a_type == A_FLOAT) {
            x->learn_mode = (int)atom_getfloat(argv);
        }
#ifdef MAXMSP
        else if (argv->a_type == A_LONG) {
            x->learn_mode = (int)atom_getlong(argv);
        }
#endif
    }
}
