//
// mapper.c
// a maxmsp and puredata external encapsulating the functionality of libmapper
// http://www.idmil.org/software/libmapper
// Joseph Malloch, IDMIL 2010
//
// This software was written in the Input Devices and Music Interaction
// Laboratory at McGill University in Montreal, and is copyright those
// found in the AUTHORS file.  It is licensed under the GNU Lesser Public
// General License version 2.1 or later.  Please see COPYING for details.
// 

#define MAXMSP // comment for pd, uncomment for maxmsp

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
#include <mapper/mapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <lo/lo.h>

#include <unistd.h>
#include <arpa/inet.h>

#define INTERVAL 1
#define MAX_LIST 256

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
	t_atom buffer[MAX_LIST];
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
void mapper_float_handler(mapper_signal msig, void *v);
void mapper_int_handler(mapper_signal msig, void *v);
void mapper_print_properties(t_mapper *x);
void mapper_read_definition(t_mapper *x);
void mapper_register_signals(t_mapper *x);
void mapper_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
void mapper_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
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
        class_addmethod(c, (method)mapper_learn,          "learn",    A_GIMME,    0);
        class_addmethod(c, (method)mapper_set,            "set",      A_GIMME,    0);
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
        class_addmethod(c, (t_method)mapper_set, gensym("set"), A_GIMME, 0);
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
            #ifdef MAXMSP
            mapper_register_signals(x);
            #endif
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
        post("Freeing device %s...", mdev_name(x->device));
        mdev_free(x->device);
    }
}

// *********************************************************
// -(print properties)--------------------------------------
void mapper_print_properties(t_mapper *x)
{
    t_atom my_list[2];
	char *message;
    
    if (x->ready) {        
        //output name
        message = strdup(mdev_name(x->device));
#ifdef MAXMSP
        atom_setsym(my_list, gensym("name"));
        atom_setsym(my_list + 1, gensym(message));
#else
        SETSYMBOL(my_list, gensym("name"));
        SETSYMBOL(my_list + 1, gensym(message));
#endif
        outlet_list(x->outlet2, ps_list, 2, my_list);
        
        //output IP
        const struct in_addr *ip = mdev_ip4(x->device);
        message = strdup(inet_ntoa(*ip));
#ifdef MAXMSP
        atom_setsym(my_list, gensym("IP"));
        atom_setsym(my_list + 1, gensym(message));
#else
        SETSYMBOL(my_list, gensym("IP"));
        SETSYMBOL(my_list + 1, gensym(message));
#endif
        outlet_list(x->outlet2, ps_list, 2, my_list);
        
        //output port
#ifdef MAXMSP
        atom_setsym(my_list, gensym("port"));
        atom_setlong(my_list + 1, mdev_port(x->device));
#else
        SETSYMBOL(my_list, gensym("port"));
        SETFLOAT(my_list + 1, (float)mdev_port(x->device));
#endif
        outlet_list(x->outlet2, ps_list, 2, my_list);
        
        //output numInputs
#ifdef MAXMSP
        atom_setsym(my_list, gensym("numInputs"));
        atom_setlong(my_list + 1, mdev_num_inputs(x->device));
#else
        SETSYMBOL(my_list, gensym("numInputs"));
        SETFLOAT(my_list + 1, (float)mdev_num_inputs(x->device));
#endif
        outlet_list(x->outlet2, ps_list, 2, my_list);
        
        //output numOutputs
#ifdef MAXMSP
        atom_setsym(my_list, gensym("numOutputs"));
        atom_setlong(my_list + 1, mdev_num_outputs(x->device));
#else
        SETSYMBOL(my_list, gensym("numOutputs"));
        SETFLOAT(my_list + 1, (float)mdev_num_outputs(x->device));
#endif
        outlet_list(x->outlet2, ps_list, 2, my_list);
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
	t_atom my_list[2];
    //need to read attribs: type, units, min/minimum, max/maximum
    //char *type;
    char *sig_name, *sig_units = 0, sig_type = 0;
    int sig_min_int, sig_max_int, sig_length = 1;
    float sig_min_float, sig_max_float;
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
                else if ((strcmp(atom_getsym(argv+i)->s_name, "@min") == 0) ||
                         (strcmp(atom_getsym(argv+i)->s_name, "@minimum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_min_float = atom_getfloat(argv + i + 1);
                        sig_min_int = (int)sig_min_float;
                        i++;
                    }
                    else if ((argv + i + 1)->a_type == A_LONG) {
                        sig_min_int = (int)atom_getlong(argv + i + 1);
                        sig_min_float = (float)sig_min_int;
                        i++;
                    }
                }
                else if ((strcmp(atom_getsym(argv+i)->s_name, "@max") == 0) ||
                         (strcmp(atom_getsym(argv+i)->s_name, "@maximum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_max_float = atom_getfloat(argv + i + 1);
                        sig_max_int = (int)sig_max_float;
                        i++;
                    }
                    else if ((argv + i + 1)->a_type == A_LONG) {
                        sig_max_int = (int)atom_getlong(argv + i + 1);
                        sig_max_float = (float)sig_max_int;
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
                               sig_type == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float, 
                               sig_type == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float, 
                               sig_type == 'i' ? mapper_int_handler : mapper_float_handler, x);
                
                //output numInputs
                atom_setsym(my_list, gensym("numInputs"));
                atom_setlong(my_list + 1, mdev_num_inputs(x->device));
                outlet_list(x->outlet2, ps_list, 2, my_list);
            } 
            else if (strcmp(atom_getsym(argv)->s_name, "output") == 0) {
                mdev_add_output(x->device, atom_getsym(argv + 1)->s_name, sig_length, sig_type, sig_units, 
                                sig_type == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float, 
                                sig_type == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float);
                
                //output numOutputs
                atom_setsym(my_list, gensym("numOutputs"));
                atom_setlong(my_list + 1, mdev_num_outputs(x->device));
                outlet_list(x->outlet2, ps_list, 2, my_list);
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
                        char *temp = strdup((argv+i+1)->a_w.w_symbol->s_name);
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
                else if ((strcmp((argv+i)->a_w.w_symbol->s_name, "@min") == 0) || 
                         (strcmp((argv+i)->a_w.w_symbol->s_name, "@minimum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_min_float = atom_getfloat(argv + i + 1);
                        sig_min_int = (int)sig_min_float;
                        i++;
                    }
                }
                else if ((strcmp((argv+i)->a_w.w_symbol->s_name, "@max") == 0) || 
                         (strcmp((argv+i)->a_w.w_symbol->s_name, "@maximum") == 0)) {
                    if ((argv + i + 1)->a_type == A_FLOAT) {
                        sig_max_float = atom_getfloat(argv + i + 1);
                        sig_max_int = (int)sig_max_float;
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
        if (sig_type) {
            if (strcmp((argv)->a_w.w_symbol->s_name, "input") == 0) {
                    mdev_add_input(x->device, (argv + 1)->a_w.w_symbol->s_name, sig_length, sig_type, sig_units, 
                                   sig_type == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float, 
                                   sig_type == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float, 
                                   sig_type == 'i' ? mapper_int_handler : mapper_float_handler, x);
                
                //output numInputs
                SETSYMBOL(my_list, gensym("numInputs"));
                SETFLOAT(my_list + 1, mdev_num_inputs(x->device));
                outlet_anything(x->outlet2, ps_list, 2, my_list);
            } 
            else if (strcmp((argv)->a_w.w_symbol->s_name, "output") == 0) {
                mdev_add_output(x->device, (argv + 1)->a_w.w_symbol->s_name, sig_length, sig_type, sig_units, 
                                sig_type == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float, 
                                sig_type == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float);
                                
                //output numOutputs
                SETSYMBOL(my_list, gensym("numOutputs"));
                SETFLOAT(my_list + 1, mdev_num_outputs(x->device));
                outlet_anything(x->outlet2, ps_list, 2, my_list);
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
	mapper_signal msig;
    t_atom my_list[2];
    char *sig_name = NULL, *direction = NULL;
    
    if (argc < 2) {
        return;
    }
#ifdef MAXMSP
    if (argv->a_type == A_SYM && (argv+1)->a_type == A_SYM) {
        direction = strdup(atom_getsym(argv)->s_name);
        sig_name = strdup(atom_getsym(argv+1)->s_name);
    }
#else
    if (argv->a_type == A_SYMBOL && (argv+1)->a_type == A_SYMBOL) {
        direction = strdup(argv->a_w.w_symbol->s_name);
        sig_name = strdup((argv+1)->a_w.w_symbol->s_name);
    }
#endif
    else {
        return;
    }
    
    if (strcmp(direction, "output") == 0) {
        if (msig=mdev_get_output_by_name(x->device, sig_name, 0)) {
            mdev_remove_output(x->device, msig);
    #ifdef MAXMSP
            atom_setsym(my_list, gensym("numOutputs"));
            atom_setlong(my_list + 1, (long)mdev_num_outputs(x->device));
    #else
            SETSYMBOL(my_list, gensym("numOutputs"));
            SETFLOAT(my_list, (float)mdev_num_outputs(x->device));
    #endif
            outlet_anything(x->outlet2, ps_list, 2, my_list);
        }
    }
    else if (strcmp(direction, "input") == 0) {
        if (msig=mdev_get_input_by_name(x->device, sig_name, 0)) {
            mdev_remove_input(x->device, msig);
    #ifdef MAXMSP
            atom_setsym(my_list, gensym("numInputs"));
            atom_setlong(my_list + 1, (long)mdev_num_inputs(x->device));
    #else
            SETSYMBOL(my_list, gensym("numInputs"));
            SETFLOAT(my_list, (float)mdev_num_inputs(x->device));
    #endif
            outlet_anything(x->outlet2, ps_list, 2, my_list);
        }
    }
}
    
// *********************************************************
// -(set signal value)--------------------------------------
void mapper_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    // not yet implemented!
}

// *********************************************************
// -(anything)----------------------------------------------
void mapper_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_atom my_list[2];
    
	if (argc) {
        //find signal
        mapper_signal msig;
        if (!(msig=mdev_get_output_by_name(x->device, s->s_name, 0))) {
            if (x->learn_mode) {
                // register as new signal
                if (argv->a_type == A_FLOAT) {
                    msig = mdev_add_output(x->device, s->s_name, argc, 'f', 0, 0, 0);
                }
#ifdef MAXMSP
                else if (argv->a_type == A_LONG) {
                    msig = mdev_add_output(x->device, s->s_name, argc, 'i', 0, 0, 0);
                }
#endif
                else {
                    return;
                }
                //output updated numOutputs
#ifdef MAXMSP
                atom_setsym(my_list, gensym("numOutputs"));
                atom_setlong(my_list + 1, mdev_num_outputs(x->device));
#else
                SETSYMBOL(my_list, gensym("numOutputs"));
                SETFLOAT(my_list + 1, mdev_num_outputs(x->device));
#endif
                outlet_anything(x->outlet2, ps_list, 2, my_list);
            }
            else {
                return;
            }

        }
        mapper_db_signal props = msig_properties(msig);
        if (props->length != argc) {
            post("Vector length does not match signal definition!\n");
            return;
        }
        if (props->type == 'i') {
            int payload[props->length];
            for (i = 0; i < argc; i++) {
                if ((argv + i)->a_type == A_FLOAT)
                    payload[i] = (int)atom_getfloat(argv + i);
#ifdef MAXMSP
                else if ((argv + i)->a_type == A_LONG)
                    payload[i] = (int)atom_getlong(argv + i);
#endif
                
            }
            //update signal
            msig_update(msig, payload);
        }
        else if (props->type == 'f') {
            float payload[props->length];
            for (i = 0; i < argc; i++) {
                if ((argv + i)->a_type == A_FLOAT)
                    payload[i] = atom_getfloat(argv + i);
#ifdef MAXMSP
                else if ((argv + i)->a_type == A_LONG)
                    payload[i] = (float)atom_getlong(argv + i);
#endif
                
            }
            //update signal
            msig_update(msig, payload);
        }
        else {
            return;
        }
    }
}

// *********************************************************
// -(int handler)-------------------------------------------
void mapper_int_handler(mapper_signal msig, void *v)
{
    mapper_db_signal props = msig_properties(msig);
    t_mapper *x = props->user_data;
	char *path = strdup(props->name);
    int i, length = props->length;
    int *pi = (int*)v;
	
	if (length > (MAX_LIST-1)) {
		post("Maximum list length is %i!\n", MAX_LIST-1);
		length = MAX_LIST-1;
	}

#ifdef MAXMSP
    atom_setsym(x->buffer, gensym(path));
    for (i = 0; i < length; i++) {
        atom_setlong(x->buffer + i + 1, (long)*(pi+i));
    }
#else
    SETSYMBOL(my_list, gensym(path));
    for (i = 0; i < length; i++) {
        SETFLOAT(x->buffer + i + 1, (float)*(pi+i));
    }
#endif
    outlet_list(x->outlet1, ps_list, length+1, x->buffer);
}

// *********************************************************
// -(float handler)-----------------------------------------
void mapper_float_handler(mapper_signal msig, void *v)
{
    mapper_db_signal props = msig_properties(msig);
    t_mapper *x = props->user_data;
	char *path = strdup(props->name);
    int i, length = props->length;
    float *pf = (float*)v;
	
	if (length > (MAX_LIST-1)) {
		post("Maximum list length is %i!\n", MAX_LIST-1);
		length = MAX_LIST-1;
	}
	
#ifdef MAXMSP
    atom_setsym(x->buffer, gensym(path));
    for (i = 0; i < length; i++) {
        atom_setfloat(x->buffer + i + 1, *(pf+i));
    }
#else
    SETSYMBOL(my_list, gensym(path));
    for (i = 0; i < length; i++) {
        SETFLOAT(x->buffer + i + 1, *(pf+i));
    }
#endif
    outlet_list(x->outlet1, ps_list, length+1, x->buffer);
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
    float sig_min_float, sig_max_float;
    long sig_min_long, sig_max_long, sig_length;
    int sig_min_int, sig_max_int;
    
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
                    sig_length = 1;
                    
                    // each atom object points to a dictionary, need to recover atoms by key
                    temp = atom_getobj(&(signals[i]));
                    if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) == MAX_ERR_NONE) {
                        dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units);
                        dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type);
                        dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length);
                        if (dictionary_getfloat((t_dictionary *)temp, sym_minimum, &sig_min_double) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_double;
                            sig_min_int = (int)sig_min_double;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_minimum, &sig_min_long) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_long;
                            sig_min_int = (int)sig_min_long;
                        }
                        if (dictionary_getfloat((t_dictionary *)temp, sym_maximum, &sig_max_double) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_double;
                            sig_max_int = (int)sig_max_double;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_maximum, &sig_max_long) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_long;
                            sig_min_int = (int)sig_max_long;
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
                                       sig_type_char == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float, 
                                       sig_type_char == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float, 
                                       sig_type_char == 'i' ? mapper_int_handler : mapper_float_handler, x);
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
                    sig_length = 1;
                    
                    // each atom object points to a dictionary, need to recover atoms by key
                    temp = atom_getobj(&(signals[i]));
                    if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) == MAX_ERR_NONE) {
                        dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units);
                        dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type);
                        dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length);
                        if (dictionary_getfloat((t_dictionary *)temp, sym_minimum, &sig_min_double) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_double;
                            sig_min_int = (int)sig_min_double;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_minimum, &sig_min_long) == MAX_ERR_NONE) {
                            sig_min_float = (float)sig_min_long;
                            sig_min_int = (int)sig_min_long;
                        }
                        if (dictionary_getfloat((t_dictionary *)temp, sym_maximum, &sig_max_double) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_double;
                            sig_max_int = (int)sig_max_double;
                        }
                        else if (dictionary_getlong((t_dictionary *)temp, sym_maximum, &sig_max_long) == MAX_ERR_NONE) {
                            sig_max_float = (float)sig_max_long;
                            sig_max_int = (int)sig_max_long;
                        }
                        if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0))
                            sig_type_char = 'i';
                        else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0))
                            sig_type_char = 'f';
                        else {
                            post("Skipping registration of signal %s (unknown type).\n", sig_name);
                            continue;
                        }

                        mdev_add_output(x->device, sig_name, (int)sig_length, sig_type_char, sig_units, 
                                        sig_type_char == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float, 
                                        sig_type_char == 'f' ? (void *)&sig_max_int : (void *)&sig_max_float);
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
    int mode = x->learn_mode;
    
    if (argc > 0) {
        if (argv->a_type == A_FLOAT) {
            mode = (int)atom_getfloat(argv);
        }
#ifdef MAXMSP
        else if (argv->a_type == A_LONG) {
            mode = (int)atom_getlong(argv);
        }
#endif
        
        if (mode != x->learn_mode) {
            x->learn_mode = mode;
            if (mode == 0)
                post("Learning mode off.\n");
            else
                post("Learning mode on.\n");
        }
    }
}
