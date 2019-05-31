//
//  nencoder~.c
//  pd-ne
//
//  Created by javiernonis on 5/28/19.
//  Copyright © 2019 jnonis. All rights reserved.
//

#include "m_pd.h"
#include "ne.h"

static t_class *nencoder_tilde_class;

typedef struct _nencoder_tilde {
    t_object x_obj;
    t_sample f_dummy;
    
    float in_carrier;
    
    float phase;
    float carrier;

    float out;
    
    t_outlet *x_out;
} t_nencoder_tilde;

t_int *nencoder_tilde_perform(t_int *w) {
    t_nencoder_tilde *x = (t_nencoder_tilde *) (w[1]);
    t_sample *input = (t_sample *) (w[2]);
    t_sample *output = (t_sample *) (w[3]);
    int n = (int) (w[4]);
    
    // Implement a simple sine oscillator for the carrier
    float deltaTime = 1.f / sys_getsr();
    
    // Compute the frequency from the pitch parameter and input
    float pitch = 0.0f;
    pitch = clamp(pitch, -1.0f, 1.0f);
    // The carrier pitch is around C9 - C#9
    float freq = 8697.36f * powf(2.0f, pitch);
    
    for (int i = 0; i < n; i++) {
        // Accumulate the phase
        x->phase += freq * deltaTime;
        if (x->phase >= 1.0f) {
            x->phase -= 1.0f;
        }
    
        // Compute the carrier output
        float sine = sinf(2.0f * M_PI * x->phase);
        x->carrier = sine * x->in_carrier;
            
        float v = x->carrier;
        v *= clamp(input[i], 0.0f, 1.0f);
        output[i] = v;
    }
    
    return (w + 5);
}

void nencoder_tilde_dsp(t_nencoder_tilde *x, t_signal **sp) {
    dsp_add(nencoder_tilde_perform,
            4,
            x,
            sp[0]->s_vec,
            sp[1]->s_vec,
            sp[0]->s_n);
}

void nencoder_tilde_free(t_nencoder_tilde *x) {
    outlet_free(x->x_out);
}

void *nencoder_tilde_new(t_floatarg f) {
    t_nencoder_tilde *x = (t_nencoder_tilde *)pd_new(nencoder_tilde_class);
    
    x->in_carrier = 1.0f;
    
    x->carrier = 0.0f;
    x->phase = 0.0f;
    
    x->out = 0.0f;
    
    x->x_out = outlet_new(&x->x_obj, &s_signal);
    
    return (void *)x;
}

void nencoder_tilde_carrier(t_nencoder_tilde *x, t_floatarg f) {
    x->in_carrier = f;
}

void nencoder_tilde_setup(void) {
    nencoder_tilde_class = class_new(gensym("nencoder~"),
                                     (t_newmethod)nencoder_tilde_new,
                                     (t_method)nencoder_tilde_free,
                                     sizeof(t_nencoder_tilde),
                                     CLASS_DEFAULT,
                                     A_DEFFLOAT,
                                     0);
    class_addmethod(nencoder_tilde_class,
                    (t_method)nencoder_tilde_dsp,
                    gensym("dsp"),
                    A_NULL);
    CLASS_MAINSIGNALIN(nencoder_tilde_class, t_nencoder_tilde, f_dummy);
    
    class_addmethod(nencoder_tilde_class,
                    (t_method)nencoder_tilde_carrier,
                    gensym("carrier"),
                    A_DEFFLOAT,
                    A_NULL);
}
